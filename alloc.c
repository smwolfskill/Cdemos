/**
 * Machine Problem: Malloc
 * CS 241 - Fall 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MEMNODE_SIZE (sizeof(memNode_t) % sizeof(void*) == 0 ? (sizeof(memNode_t)) : (sizeof(memNode_t) + sizeof(void*) - (sizeof(memNode_t) % sizeof(void*)) ) )
#define FREENODE_SIZE (sizeof(freeNode_t) % sizeof(void*) == 0 ? (sizeof(freeNode_t)) : (sizeof(freeNode_t) + sizeof(void*) - (sizeof(freeNode_t) % sizeof(void*)) ) )
#define MIN_SIZE (FREENODE_SIZE)
#define T (0)
//MIN_SIZE is min. data size allowed for mem (not including MEMNODE_SIZE)

//TODO: for optimazation, if want, make global and set it to MEMNODE_SIZE
//Rounds sizeToRound up to a multiple of void*.
int roundUpToVoidPtr(size_t sizeToRound) {
	if(sizeToRound % sizeof(void*) == 0) return sizeToRound;
	return sizeToRound + sizeof(void*) - (sizeToRound % sizeof(void*));
}

//My struct for keeping track of things in memory
typedef struct memNode {
	short used; //0 is free; 1 is in-use
	size_t size; //size in bytes of JUST the data
	size_t usedBytes; //bytes that are currently in use
	struct memNode * prev;
	struct memNode * next;
} memNode_t;

typedef struct memList {
	memNode_t * head; //first memNode we create
	memNode_t * tail; //last memNode created
} memList_t;

memList_t * memList_init() {
	memList_t * memList = (memList_t*) sbrk(sizeof(memList_t));
	memList->head = NULL;
	memList->tail = NULL;
	return memList;
}

int memList_destroy(memList_t * memList) {
	return brk(/*&*/memList->head);
}

typedef struct freeNode {
	struct memNode * mem;
	struct freeNode * prev;
	struct freeNode * next;
} freeNode_t;

typedef struct freeList {
	freeNode_t * head; //first free'd memNode
	freeNode_t * tail; //last free'd memNode
} freeList_t;

freeList_t * freeList_init() {
	freeList_t * freeList = (freeList_t*) sbrk(sizeof(freeList_t));
	freeList->head = NULL;
	freeList->tail = NULL;
	return freeList;
}

//memList_t * memList = NULL;
//freeList_t * freeList = NULL;
static memNode_t * head = NULL; //first memNode we create
static memNode_t * tail = NULL; //last memNode created
static freeNode_t * freeHead = NULL; //first free'd memNode
static freeNode_t * freeTail = NULL; //last free'd memNode
memNode_t * getHead() { return head; }
freeNode_t * getFreeHead() { return freeHead; }

void memNode_setData(memNode_t * mem, freeNode_t * newData) {
	freeNode_t ** data = (freeNode_t**)(((void*) mem) + MEMNODE_SIZE);
	*data = newData; //set the free'd toAdd's data to point to freedNode for O(1) access. Think this works!
}

//freedNode->used == 0; must be a list of only freed!
// Returns 0 if sbrk failed, else 1
short freeNode_push(memNode_t * toAdd) {
	if(T) printf("freeNode_push called w/ toAdd = %p\n", toAdd);
	//freeNode_t * freedNode = (freeNode_t*) sbrk(sizeof(freeNode_t));
	//if((void*) freedNode == (void*) -1) return 0; //sbrk failed
	freeNode_t * freedNode = (freeNode_t*) malloc(sizeof(freeNode_t)); //use our malloc
	if(!freedNode) { write(2, "freeNode_push: malloc failed!\n", 30); return 0; }
	//if((void*) freedNode != ((void*)toAdd) + MEMNODE_SIZE)
	freedNode->mem = toAdd;
	/*else { //malloc gave us addr. where toAdd is b/c it's free: so offset
		//freedNode->mem = (memNode_t*)(((void*) toAdd) + MEMNODE_SIZE + FREENODE_SIZE);
	}*/
	if(!freeHead) freeHead = freedNode; //no head yet (now there is)
	if(freeTail) freeTail->next = freedNode;
	freedNode->prev = freeTail;
	freeTail = freedNode;
	memNode_setData(toAdd, freedNode);
	//printf("freeNode_push: mem = %p; data = %p; *data = %p\n", toAdd, data, *data);
	if(T) printf("freeNode_push:\tdone!\n");
	return 1;
}

//for when want to use the node again; removing it from free'd list
void freeNode_remove(freeNode_t * usedNode) {
	if(!usedNode) { write(2, "freeNode_remove: usedNode was null!\n", 36);}
	if(usedNode->prev) usedNode->prev->next = usedNode->next;
	else freeHead = usedNode->next; //usedNode was freeHead
	if(usedNode->next) usedNode->next->prev = usedNode->prev;
	else freeTail = usedNode->prev; //usedNode was freeTail
	free(usedNode); //use our free
}

//Searches entire list to find the freeNode whose mem == toFind (address),
// and then removes it from the list. Only needed when joining (and is slow)
/*void freeNode_findAndRemove(memNode_t * toFind) {
	freeNode_t * cur = freeHead;
	while(cur) {
		if(cur->mem == toFind) break;
		cur = cur->next;
	}
	if(cur) { //shouldn't be null, but just in case...
		freeNode_remove(cur);
	} else write(0, "freeNode_findAndRemove: not found!\n", 15);
}*/

freeNode_t * freeNode_findByAssociation(memNode_t * mem) {
	freeNode_t ** toFind = (freeNode_t**) (((void*) mem) + MEMNODE_SIZE);
	return *toFind;
}

//Assumes mem's data = addr. of freeNode to remove. This should happen
//automatically when mem is free'd. In effect, this fxn. removes the 
//freeNode associated with mem. O(1) instead of O(n) search earlier.
void freeNode_removeByAssociation(memNode_t * mem) {
	freeNode_t * toRemove = freeNode_findByAssociation(mem);
	if(T) printf("freeNode_removeByAssociation: mem = %p; removing freeNode @ %p\n", mem, toRemove);
	freeNode_remove(toRemove);
}

void memNode_removeTail() { //remove and deallocate tail from memList
	if(tail->prev) { //List size >= 2
  		tail = tail->prev;
  		brk(tail->next);
  		tail->next = NULL;
  	} else { //we free'd the ONLY node in the list
  		brk(tail);
  		head = NULL;
  		tail = NULL;
  	}
}

//Joins together two memNodes. a->next should = b,
// and both should be !used, unless a is used and are expanding a
void memNode_join(memNode_t * a, memNode_t * b) {
	a->size += MEMNODE_SIZE + b->size;
	a->next = b->next;
	if(!a->next) tail = a;
	else a->next->prev = a;
}

//Splits (start) into 2 memNodes and returns the second one (newly created).
// assumes newSize <= start->size + MEMNODE_SIZE (so room for bookkeeping)
//To access the new node, refer to (start)->next
memNode_t * memNode_split(memNode_t * start, size_t newSize) {
	if(T) printf("memNode_split called...\n");
	short joined = 0;
	memNode_t * next = (memNode_t*) (((char*) start) + (newSize + MEMNODE_SIZE));
	next->used = 0;
	next->size = start->size - newSize - MEMNODE_SIZE; //should be at least 4 if this is called correctly
	next->usedBytes = 0;
	next->next = start->next;
	next->prev = start;
	if(!start->next) tail = next;
	start->size = newSize;
	start->next = next;
	if(next->next && !next->next->used) { //possibile join next w/ next->next
		joined = 1;
		if(T) printf("memNode_split: join mem->next, mem->next->next\n");
		//freeNode_findAndRemove(next->next);
		freeNode_t * freeNode = freeNode_findByAssociation(next->next);
		memNode_setData(next, freeNode);
		freeNode->mem = next;
		memNode_join(next, next->next);
	}
	if(!joined) freeNode_push(next);
	if(T) printf("memNode_split: done!\n");
	return next;
}

memNode_t * memNode_create(size_t size) {
	static short firstTime = 1;
	if(firstTime) { head = NULL; tail = NULL; freeHead = NULL; freeTail = NULL; firstTime = 0; }
	
	size_t usedBytes = size;
	if(size < MIN_SIZE) { size = MIN_SIZE; }
	memNode_t * mem;
	freeNode_t * cur = freeHead;
	short foundNode = 0; //if found previously used but now free node
	//First fit algorithm
	while(cur) {
		if(!cur->mem->used && cur->mem->size >= size) {
			foundNode = 1;
			break;
		}
		cur = cur->next;
	}
	if(!foundNode) { //need to make a new memNode_t
		//Use sbrk w/ enough space for memNode_t AND of course the requested bytes (size)
		mem = (memNode_t *) sbrk(MEMNODE_SIZE + size);
		if((void*) mem == (void*) -1) return NULL; //sbrk failed
		mem->next = NULL;
		mem->prev = tail;
		if(!head) head = mem; //only for 1st use
		if(tail) tail->next = mem;
		tail = mem;
		mem->size = size;
	} else { //can use existing free'd node
		mem = cur->mem;
		size_t roundedSize = roundUpToVoidPtr(size);
		if(mem->size > roundedSize + MEMNODE_SIZE + MIN_SIZE + MEMNODE_SIZE + MIN_SIZE) { //no point creating new node if only enough space for bookkeeping (incl. freeNode) and small #bytes
			memNode_t * next = memNode_split(mem, roundedSize);
			cur->mem = next;
			memNode_setData(next, cur);
			//PROBLEM: puts freeNode where the split is, so freeNode->mem points to wrong thing
			mem->size = roundedSize;
		} else //else, don't split. usedBytes < size, so a little extra space
			freeNode_remove(cur);
	}
	mem->used = 1;
	mem->usedBytes = usedBytes; //original size
	return mem;
}

/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements. (me: in bytes)
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) { //think done. untested
  // implement calloc!
  memNode_t * mem = memNode_create(num * size);
  if(mem) { //if create didn't fail, initialize all bits to 0. Do this byte-by-byte
  	size_t i = MEMNODE_SIZE;
  	while(i < MEMNODE_SIZE + /*num*size*/mem->size) {
  		((char*)mem)[i] = (char) 0;
  		i++;
  	}
  }
  return ((void*) mem) + MEMNODE_SIZE /*/ sizeof(void*)*/;
}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
  // implement malloc!
  //Create memNode_t BEFORE creating the data; memNode_create will effectively allocate everything!
  memNode_t * mem = memNode_create(size);
  //NO: void * mem = sbrk(size);
  return ((void*) mem) + MEMNODE_SIZE /*/ sizeof(void*)*/;
  //Since user will assign to this, return addr. of the data, NOT memNode
}

/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
  if(!ptr) return;
  short joined = 0;
  memNode_t * mem = (memNode_t*) (ptr-MEMNODE_SIZE);
  if(T) printf("free called on mem = %p...\n", mem);
  mem->used = 0;
  //check if next and prev are !used, and join if so
  if(mem->next && !mem->next->used) { //because of the freeNode search, not efficient. Only for saving space.
  	//freeNode_findAndRemove(mem->next); //See note below
  	joined = 1;
  	if(T) printf("free: join mem, mem->next\n");
  	freeNode_t * freeNode = freeNode_findByAssociation(mem->next);
  	freeNode->mem = mem;
  	memNode_setData(mem, freeNode);
  	memNode_join(mem, mem->next);
  	if(T) printf("free: \tdone joining mem, mem->next\n");
  }
  if(mem->prev && !mem->prev->used) { //join w/ PREV
  	if(T) printf("free: join mem->prev, mem\n");
  	//freeNode_findAndRemove(mem->prev);
  	if(joined) freeNode_removeByAssociation(mem); //don't need b/c never had a chance to give mem an associated freeNode in the first place. Just use mem-prev's freeNode
  	joined = 1;
  	memNode_t * prev = mem->prev;
  	memNode_join(mem->prev, mem);
  	mem = prev;
  	if(T) printf("free: \tdone joining mem->prev, mem\n");
  }
  if(tail == mem) { //if it's last node, don't deal with free'd list; just de-allocate
  	if(T) printf("free: removing tail (last node)...\n");
  	memNode_removeTail(); //PROBLEM: This is slowwwwww.
  } else if(!joined) {
   	//if joined, don't need to create new freeNode b/c can just use one already created for the other node we joined with
  	freeNode_push(mem);
  }
  //printf("free\t%p: done\n", mem);
  //PROBLEM: if join, old freeNode from mem->next is still in free list!!
  //could just have old freeNodes far back in the list s.t. they aren't
  //checked, BUT they are still checked if there's a really large request..
  //but even if checked, data will still be valid, AND would never be chosen
  //NO: situation: join'd block becomes used. Then old freeNode could be checked AND have garbage values :(
  //Only sol'n is to search the list to find mem->next, and delete the freeNode. :(
  
  //NEW SOL'N: set a memNode's data to point to the freeNode that refers to it! MAKE SURE every memNode's size is AT LEAST MEMNODE_SIZE + sizeof(freeNode_t*). This way we have O(1) access instead of O(n)!
}

/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) { //think done. untested
  // implement realloc!
  if(!ptr) return malloc(size);
  if(size == 0) { free(ptr); return NULL; } //free
  memNode_t * mem = (memNode_t*) (ptr - MEMNODE_SIZE);
  if(size < MIN_SIZE) size = MIN_SIZE;
  if(size == mem->size) return ptr; //nothing to be done
  if(size > mem->size) {
  	//check if next is free AND has enough space
  	//TODO: if implement prev, check that too!
  	if(mem->next && !mem->next->used && (mem->next->size + mem->size + MEMNODE_SIZE) >= size) {
  		//freeNode_findAndRemove(mem->next);
  		freeNode_removeByAssociation(mem->next);
  		memNode_join(mem, mem->next);
  		mem->usedBytes = size;
  		if(mem->size > size + MEMNODE_SIZE + MIN_SIZE) //create new node if enough free space for bookkeeping and var (4-bytes)
  			memNode_split(mem, size);
  		return ptr;
  	} else if(tail == mem) {//lucky; can just sbrk however many more bytes we need
  		if( ((void*) -1) == sbrk(size - mem->size)) {
  			write(2, "realloc: sbrk failed!\n", 22); return NULL; }
  		mem->used = 1;
  		mem->size = size;
  		mem->usedBytes = size;
  		return ptr;
  	} else { //unfortunate; need to copy data
  		//TODO: FIX SEGFAULT!!!
  		memNode_t * newMem = memNode_create(size);
  		if(newMem) {
  			memcpy( ((void*)newMem) + MEMNODE_SIZE, ((void*)mem) + MEMNODE_SIZE, mem->size);
  			/*size_t i = MEMNODE_SIZE;
  			while(i < MEMNODE_SIZE + mem->size) {
  				((char*)newMem)[i] = ((char*)mem)[i]; //copy data
  				i++;
  			}*/
  			//fprintf(stdout, "realloc: attempting free on ptr = %p...\n", ptr);
  			free(ptr);
  		} else return NULL; //error
  		return ((void*) newMem) + MEMNODE_SIZE;
  	}
  } else { //size < mem->size
  	if(mem->size > size + MEMNODE_SIZE + MIN_SIZE) //don't split if not enough room for bookkeeping and var (4-byte?)
  		memNode_split(mem, size);
  	else mem->usedBytes = size;
  	return ptr;
  }
}
