#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXFILA 8
#define TOTAL 64
#define NUM_PRODUTORES 2
#define NUM_CONSUMIDORES 2

static int buffer[MAXFILA];
static int inicio = 0;
static int fim = 0;
static int qtd = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t naoCheio = PTHREAD_COND_INITIALIZER;
static pthread_cond_t naoVazio = PTHREAD_COND_INITIALIZER;

void *Produtor(void *arg);
void *Consumidor(void *arg);

int main(void)
{
    pthread_t produtores[NUM_PRODUTORES];
    pthread_t consumidores[NUM_CONSUMIDORES];
    int t;

    printf("Inicio\n");
    for (t = 0; t < NUM_PRODUTORES; t++)
        pthread_create(&produtores[t], NULL, Produtor, (void *)(intptr_t) t);
    for (t = 0; t < NUM_CONSUMIDORES; t++)
        pthread_create(&consumidores[t], NULL, Consumidor, (void *)(intptr_t) t);

    for (t = 0; t < NUM_PRODUTORES; t++)
        pthread_join(produtores[t], NULL);
    for (t = 0; t < NUM_CONSUMIDORES; t++)
        pthread_join(consumidores[t], NULL);
    printf("\nFim!!\n");

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&naoCheio);
    pthread_cond_destroy(&naoVazio);
    return 0;
}

void *Produtor(void *arg)
{
    int id = (int)(intptr_t) arg;

    for (int i = 1; i <= TOTAL / NUM_PRODUTORES; i++)
    {
        sleep(1);
        int valor = rand() % 100;

        pthread_mutex_lock(&mutex);

        while (qtd == MAXFILA)
            pthread_cond_wait(&naoCheio, &mutex);

        buffer[fim] = valor;
        fim = (fim + 1) % MAXFILA;
        qtd++;
        printf("Produtor %d:   valor = %2d | fila com %d\n", id, valor, qtd);

        pthread_cond_signal(&naoVazio);
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

void *Consumidor(void *arg)
{
    int id = (int)(intptr_t) arg;

    for (int i = 1; i <= TOTAL / NUM_CONSUMIDORES; i++)
    {
        sleep(2);

        pthread_mutex_lock(&mutex);
        while (qtd == 0)
            pthread_cond_wait(&naoVazio, &mutex);

        int valor = buffer[inicio];
        inicio = (inicio + 1) % MAXFILA;
        qtd--;
        printf("Consumidor %d: valor = %2d | fila com %d\n", id, valor, qtd);

        pthread_cond_signal(&naoCheio);
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}
