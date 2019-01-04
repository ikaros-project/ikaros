//
//  IKAROS_Malloc_Debug.h
//

#ifndef USING_MALLOC_DEBUG
#define USING_MALLOC_DEBUG

//
// USE_MALLOC_DEBUG checks all memory allocations (Currently OS X only).
//

#include <malloc/malloc.h>

const int     mem_max_blocks = 100000;    // Ikaros will exit if storage is exhausted
unsigned long mem_allocated = 0;
int           mem_block_allocated_count = 0;
int           mem_block_deleted_count = 0;
void *        mem_block[mem_max_blocks];
size_t        mem_size[mem_max_blocks];
bool          mem_block_deleted[mem_max_blocks];

void *
operator new (std::size_t size) noexcept(false)
{
    void *p=calloc(size, sizeof(char));
    if (p==0) // did calloc succeed?
        throw std::bad_alloc();
    
    mem_block[mem_block_allocated_count] = p;
    mem_size[mem_block_allocated_count] = size;
    mem_block_deleted[mem_block_allocated_count] = false;
    mem_block_allocated_count++;
    
    if(mem_block_allocated_count > mem_max_blocks)
    {
        printf("OUT OF MEMORY\n");
        exit(1);
    }
    
    return p;
}

void *
operator new [] (std::size_t size) noexcept(false)
{
    return operator new(size);
}

void
operator delete (void *p) throw()
{
    if(p == NULL) // this is ok
        return;
    
    // Look for block (backwards to allow several allocation of the same memory)
    
    for(int i=mem_block_allocated_count; i>=0; i--)
        if(mem_block[i] == p)
        {
            if(mem_block_deleted[i])
            {
                printf("Attempting to delete already deleted memory [%d]: %p\n", i, p);
                return;
            }
            
            else
            {
                mem_block[i] = NULL;
                mem_block_deleted[i] = true;
                mem_block_deleted_count++;
                mem_allocated -= malloc_size(p);
                
                free(p);
                return;
            }
        }
    
    printf("Attempting to delete memory not allocated with new: %p\n", p);
}

void
operator delete [] (void *p) throw()
{
    operator delete (p);
}

void
dump_memory()
{
    printf("Allocated Memory\n");
    printf("=======================\n");
    int cnt = 0;
    for(int i=0; i<mem_block_allocated_count; i++)
        if(!mem_block_deleted[i])
        {
            printf("%4d: %p\t[%lu->%lu]\t", i, mem_block[i], mem_size[i], malloc_size(mem_block[i]));
            for(unsigned int j=0; j<mem_size[i]; j++)
            {
                char * p = (char *)(mem_block[i]);
                char c = p[j];
                if(' ' <= c && c < 'z')
                    printf("%c", c);
                else
                    printf("#");
            }
            printf("\n");
            cnt++;
        }
    printf("No of blocks: %d\n", cnt);
}

#endif /* IKAROS_Malloc_Debug_h */
