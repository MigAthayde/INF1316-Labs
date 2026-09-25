#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

int main(void)
{
    int fifo_pedidos, fifo_respostas;
    char mensagem[100];
    int pid_cliente;
    char texto[100];
    int i;

    if (mkfifo("pedidosFifo", S_IRUSR | S_IWUSR) == 0)
    {
        puts("FIFO de pedidos criada com sucesso");
    }
    else if (errno == EEXIST)
    {
        puts("FIFO de pedidos ja existe, reutilizando");
    }
    else
    {
        puts("Erro ao criar FIFO de pedidos");
        perror("mkfifo");
        exit(1);
    }

    if (mkfifo("serverFifo", S_IRUSR | S_IWUSR) == 0)
    {
        puts("FIFO de respostas criada com sucesso");
    }
    else if (errno == EEXIST)
    {
        puts("FIFO de respostas ja existe, reutilizando");
    }
    else
    {
        puts("Erro ao criar FIFO de respostas");
        perror("mkfifo");
        exit(1);
    }

    fifo_pedidos = open("pedidosFifo", O_RDWR);
    if (fifo_pedidos == -1)
    {
        perror("open pedidosFifo");
        exit(1);
    }

    fifo_respostas = open("serverFifo", O_RDWR);
    if (fifo_respostas == -1)
    {
        perror("open serverFifo");
        exit(1);
    }

    while (read(fifo_pedidos, mensagem, sizeof(mensagem)) > 0)
    {
        sscanf(mensagem, "%d|%[^\n]", &pid_cliente, texto);

        for (i = 0; texto[i] != '\0'; i++)
        {
            texto[i] = toupper((unsigned char) texto[i]);
        }

        printf("Pedido de %d: %s -> %s\n", pid_cliente, mensagem, texto);

        snprintf(mensagem, sizeof(mensagem), "%d|%s", pid_cliente, texto);
        write(fifo_respostas, mensagem, strlen(mensagem) + 1);
    }

    close(fifo_pedidos);
    close(fifo_respostas);

    return 0;
}
