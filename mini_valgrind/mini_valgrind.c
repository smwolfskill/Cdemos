/**
 * Mini Valgrind Lab
 * CS 241 - Fall 2016
 */

#include "mini_valgrind.h"
#include "print.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef malloc
#undef realloc
#undef free

#define mdModVoidPtr (sizeof(meta_data) % sizeof(void*))
#define SIZEOFMETA_DATA (sizeof(meta_data) + (mdModVoidPtr != 0 ? sizeof(void*) - mdModVoidPtr : 0))

/*
 * Replace normal malloc, this malloc will also create meta data info
 * and insert it to the head of the list.
 * You have to malloc enough size to hold both the size of your allocated
 * and the meta_data structure.
 * In this function, you should only call malloc only once.
 *
 * @param size
 *	Size of the memory block, in bytes.
 * @param file
 *	Name of the file that uses mini_valgrind to detect memory leak.
 * @param line
 *	Line in the file that causes memory leak
 *
 * @return
 *	On success, return a pointer to the memory block allocated by
 *	the function. Note that this pointer should return the actual
 *	address the pointer starts and not the meta_data.
 *
 *	If the function fails to allocate the requested block of memory,
 *	return a null pointer.
 */
void *mini_malloc(size_t size, const char *file, size_t line) {
  // your code here
	/*Make sure reserve a multiple of (sizeof(void*)) space for meta_data so that we
	  can access it easily via pointer arithmetic */
	//size_t mdModVoidPtr = sizeof(meta_data) % sizeof(void*);
	//size_t mdSize = sizeof(meta_data) + (mdModVoidPtr != 0 ? sizeof(void*) - mdModVoidPtr : 0); //round up to multiple of sizeof(void*), but NOT if it's already a multiple
	//printf("sizeof(meta_data) = %lu; sizeof(void*) = %lu; CONSTANT = %lu\n", sizeof(meta_data), sizeof(void*), SIZEOFMETA_DATA);
	void * newMem = malloc(SIZEOFMETA_DATA + size);
	//printf("\tmini_malloc(): allocated meta_data at addr. %p\n", newMem);
	if(newMem != NULL) {
		meta_data * newMd = (meta_data*) newMem;
		insert_meta_data(newMd, size, file, line);
	} else return NULL;
  	return &newMem[SIZEOFMETA_DATA / sizeof(void*)]; //to counteract pointer arithmetic of multiplying constants by sizeof(ptr type)
}

/*
 * Replace normal realloc, this realloc will also first check whether the
 * pointer that passed in has memory. If it has memory then resize the memory
 * to it. Or if the pointer doesn't have any memory, then call malloc to
 * provide memory.
 * For total usage calculation, if the new size is larger than the old size,
 * the total usage should increase the difference between the old size and the
 * new size. If the new size is smeller or equal to the old size, the total
 * usage should remain the same.
 * You have to realloc enough size to hold both the size of your allocated
 * and the meta_data structure.
 * In this function, you should only call malloc only once.
 *
 * @param ptr
 *      The pointer require realloc
 * @param size
 *	Size of the memory block, in bytes.
 * @param file
 *	Name of the file that uses mini_valgrind to detect memory leak.
 * @param line
 *	Line in the file that causes memory leak
 *
 * @return
 *	On success, return a pointer to the memory block allocated by
 *	the function. Note that this pointer should return the ACTUAL
 *	ADDRESS THE POINTER STARTS and not the meta_data.
 *
 *	If the function fails to allocate the requested block of memory,
 *	return a null pointer.
 */
 
 //DON't NEED TO DO
void *mini_realloc(void *ptr, size_t size, const char *file, size_t line) {
  // Don't need to free if shrink for realloc
	if(!ptr) return mini_malloc(size, file, line);
	size_t oldSize = ((meta_data*)ptr)->size; //had to do this to not get compiler error, but it looks weird...not sure if works...
	ptr = realloc(ptr, SIZEOFMETA_DATA + size);
	total_usage += (size - oldSize);
	if(ptr != NULL) ((meta_data*)ptr)->size = size;
  return ptr;
}

/*
 * Replace normal free, this free will also delete the node in the list.
 *
 * @param ptr
 *	Pointer to a memory block previously allocated. If a null pointer is
 *	passed, no action occurs.
 */
void mini_free(void *ptr) {
  // your code here
  if(ptr == NULL) return;
  //assume ptr is to the start of the actual mem., not meta_data
  remove_meta_data(&ptr[-1 * (SIZEOFMETA_DATA / sizeof(void*))]);
}

/*
 * Helper function to insert the malloc ptr node to the list.
 * Accumulate total_usage here.
 *
 * @param md
 * 	Pointer to the meta_data
 * @param size
 *	Size of the memory block, in bytes.
 * @param file
 *	Name of the file that uses mini_valgrind to detect memory leak.
 * @param line
 *	Line in the file that causes memory leak
 */
void insert_meta_data(meta_data *md, size_t size, const char *file,
                      size_t line) {
  /* set value for malloc_info*/
	total_usage += size;
	md->size = size;
	md->line_num = line;
	size_t i = 0;
	while(i < MAX_FILENAME_LENGTH && file[i]) {
		md->file_name[i] = file[i];
		i++;
	}
	if(i == MAX_FILENAME_LENGTH) md->file_name[i - 1] = '\0';
	//strcpy(temp, file); //FILENAME stored wrong!
	//printf("\tinsert_md(): file_name = %s\n\tassigned: %s\n", file, temp/*md->file_name*/);
	md->next = head; //have soon-to-be new head point to current head as its next
	head = md;
}

/*
 * Helper function to remove the free ptr node from the list.
 * Add to total_free here.
 *
 * @param ptr
 *	Pointer to a memory block previously allocated.
 */
void remove_meta_data(void *ptr) { //think done
  /* check if ptr is in the list and delete it from list */
  //assume ptr is to the start of meta_data
  //1. Check if it's an address in the list
  //printf("\trm_meta_data(): & = %p; & + SIZEOFMETA_DATA = %p\n", ptr, &ptr[SIZEOFMETA_DATA / sizeof(void*)]);
  meta_data * cur = head;
  meta_data * prev = NULL; //meta_data whose next points to ptr
  short validAddress = 0;
  while(cur) {
  	//printf("\tcur = %p\n", cur);
  	if(cur == ptr) { validAddress = 1; break; }
  	prev = cur;
  	cur = cur->next;
  }
  if(!validAddress) { bad_frees++; return; }
  //2. If got here, was valid, so adjust the LL and free ptr.
  meta_data * this = (meta_data*) ptr;
  total_free += this->size;
  if(prev == NULL) { //'this' is head
  	head = this->next;
  } else { //regular node in LL: has prev and next
  	prev->next = this->next;
  }
  free(ptr);
}

/*
 * Deletes all nodes from the list. Called when the program exits so make sure
 * to not count these blocks as freed.
 */
void destroy() { //I think done
  // your code here
	meta_data * cur = head;
	meta_data * next;
	while(cur) {
		next = cur->next;
		free(cur);
		cur = next;
	}
	head = NULL;
}

/*
 * Print mini_valgrind leak report.
 */
void print_report() {
    print_leak_info(head, total_usage, total_free, bad_frees);
}