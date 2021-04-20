#ifndef __MEMPOOL_H__
#define __MEMPOOL_H__

#define UNITSIZE 1024*1024*20
typedef struct block
{
   void *data;
   struct block  *next;
}block;
typedef struct MemPool
{
    void*   EndBlock;	
    void*   self1;
    struct block*	AllocatedMemBlock;	
    struct block*	FreeMemBlock;		
}MemPool;

    struct block* mempool_allocblock(struct MemPool pool);
	
    void InitPool();
    void* acemesh_myalloc_aligned_4(int datasize);
	void* acemesh_myalloc_aligned_8(int datasize);
	void* acemesh_myalloc_aligned_16(int datasize);
	void* acemesh_myalloc_aligned_32(int datasize);
    void ReInitial();
    void FreePool();

#endif
