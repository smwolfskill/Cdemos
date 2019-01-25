/**
 * Machine Problem: Vector
 * CS 241 - Fall 2016
 */

/* An automatically-expanding array of strings. */
#include "vector.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define INITIAL_CAPACITY 10

/*
 * Vector structure
 * Do not modify the structure
 * array: Void pointer to the beginning of an array of void pointers to
 * arbitrary data.
 * size: The number of elements in the vector. This is the number of actual
 * objects held in the vector, which is not necessarily equal to its capacity.
 * capacity: The size of the storage space currently allocated for the vector,
 * expressed in terms of elements.
 * copyt_constructor: the function callback for you to define the way you want
 * to copy elements
 * destructor:  the function callback for you to define the way you want to
 * destroy elements
 */
struct Vector {
  copy_constructor_type copy_constructor;
  destructor_type destructor;

  void **array;
  size_t size;
  size_t capacity;
};

Vector *Vector_create(copy_constructor_type copy_constructor,
                      destructor_type destructor) {
  // your code here
  Vector * newVec = (Vector*) malloc(sizeof(Vector));
  newVec->copy_constructor = copy_constructor;
  newVec->destructor = destructor;
  newVec->size = 0;
  newVec->capacity = 10;
  newVec->array = malloc(10 * sizeof(void*));
  while(newVec->size < newVec->capacity) newVec->array[newVec->size++] = NULL;
  newVec->size = 0;
  return newVec;
}

//In C there is no 'this', so have to pass in ptr to it in every function
void Vector_destroy(Vector *vector) { //I think done
  assert(vector);
  // your code here
  size_t i = 0;
  while(i < vector->capacity) {
  	if(vector->array[i]) {
  		vector->destructor(vector->array[i]); //don't call when NULL
  		//free(vector->array[i]);
  	}	
  	i++;
  }
  free(vector->array);
  free(vector);
  //don't assume nothing bad happens when call destructor(NULL) !!
}

size_t Vector_size(Vector *vector) { //I think done
  assert(vector);
  // your code here
  return vector->size;
}

size_t Vector_capacity(Vector *vector) {
  assert(vector);
  // your code here
  return vector->capacity;
}

void Vector_resize(Vector *vector, size_t new_size) {
  assert(vector);
  // your code here
  //Double or halve capacity depending on new_size, realloc, and initialize
  size_t oldCapacity = vector->capacity;
  vector->size = new_size;
  while(new_size > vector->capacity) {
  	vector->capacity *= 2;
  }
  while(4 * new_size <= vector->capacity) {
  	if((vector->capacity / 2) < 10) {
  		vector->capacity = 10; //10 capacity minimum
  		break;
  	} else vector->capacity /= 2;
  }
  if(oldCapacity != vector->capacity) { //changed capacity
  	size_t i = vector->capacity;
  	while(i < oldCapacity) { //if shrank, free old elements
  		free(vector->array[i]);
  	}
  	vector->array = realloc(vector->array, vector->capacity * sizeof(void*));
  	
  	i = oldCapacity;
  	while(i < vector->capacity) vector->array[i++] = NULL; //if grew, new entries are initialized to NULL
  }
  return;
}

void Vector_set(Vector *vector, size_t index, void *elem) {
  assert(vector);
  // your code here
  assert(index < vector->size); //make sure valid index
  //if(vector->array[index] == NULL && elem != NULL) vector->size++; //only increment if changing a NULL to a non-NULL in the array
  free(vector->array[index]);
  if(elem == NULL) vector->array[index] = NULL;
  else {
  	/*vector->array[index] = malloc(sizeof(*elem));
  	memcpy(vector->array[index], elem, sizeof(*elem));*/
  	vector->array[index] = vector->copy_constructor(elem);
  }
}

void *Vector_get(Vector *vector, size_t index) {
  assert(vector);
  // your code here
  assert(index < vector->size); //make sure valid index
  return vector->array[index]; 
}

void Vector_insert(Vector *vector, size_t index, void *elem) {
  assert(vector);
  // your code here
  if(index >= vector->size) Vector_resize(vector, index + 1);
  else Vector_resize(vector, vector->size + 1);
  //Before assigning, shift everything of greater index right 1
  size_t i = vector->capacity - 1;
  while(i > index) {
  	vector->array[i] = vector->array[i - 1];
  	i--;
  }
  //Now assign
  if(elem != NULL) { 
  	vector->array[index] = vector->copy_constructor(elem);
  } else vector->array[index] = NULL; //not assuming that the given copy ctor will return NULL if input NULL
  
}

void Vector_delete(Vector *vector, size_t index) {
  assert(vector);
  // your code here, what asserts might you want?
  assert(index < vector->capacity);
  free(vector->array[index]);
  Vector_resize(vector, vector->size - 1);
  size_t i = index;
  while(i < (vector->capacity - 1)) { //shift everything at a greater index left 1
  	vector->array[i] = vector->array[i + 1];
  	i++;
  }
  vector->array[vector->capacity - 1] = NULL;
}

void Vector_append(Vector *vector, void *elem) {
  assert(vector);
  // your code here
  Vector_resize(vector, vector->size + 1);
  if(elem != NULL) { 
  	vector->array[vector->size-1] = vector->copy_constructor(elem);
  } else vector->array[vector->size-1] = NULL; //not assuming that the given copy ctor will return NULL if input NULL
}
