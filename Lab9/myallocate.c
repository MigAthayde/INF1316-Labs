#include<stdio.h>
#include<unistd.h>

void *memorypool;

void *myallocate(int n);
void initmemorypool(void);
void resetmemorypool(void);

int main(void)
{
    initmemorypool();

    printf("---  Limite inicial do heap %p -----\n", sbrk(0));

    int* v1 = (int*) myallocate(sizeof(int)*4);
    if (v1 == NULL)
    {
        printf("Erro na alocacao de momeria pro v1");
        return 1;
    }

    printf("--- Novo limite para o heap %p -----\n", sbrk(0));
    resetmemorypool();
    return 0;
}

void *myallocate( int n){
    void* ptr = sbrk(n);
    if(ptr == (void*)-1)
    {
        return NULL;
    }
    return ptr;
}
void initmemorypool(void) 
{
    memorypool = sbrk(0);
}
void resetmemorypool(void)
{
    brk(memorypool);
}