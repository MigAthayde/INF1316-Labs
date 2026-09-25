#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

int main(void)
{

    int pid1, pid2;
    int fifo;
    char mensagem[21];
    if (mkfifo("filhoFifo", S_IRUSR | S_IWUSR) == 0)
    {
        puts("FIFO de chat criada com sucesso");
    }
    else if (errno == EEXIST)
    {
        puts("FIFO de chat ja existe, reutilizando");
    }
    else
    {
        puts("Erro ao criar FIFO de chat");
        perror("mkfifo");
        exit(1);
    }

    fifo = open("filhoFifo", O_RDONLY | O_NONBLOCK);
    if (fifo == -1)
    {
        puts("Erro ao abrir FIFO de chat");
        perror("open");
        exit(1);
    }

    pid1 = fork();
    if (pid1 < 0)
    {
        puts("Erro ao criar processo filho 1");
        exit(1);
    }
    else if (pid1 == 0)
    {
        int fifo_filho = open("filhoFifo", O_WRONLY);
        write(fifo_filho, "Mensagem do filho 1", 20);
        close(fifo_filho);
        exit(0);
    }

    pid2 = fork();
    if (pid2 < 0)
    {
        puts("Erro ao criar processo filho 2");
        exit(1);
    }
    else if (pid2 == 0)
    {
        int fifo_filho = open("filhoFifo", O_WRONLY);
        write(fifo_filho, "Mensagem do filho 2", 20);
        close(fifo_filho);
        exit(0);
    }

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    read(fifo, mensagem, 20);
    mensagem[20] = '\0';
    printf("Mensagem recebida: %s\n", mensagem);

    read(fifo, mensagem, 20);
    mensagem[20] = '\0';
    printf("Mensagem recebida: %s\n", mensagem);

    close(fifo);

    return 0;
}