#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#define TAM 100
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo>\n", argv[0]);
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);
    if (fd == -1) { 
        perror("open"); 
        exit(1); 
    }
    char *p = mmap(0, TAM, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap"); 
        exit(1); 
    }

    printf("[prog2] Li da memoria mapeada: \"%s\"\n", p);

    strcpy(p, "Mensagem de prog1 lida com sucesso. Agora essa é a mensagem de prog2\n");

    printf("[prog2] Enviei resposta para o prog1.\n");

    close(fd);

    munmap(p, TAM);
    return 0;
}