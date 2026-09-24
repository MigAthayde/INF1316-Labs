#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(void)
{

    int fifo;
    char mensagem[100];
    
    if( mkfifo("chatFifo", S_IRUSR | S_IWUSR) == 0)
    {
        puts("FIFO de chat criada com sucesso");
    }
    else
    {
        puts("Erro ao criar FIFO de chat");
        exit(1);
    }

    fifo = open("chatFifo", O_RDONLY);
    if (fifo == -1)
    {
        puts("Erro ao abrir FIFO de chat");
        perror("open");
        exit(1);
    }
    else{
        puts("Iniciando chat via FIFO...");
    }

    while(read(fifo, &mensagem, sizeof(mensagem)) > 0)
    {
        printf("USR2> ");
        printf("%s", mensagem);
    }
    close(fifo);

    return 0;
}