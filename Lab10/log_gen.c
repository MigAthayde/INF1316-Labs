#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_KEY 999
#define MIN_KEY 0

typedef enum {
    READ, WRITE
} OpType;

void gerar_operacoes(FILE *out, int n)
{
    srand(time(NULL));
    for (int i = 0; i < n; i++)
    {
        OpType op = rand() % 2;
        int key = (rand() % (MAX_KEY - MIN_KEY + 1)) + MIN_KEY;

        if(op == READ)
        {
            fprintf(out, "R, %d\n", key);
        }
        else
        {
            int value = (rand() % 4991) + 10;
            fprintf(out, "W, %d, %d\n", value, key);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Programa não aceita mais de 1 argumento");
        exit(1);
    }

    FILE *f = fopen("log.txt", "w");
    if(!f)
    {
        perror("fopen");
        exit(1);
    }

    int n = atoi(argv[1]);
    gerar_operacoes(f, n);

    return 0;
}
