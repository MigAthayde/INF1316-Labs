#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define FIFO "chatFifo"

int main(void)
{

    int fifo;
    char mensagem[100];
    fifo = open(FIFO, O_WRONLY);
    if (fifo == -1)
    {
        puts("Erro ao abrir FIFO de chat");
        exit(1);
    }
    else{
        puts("Iniciando chat via FIFO...");
    }

    while(1)
    {
        printf("USR1> ");
        fgets(mensagem, sizeof(mensagem), stdin);
        write(fifo, mensagem, sizeof(mensagem));

        if(strcmp(mensagem, "sair\n") == 0)
        {
            break;
        }
    }

    close(fifo);

    return 0;
}