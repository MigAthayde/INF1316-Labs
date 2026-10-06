#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

int main(void)
{
    int fifo_pedidos, fifo_respostas;
    char mensagem[100];
    char texto[100];
    char resposta[100];
    int meu_pid;
    int pid_resposta;
    char texto_resposta[100];

    meu_pid = getpid();

    fifo_pedidos = open("pedidosFifo", O_WRONLY);
    if (fifo_pedidos == -1)
    {
        perror("open pedidosFifo");
        exit(1);
    }

    fifo_respostas = open("serverFifo", O_RDONLY);
    if (fifo_respostas == -1)
    {
        perror("open serverFifo");
        exit(1);
    }

    while (1)
    {
        printf("%d> ", meu_pid);
        if (fgets(texto, sizeof(texto), stdin) == NULL)
        {
            break;
        }
        texto[strcspn(texto, "\n")] = '\0';

        if (strcmp(texto, "sair") == 0)
        {
            break;
        }

        snprintf(mensagem, sizeof(mensagem), "%d|%s", meu_pid, texto);
        write(fifo_pedidos, mensagem, strlen(mensagem) + 1);

        do
        {
            if (read(fifo_respostas, resposta, sizeof(resposta)) <= 0)
            {
                break;
            }
            sscanf(resposta, "%d|%[^\n]", &pid_resposta, texto_resposta);
        } while (pid_resposta != meu_pid);

        printf("Resposta: %s\n", texto_resposta);
    }

    close(fifo_pedidos);
    close(fifo_respostas);

    return 0;
}
