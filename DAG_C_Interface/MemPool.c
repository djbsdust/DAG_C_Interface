#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "MemPool.h"


struct MemPool pool;
struct block* mempool_allocblock(struct MemPool pool)
{
	struct block *myblock=(struct block *)malloc(sizeof(struct block));
	myblock->data=malloc(UNITSIZE);
	myblock->next=NULL;
	return myblock;
}
void InitPool()
{
    pool.EndBlock=NULL;
    pool.AllocatedMemBlock=NULL;
    pool.FreeMemBlock=NULL;
    pool.self1=NULL;
	
    pool.FreeMemBlock=mempool_allocblock(pool);
//    void *s=memset(((char *)pool.MemBlock+sizeof(struct Unit)),0, pool.UnitSize);
    pool.EndBlock=(void *)((char *)(pool.FreeMemBlock->data) +UNITSIZE);
    pool.self1=(void *)(pool.FreeMemBlock->data);
    struct  block *pCurUnit=pool.FreeMemBlock ;
    pool.FreeMemBlock	= pool.FreeMemBlock->next;	
    pCurUnit->next = pool.AllocatedMemBlock;	
    pool.AllocatedMemBlock = pCurUnit;
}
void* acemesh_myalloc_aligned_4(int datasize)
{
	 if((long long)pool.self1%4>0)
	   pool.self1=(void*)((char*)pool.self1+(4-(long long)pool.self1%4));
    if(((char*)pool.EndBlock-(char*)pool.self1) < datasize)
    {
        if(pool.FreeMemBlock == NULL)
        {  
            pool.FreeMemBlock=mempool_allocblock(pool);  
        }
        struct  block *pCurUnit=pool.FreeMemBlock ;
        pool.FreeMemBlock	= pool.FreeMemBlock->next;	
        pCurUnit->next = pool.AllocatedMemBlock;	
        pool.AllocatedMemBlock = pCurUnit;
        pool.EndBlock=(void *)((char *)(pCurUnit->data)+UNITSIZE);
        pool.self1=(void *)(pCurUnit->data);
    }
    void *self=(void *)pool.self1;
    pool.self1=(void *)((char *)pool.self1+datasize);   
    return (void *)self;
}
void* acemesh_myalloc_aligned_8(int datasize)
{
    if((long long)pool.self1%8>0)
	   pool.self1=(void*)((char*)pool.self1+(8-(long long)pool.self1%8));

    if(((char*)pool.EndBlock-(char*)pool.self1) < datasize)
    {
        if(pool.FreeMemBlock == NULL)
        {  
            pool.FreeMemBlock=mempool_allocblock(pool);  
        }
        struct  block *pCurUnit=pool.FreeMemBlock ;
        pool.FreeMemBlock	= pool.FreeMemBlock->next;	
        pCurUnit->next = pool.AllocatedMemBlock;	
        pool.AllocatedMemBlock = pCurUnit;
        pool.EndBlock=(void *)((char *)(pCurUnit->data)+UNITSIZE);
        pool.self1=(void *)(pCurUnit->data);
    }
    void *self=(void *)pool.self1;
    pool.self1=(void *)((char *)pool.self1+datasize);   
    return (void *)self;
}
void* acemesh_myalloc_aligned_16(int datasize)
{
    if((long long)pool.self1%16>0)
	   pool.self1=(void*)((char*)pool.self1+(16-(long long)pool.self1%16));
    if(((char*)pool.EndBlock-(char*)pool.self1) < datasize)
    {
        if(pool.FreeMemBlock == NULL)
        {  
            pool.FreeMemBlock=mempool_allocblock(pool);  
        }
        struct  block *pCurUnit=pool.FreeMemBlock ;
        pool.FreeMemBlock	= pool.FreeMemBlock->next;	
        pCurUnit->next = pool.AllocatedMemBlock;	
        pool.AllocatedMemBlock = pCurUnit;
        pool.EndBlock=(void *)((char *)(pCurUnit->data)+UNITSIZE);
        pool.self1=(void *)(pCurUnit->data);
    }
    void *self=(void *)pool.self1;
    pool.self1=(void *)((char *)pool.self1+datasize);   
    return (void *)self;
}
void* acemesh_myalloc_aligned_32(int datasize)
{
    if((long long)pool.self1%32>0)
	   pool.self1=(void*)((char*)pool.self1+(32-(long long)pool.self1%32));
    if(((char*)pool.EndBlock-(char*)pool.self1) < datasize)
    {
        if(pool.FreeMemBlock == NULL)
        {  
            pool.FreeMemBlock=mempool_allocblock(pool);  
        }
        struct  block *pCurUnit=pool.FreeMemBlock ;
        pool.FreeMemBlock	= pool.FreeMemBlock->next;	
        pCurUnit->next = pool.AllocatedMemBlock;	
        pool.AllocatedMemBlock = pCurUnit;
        pool.EndBlock=(void *)((char *)(pCurUnit->data)+UNITSIZE);
        pool.self1=(void *)(pCurUnit->data);
    }
    void *self=(void *)pool.self1;
    pool.self1=(void *)((char *)pool.self1+datasize);   
    return (void *)self;
}
void ReInitial()
{
    while(pool.AllocatedMemBlock != NULL)
    {
        struct block *pCurUnit =pool.AllocatedMemBlock; 
        //void* s=memset(((char *)pool.AllocatedMemBlock+sizeof(struct Unit)),0, pool.UnitSize);
        pool.AllocatedMemBlock = pool.AllocatedMemBlock->next;
        pCurUnit->next = pool.FreeMemBlock;
        pool.FreeMemBlock = pCurUnit;		
    }
    pool.self1=(void *)(pool.FreeMemBlock->data);
    pool.EndBlock=(void *)((char *)(pool.FreeMemBlock->data)+UNITSIZE);
    struct  block *pCurUnit=pool.FreeMemBlock ;
    pool.FreeMemBlock	= pool.FreeMemBlock->next;	
    pCurUnit->next = pool.AllocatedMemBlock;	
    pool.AllocatedMemBlock = pCurUnit;
}

void FreePool()
{
    while(pool.FreeMemBlock != NULL)
    {
        struct  block *pCurUnit=pool.FreeMemBlock;
        pool.FreeMemBlock=pool.FreeMemBlock->next;
        free(pCurUnit->data);
        free(pCurUnit);
    }
}

