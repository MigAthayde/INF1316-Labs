#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define DELAY 50000
#define CACHE_HIT 1
#define CACHE_MISS 0
#define CACHE_SIZE 100
#define DB_SIZE 1000
#define DB_SUCCESS 1
#define DB_ERROR 0

// OP_WRITE (novo): escrita write-through -> grava no cache E repassa ao banco
typedef enum { OP_GET, OP_PUT, OP_WRITE, OP_INVALIDATE, OP_DB_READ, OP_DB_WRITE } ReqType;
long n_ops = 0, hits = 0, misses = 0, db_reads = 0, db_writes = 0;

typedef struct {
    pthread_mutex_t mtx;
    pthread_cond_t  cond_req;   // servidor espera aqui
    pthread_cond_t  cond_resp;  // cliente espera aqui
    int has_req, has_resp;
    ReqType type;
    int key, value;             // entrada / saída
    int result;                 // CACHE_HIT/MISS, DB_SUCCESS/ERROR
} Mailbox;

typedef struct {
    int key;
    // Integer in the range 0..999
    int value;
    // Integer value
    time_t timestamp; // time when entry was written/accessed
} CacheEntry;

Mailbox cache_box = { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER };
Mailbox db_box    = { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER };

int banco_de_dados[DB_SIZE];
CacheEntry cache[CACHE_SIZE];
int is_valid[CACHE_SIZE] = {0};

long tick = 0;

// Flag marcada pelo handler do Ctrl-Z; quem limpa o cache de fato é a cache_thread
volatile sig_atomic_t limpar_cache = 0;

// Instante em que o programa começou (para medir o tempo no item 3)
struct timespec t_inicio;

int call(Mailbox *mb, ReqType type, int key, int *value);
int db_read(int key, int* out_value);
int db_write(int key, int new_value);
int cache_find(int key);
int cache_get(int key, int* out_value);
int cache_put(int key, int new_value);
void cache_invalidate(int key);
void cache_clear(void);
void on_tstp(int sig);
void *db_thread(void *arg);
void *cache_thread(void *arg);
void *cliente_thread(void *arg);

int main(void)
{
    for (int i = 0; i < DB_SIZE; i++)
    {
        banco_de_dados[i] = i+1;
    }

    struct sigaction sa = {0};
    sa.sa_handler = on_tstp;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa, NULL);

    clock_gettime(CLOCK_MONOTONIC, &t_inicio);

    pthread_t tdb, tcache, tcli;
    pthread_create(&tdb, NULL, db_thread, NULL);
    pthread_create(&tcache, NULL, cache_thread, NULL);
    pthread_create(&tcli, NULL, cliente_thread, NULL);

    pthread_join(tcli, NULL); 
    return 0;
}

int call(Mailbox *mb, ReqType type, int key, int *value)
{
    pthread_mutex_lock(&mb->mtx);

    while(mb->has_req || mb->has_resp)
    {
        pthread_cond_wait(&mb->cond_resp, &mb->mtx);
    }
    mb->type = type;
    mb->key = key;
    mb->value = value ? *value : 0;
    mb->has_req = 1;
    pthread_cond_signal(&mb->cond_req);

    while(!mb->has_resp)
    {
        pthread_cond_wait(&mb->cond_resp, &mb->mtx);
    }
    if(value)
    {
        *value = mb->value;
    }
    int r = mb->result;
    mb->has_resp = 0;
    pthread_cond_broadcast(&mb->cond_resp);
    pthread_mutex_unlock(&mb->mtx);
    return r;
}

int db_read(int key, int* out_value) {
    // Validação de segurança dos limites do banco de dados
    if (key < 0 || key >= DB_SIZE) {
        printf("[DB-ERRO] Tentativa de leitura em chave inválida: %d\n", key);
        return DB_ERROR;
    }
    // Copia o valor do banco de dados para o ponteiro de saída
    *out_value = banco_de_dados[key];
    printf("[DB-READ] Lendo do Banco -> Chave: %d, Valor: %d\n", key, *out_value);
    usleep(DELAY);
    return DB_SUCCESS;
}

int db_write(int key, int new_value) {
    // Validação de segurança dos limites do banco de dados
    if (key < 0 || key >= DB_SIZE) {
        printf("[DB-ERRO] Tentativa de escrita em chave inválida: %d\n", key);
        return DB_ERROR;
    }
    banco_de_dados[key] = new_value;
    usleep(DELAY);
    printf("[DB-WRITE] Gravando no Banco -> Chave: %d, Novo Valor: %d\n", key, new_value);
    return DB_SUCCESS;
}


int cache_find(int key) {
    for (int i = 0; i < CACHE_SIZE; i++)
        if (is_valid[i] && cache[i].key == key)
            return i;
    return -1;
}

int cache_get(int key, int* out_value) {
    int i = cache_find(key);
    if (i >= 0) {
        *out_value = cache[i].value;
        cache[i].timestamp = ++tick;
        return CACHE_HIT;
    }
    return CACHE_MISS;
}

int cache_put(int key, int new_value) {
    int i = cache_find(key);
    if (i >= 0) {

        cache[i].value = new_value;
        cache[i].timestamp = ++tick;
        return CACHE_HIT;
    }


    int alvo = -1;
    int mais_antigo = 0;
    for (int j = 0; j < CACHE_SIZE; j++) {
        if (!is_valid[j]) { alvo = j; break; }
        if (cache[j].timestamp < cache[mais_antigo].timestamp)
            mais_antigo = j;
    }
    if (alvo < 0)
        alvo = mais_antigo;

    cache[alvo].key = key;
    cache[alvo].value = new_value;
    cache[alvo].timestamp = ++tick;
    is_valid[alvo] = 1;
    return CACHE_MISS;
}


void cache_invalidate(int key) {
    int i = cache_find(key);
    if (i >= 0)
        is_valid[i] = 0;
}

void cache_clear(void) {
    for (int i = 0; i < CACHE_SIZE; i++)
        is_valid[i] = 0;
}

void on_tstp(int sig)
{
    (void)sig;
    limpar_cache = 1;
}

void *db_thread(void *arg)
{
    (void)arg;
    Mailbox *mb = &db_box;

    pthread_mutex_lock(&mb->mtx);
    while(1)
    {
        while(!mb->has_req)
        {
            pthread_cond_wait(&mb->cond_req, &mb->mtx);
        }

        if(mb->type == OP_DB_READ)
        {
            mb->result = db_read(mb->key, &mb->value);
        }
        else if(mb->type == OP_DB_WRITE)
        {
            mb->result = db_write(mb->key, mb->value);
        }
        else
        {
            mb->result = DB_ERROR;
        }

        mb->has_req = 0;
        mb->has_resp = 1;
        pthread_cond_signal(&mb->cond_resp);
    }

    return NULL;
}

void *cache_thread(void *arg)
{
    (void)arg;
    Mailbox *mb = &cache_box;

    pthread_mutex_lock(&mb->mtx);
    while(1)
    {
        while(!mb->has_req)
        {
            pthread_cond_wait(&mb->cond_req, &mb->mtx);
        }

        if(limpar_cache)
        {
            cache_clear();
            limpar_cache = 0;
            printf("\n[CACHE] Ctrl-Z recebido: cache limpo!\n");
        }

        if(mb->type == OP_GET)
        {
            mb->result = cache_get(mb->key, &mb->value);
        }
        else if(mb->type == OP_PUT)
        {
            // usado no miss de leitura: só copia o valor do banco para o cache
            mb->result = cache_put(mb->key, mb->value);
        }
        else if(mb->type == OP_WRITE)
        {
            // WRITE-THROUGH: atualiza o cache e, em seguida, o próprio cache
            // repassa a escrita ao banco (seta cache <-> banco do diagrama).
            // Só responde ao cliente depois que o banco confirmou.
            cache_put(mb->key, mb->value);
            int v = mb->value;
            mb->result = call(&db_box, OP_DB_WRITE, mb->key, &v);
        }
        else if(mb->type == OP_INVALIDATE)
        {
            cache_invalidate(mb->key);
            mb->result = 0;
        }
        else
        {
            mb->result = CACHE_MISS;
        }

        mb->has_req = 0;
        mb->has_resp = 1;
        pthread_cond_signal(&mb->cond_resp);
    }

    return NULL;
}

void *cliente_thread(void *arg)
{
    (void)arg;
    FILE *f = fopen("log.txt", "r");
    if (!f)
    {
        perror("log.txt");
        exit(1);
    }

    char op;
    int key, value;
    while(1)
    {
        if(fscanf(f, " %c,", &op) != 1)
        {
            rewind(f);   // fim do log: recomeça do início (leitura cíclica)
            continue;
        }

        if(op == 'R')
        {
            fscanf(f, "%d", &key);
            if (call(&cache_box, OP_GET, key, &value) == CACHE_HIT)
            {
                hits++;
            }
            else{
                misses++;
                call(&db_box, OP_DB_READ, key, &value);
                db_reads++;
                call(&cache_box, OP_PUT, key, &value);
            }
        }
        else
        {
            fscanf(f, "%d, %d", &value, &key);
            // WRITE-THROUGH: o cliente só fala com o cache; o cache cuida do banco
            call(&cache_box, OP_WRITE, key, &value);
            db_writes++;
        }

        n_ops++;
        if(n_ops % 500 == 0)
        {
            struct timespec agora;
            clock_gettime(CLOCK_MONOTONIC, &agora);
            double seg = (agora.tv_sec - t_inicio.tv_sec) + (agora.tv_nsec - t_inicio.tv_nsec) / 1e9;
            printf(">>> [WT] ops=%ld hits=%ld misses=%ld db_reads=%ld db_writes=%ld tempo=%.2fs\n", n_ops, hits, misses, db_reads, db_writes, seg);
        }
    }
    return NULL;
}
