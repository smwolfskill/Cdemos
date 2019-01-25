/**
 * Machine Problem: Vector
 * CS 241 - Fall 2016
 */

#include "vector.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

void *my_copy_ctor(void *elem) {
  char *str = elem;
  assert(str);
  return strdup(str);
}
// This is the destructor function for string element.
// Use this as destructor callback in vector.
void my_destructor(void *elem) { free(elem); }

void listAll(Vector * vec) { //list all elements up to (capacity-1)
	size_t i = 0;
	size_t size = Vector_size(vec);
	printf("    Listing elements of a string Vector of size %d and capacity %d...\n", (int) size, (int) Vector_capacity(vec));
	while(i < size/*Vector_capacity(vec)*/) {
		printf("\t%d: %s\n", (int) i, Vector_get(vec, i));
		i++;
	}
	printf("    (done)\n");
}

// Test your vector here
int main() { 
	Vector * vec = Vector_create(my_copy_ctor, my_destructor);
	//1: Create and set a couple values and destroy the vector
	/*Vector_resize(vec, 5);
	Vector_set(vec, 0, "my vector");
	Vector_set(vec, 2, "line2");
	Vector_append(vec, "HI!");
	Vector_insert(vec, 5, "iNsErT!!");
	printf("List all BEFORE delete index 2:\n");
	listAll(vec);
	Vector_delete(vec, 2);
	printf("List all AFTER delete index 2:\n");
	listAll(vec);*/
	//2: Create and append 100, then shrink
	/*int i = -1;
	while(++i < 100) {
		Vector_append(vec, "a");
	}
	free(tmp);
	//printf("List all BEFORE shrink:\n");
	//listAll(vec);
	Vector_resize(vec, 41);
	printf("List all AFTER shrink:\n");
	listAll(vec);*/
	//3: Create and insert 100
	//Vector_resize(vec, 30);
	int i = -1;
	char * tmp = malloc(2);
	tmp[1] = '\0';
	while(++i < 20) {
		tmp[0] = (char) (i + (int) 'A');
		Vector_insert(vec, 11, tmp);
	}
	free(tmp);
	listAll(vec);
	//
	Vector_destroy(vec);
	return 0; 
}
