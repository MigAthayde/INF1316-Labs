#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/param.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int file_select(const struct dirent *entry)
{
    return strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0;
}

long long soma_diretorio(const char*dir)
{
    struct dirent **files;
    struct stat st;
    char caminho[MAXPATHLEN];
    long long total = 0;
    int count, i;

    count = scandir(dir, &files, file_select, alphasort);
    if (count < 0)
    {
        perror("scandir");
        return 0;
    }

    for (i = 0; i < count; i++)
    {
        snprintf(caminho, sizeof(caminho), "%s/%s", dir, files[i]->d_name);

        if (lstat(caminho, &st) == -1) {
            perror(caminho);
        } else if (S_ISDIR(st.st_mode)) {
            total += soma_diretorio(caminho);   /* chamada recursiva */
        } else if (S_ISREG(st.st_mode)) {
            total += st.st_size;
        }
        free(files[i]);
    }

    free(files);
    return total;
}

int main(void)
{
    char pathname[MAXPATHLEN];

    if(getcwd(pathname, sizeof(pathname)) == NULL)
    {
        perror("getcwd");
        return 1;
    }

    printf("Diretorio: %s\n", pathname);
    printf("Tamanho total: %lld bytes\n", soma_diretorio(pathname));
    return 0;
}