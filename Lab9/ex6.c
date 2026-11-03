#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char *argv[])
{
    struct stat sb;
    char *p;
    int fd;
    off_t tam_antigo, tam_novo;
    char *texto = "Texto adicionado no final pelo ex5!\n";

    if (argc < 2) {
        fprintf (stderr, "usage: %s <file> [texto]\n", argv[0]);
        exit(1);
    }

    if(argc >= 3)
    {
        texto = argv[2];
    }

    fd = open (argv[1], O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror ("open");
        exit(1);
    }
    if (fstat(fd, &sb) == -1) {
        perror ("fstat");
        exit(1);
    }
    if (!S_ISREG (sb.st_mode)) {
        fprintf (stderr, "%s is not a file\n", argv[1]);
        exit(1);
    }

    tam_antigo = sb.st_size;
    if(tam_antigo == 0)
    {
        fprintf(stderr, "Arquivo vazio, use um arquivo existente como base.\n");
        exit(1);
    }
    p = mmap (0, tam_antigo, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror ("mmap"); exit(1);
    }

    tam_novo = tam_antigo + strlen(texto);

    if (ftruncate(fd, tam_novo) == -1) {
        perror ("ftruncate");
        exit(1);
    }

    p = mremap(p, tam_antigo, tam_novo, MREMAP_MAYMOVE);
    if (p == MAP_FAILED)
    {
        perror("mremap");
        exit(1);
    }
    if (close (fd) == -1) {
        perror ("close"); exit(1);
    }

    memcpy(p + tam_antigo, texto, strlen(texto));

    printf("Tamanho antigo: %ld bytes\n", (long)tam_antigo);
    printf("Tamanho novo:   %ld bytes\n", (long)tam_novo);

    if (munmap (p, tam_novo) == -1) {
        perror ("munmap");
        exit(1);
    }
    return 0;
}
