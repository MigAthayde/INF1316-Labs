#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXFILA 8
#define TOTAL 64

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
    pthread_t produtor, consumidor;

    printf("Inicio\n");
    pthread_create(&produtor, NULL, Produtor, NULL);
    pthread_create(&consumidor, NULL, Consumidor, NULL);
    pthread_join(produtor, NULL);
    pthread_join(consumidor, NULL);
    printf("\nFim!!\n");

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&naoCheio);
    pthread_cond_destroy(&naoVazio);
    return 0;
}

void *Produtor(void *arg)
{
    for (int i = 1; i <= TOTAL; i++)
    {
        sleep(1);
        int valor = rand() % 100;

        pthread_mutex_lock(&mutex);
        while (qtd == MAXFILA)
            pthread_cond_wait(&naoCheio, &mutex);

        buffer[fim] = valor;
        fim = (fim + 1) % MAXFILA;
        qtd++;
        printf("Produtor:   %2do valor = %2d | fila com %d\n", i, valor, qtd);

        pthread_cond_signal(&naoVazio);
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

void *Consumidor(void *arg)
{
    for (int i = 1; i <= TOTAL; i++)
    {
        sleep(2);

        pthread_mutex_lock(&mutex);
        while (qtd == 0)
            pthread_cond_wait(&naoVazio, &mutex);

        int valor = buffer[inicio];
        inicio = (inicio + 1) % MAXFILA;
        qtd--;
        printf("Consumidor: %2do valor = %2d | fila com %d\n", i, valor, qtd);

        pthread_cond_signal(&naoCheio);
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}
