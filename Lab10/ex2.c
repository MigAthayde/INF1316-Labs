#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define DELAY 50000
#define CACHE_HIT 1
#define CACHE_MISS 0
#define CACHE_SIZE 100
#define DB_SIZE 1000
#define DB_SUCCESS 1
#define DB_ERROR 0

typedef enum { OP_GET, OP_PUT, OP_INVALIDATE, OP_DB_READ, OP_DB_WRITE } ReqType;

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

int call(Mailbox *mb, ReqType type, int key, int *value);
int db_read(int key, int* out_value);
int db_write(int key, int new_value);
int cache_find(int key);
int cache_get(int key, int* out_value);
int cache_put(int key, int new_value);
void *db_thread(void *arg);

int main(void)
{
    for (int i = 0; i < DB_SIZE; i++)
    {
        banco_de_dados[i] = i+1;
    }

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
    // Atualiza o registro no banco de dados com o novo valor
    banco_de_dados[key] = new_value;
    usleep(DELAY);
    printf("[DB-WRITE] Gravando no Banco -> Chave: %d, Novo Valor: %d\n", key, new_value);
    return DB_SUCCESS;
}

// Procura o slot que guarda essa key. Devolve o índice, ou -1 se não estiver no cache.
int cache_find(int key) {
    for (int i = 0; i < CACHE_SIZE; i++)
        if (is_valid[i] && cache[i].key == key)
            return i;
    return -1;
}

int cache_get(int key, int* out_value) {
    int i = cache_find(key);
    if (i >= 0) {
        // CACHE HIT: devolve o valor e marca o acesso
        *out_value = cache[i].value;
        cache[i].timestamp = ++tick;
        return CACHE_HIT;
    }
    // CACHE MISS: a key não está em nenhum slot
    return CACHE_MISS;
}

int cache_put(int key, int new_value) {
    int i = cache_find(key);
    if (i >= 0) {
        // Key já está no cache: só atualiza
        cache[i].value = new_value;
        cache[i].timestamp = ++tick;
        return CACHE_HIT;
    }

    // Key nova: procura um slot vazio e, ao mesmo tempo, o mais antigo
    int alvo = -1;
    int mais_antigo = 0;
    for (int j = 0; j < CACHE_SIZE; j++) {
        if (!is_valid[j]) { alvo = j; break; }
        if (cache[j].timestamp < cache[mais_antigo].timestamp)
            mais_antigo = j;
    }
    if (alvo < 0)
        alvo = mais_antigo;   // cache cheio: substitui a entrada mais antiga

    cache[alvo].key = key;
    cache[alvo].value = new_value;
    cache[alvo].timestamp = ++tick;
    is_valid[alvo] = 1;
    return CACHE_MISS;
}

void *db_thread(void *arg)
{
    (void)arg;
    Mailbox *mb = &db_box;

    pthread_mutex_lock(&mb->mtx);
    while(1)
    {
        // Lógica da thread dormir até ter pedido + resposta, orchestrando essa lógica
    }
}