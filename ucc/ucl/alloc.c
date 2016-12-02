#include "ucl.h"

/**
 * Memory Management Module
 * 
 * UCC uses a heap-based memory management strategy. The memory is
 * allocated from a heap. When freeing a heap, the memory in the heap
 * will be recycled into a free block list.
 * 
 * A heap is made of a list of memory blocks. Each memory block
 * is a chunk of memory. 
 */

// free block list
static struct mblock *FreeBlocks;

/**
 * Initialize a memory heap.
 */ 
void InitHeap(Heap hp)
{
	hp->head.next = NULL;
	hp->head.begin = hp->head.end = hp->head.avail = NULL;
	hp->last = &hp->head;	
}

/**
 * This function allocates size bytes from a heap and returns
 * a pointer to the allocated memory.
 */
void* HeapAllocate(Heap hp, int size)
{
	struct mblock *blk = NULL;

	// the returned pointer must be suitably aligned to hold values of any type
	size = ALIGN(size, sizeof(union align));

	blk = hp->last;
	
	/// if the last memory block can't satisfy the request, find a big enough memory
	/// block from the free block list, if there is no such memory block, allocate 
	/// a new memory block
	while (size > blk->end - blk->avail)
	{
		// get the first block from FreeBlocks, added into the Heap
		if ((blk->next = FreeBlocks) != NULL)
		{
			FreeBlocks = FreeBlocks->next;
			blk = blk->next;
		}
		else
		{	// If the FreeBlocks is empty now, try to malloc more memory.
			int m = size + MBLOCK_SIZE + sizeof(struct mblock);

			blk->next = (struct mblock *)malloc(m);
			blk = blk->next;
			if (blk == NULL)
			{
				Fatal("Memory exhausted");
			}
			blk->end = (char *)blk + m;
		}
		// We have gotten a block from FreeList or allocated a new one now.
		// It must be the last block in the Heap. Initialize it here.
		// block->end was initialized when we allocated it.
		blk->avail = blk->begin = (char *)(blk + 1);
		blk->next = NULL;
		hp->last = blk;
	}
	// We are sure that there is enough space.
	blk->avail += size;

	return blk->avail - size;
}

/**
 * Recycle a heap's all memory blocks into free block list
 */
void FreeHeap(Heap hp)
{
	hp->last->next = FreeBlocks;
	FreeBlocks = hp->head.next;
	InitHeap(hp);
}

