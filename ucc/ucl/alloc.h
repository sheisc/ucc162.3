#ifndef __ALLOC_H_
#define __ALLOC_H_

struct mblock
{
	// pointer to next memory block in the heap
	struct mblock *next;
	// beginning of memory block
	char *begin;
	// currently available memory
	char *avail;
	// end of memory block
	char *end;
};

union align 
{
	double d;
	int (*f)(void);
};

typedef struct heap
{
	// pointer to last memory block in the heap
	struct mblock *last; 
	// memory block list head
	struct mblock head;
} *Heap;
// In C++,  void *  --> int *   is an error
// In C , it is OK  for both  void * --> int *  and  int * --> void *
// In other words, the type checker of C++ is stricter.

#define DO_ALLOC(p)    ((p) = HeapAllocate(CurrentHeap, sizeof *(p)))
#define ALLOC(p)   memset(DO_ALLOC(p), 0, sizeof *(p))
#define CALLOC(p)   memset(DO_ALLOC(p), 0, sizeof *(p))




#define MBLOCK_SIZE (4 * 1024)
#define HEAP(hp)    struct heap  hp = { &hp.head }

void  InitHeap(Heap hp);
void* HeapAllocate(Heap hp, int size);
void  FreeHeap(Heap hp);

#endif
