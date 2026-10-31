#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include<sys/time.h>

int main (int argc, char *argv[])
{
    struct stat sb;
    off_t len;
    char *p;
    int fd;
    struct timeval inicio, fim;
    long tempo_mmap, tempo_disco;

    if (argc < 2) { 
        fprintf (stderr, "usage: %s <file>\n", argv[0]); 
        exit(1); 
    }

    fd = open (argv[1], O_RDONLY);
    if (fd == -1) { 
        perror ("open"); 
        exit(1); 
    }
    if (fstat(fd, &sb)== -1) { 
        perror ("fstat"); 
        exit(1);
    }

    if (!S_ISREG (sb.st_mode)) { 
        fprintf (stderr, "%s is not a file\n", argv[1]); 
        exit(1);
    }
    p = mmap (0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) { 
        perror ("mmap"); exit(1); 
    }

    gettimeofday(&inicio, NULL);
    for (len = 0; len < sb.st_size; len++) 
        putchar (p[len]);
    gettimeofday(&fim, NULL);
    tempo_mmap = (fim.tv_sec - inicio.tv_sec) * 1000000L + (fim.tv_usec - inicio.tv_usec);

    long pagesize = sysconf(_SC_PAGESIZE);
    long pages = (sb.st_size + pagesize -1) / pagesize;

    lseek(fd, 0, SEEK_SET); // garante que o cursor está no início do arquivo
    char c;
    gettimeofday(&inicio, NULL);
    for (len = 0; len < sb.st_size; len++) {
        read(fd, &c, 1);
        putchar(c);
    }
    gettimeofday(&fim, NULL);
    if (close (fd) == -1) { 
        perror ("close"); exit(1); 
    }
    tempo_disco = (fim.tv_sec - inicio.tv_sec) * 1000000L + (fim.tv_usec - inicio.tv_usec);
    printf("\nTempo com mmap:  %ld us\n", tempo_mmap);
    printf("Tempo com disco: %ld us\n", tempo_disco);

    printf("Tamanho do arquivo: %ld bytes\n", (long)sb.st_size);
    printf("Tamanho da pagina:  %ld bytes\n", pagesize);
    printf("Paginas utilizadas: %ld\n", pages);

    if (munmap (p, sb.st_size) == -1) {
        perror ("munmap"); 
        exit(1); 
    }
    return 0;
}