#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define TAM 100

int main(int argc, char *argv[])
{

    if (argc < 2) { 
        fprintf (stderr, "usage: %s <file>\n", argv[0]); 
        exit(1);
    }

    int fd = open(argv[1], O_RDWR | O_CREAT, 0666);
    if (fd == -1) { 
        perror ("open"); 
        exit(1); 
    }
    if (ftruncate(fd, TAM) == -1) { 
        perror ("ftruncate"); 
        exit(1); 
    }
    char *p = (char*) mmap(NULL, TAM, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) { 
        perror ("mmap"); 
        exit(1); 
    }
    close(fd);

    strcpy(p, "Mensagem enviada pelo prog1!\n");
    printf("[prog1] Escrevi no arquivo mapeado: \"%s\"", p);
    printf("[prog1] Aguardando prog2 responder...\n");

    while (strcmp(p, "Mensagem enviada pelo prog1!\n") == 0) {
        sleep(1);
    }
    printf("[prog1] Recebi resposta do prog2: \"%s\"\n", p);

    munmap(p, TAM);
    return 0;
}