/**
 * Machine Problem: Vector
 * CS 241 - Fall 2016
 */
#include "document.h"
#include "vector.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

struct _document {
  Vector *vector;
};

// This is the constructor function for string element.
// Use this as copy_constructor callback in vector.
void *my_copy_ctor(void *elem) {
  char *str = elem;
  assert(str);
  return strdup(str);
}

// This is the destructor function for string element.
// Use this as destructor callback in vector.
void my_destructor(void *elem) { free(elem); }

Document * Document_create() {
  Document * document = (Document *)malloc(sizeof(Document));

  // if size isn't zero, the interface to vector changed and we might have NULLs
  // in the vector
  // you code here!
  document->vector = Vector_create(my_copy_ctor, my_destructor);
  return document;
}

void Document_write_to_file(Document *document, const char *filename) { //I think done
  assert(document);
  assert(filename);
  // see the comment in the header file for a description of how to do this!
  // your code here!
  FILE * file = fopen(filename, "w+");
  size_t i = 0;
  while(i < Vector_size(document->vector)) {
  	char * current = (char*) Vector_get(document->vector, i);
  	if(current) {
  		fprintf(file, "%s\n", current);
  	} else fprintf(file, "\n"); //treat NULL as additional newline!
  	i++;
  }
  fclose(file);
}

Document *Document_create_from_file(const char *filename) {
  assert(filename);
  // this function will read a file which is created by Document_write_to_file
  // your code here!
  	Document * loaded = Document_create();
  	FILE * file = fopen(filename, "r");
  	if(file) { 
  		char * current = NULL;
		size_t capacity = 0;
		ssize_t result = getline(&current, &capacity, file);
	
		size_t i = 0;
  		while(result > 0) {
  			Vector_resize(loaded->vector, Vector_size(loaded->vector) + 1);
  			if(current[result-1]=='\n') current[result-1] = '\0'; //replace terminating ‘\n’ with ‘\0’ which is much more useful
 	 		if(strcmp(current, "\n")) {
 	 			//printf("\tReading line %d:%s\n", (int) (i+1), current);
  				Vector_set(loaded->vector, i, current); 
  			} else Vector_set(loaded->vector, i, NULL); //treat sole newline as additional NULL!
  			i++;
 	 		result = getline(&current, &capacity, file);
  		}
  		fclose(file);
  		free(current);
  	}
  	return loaded;
}

void Document_destroy(Document *document) {
  assert(document);
  Vector_destroy(document->vector);
  free(document);
}

size_t Document_size(Document *document) {
  assert(document);
  return Vector_size(document->vector);
}

void Document_set_line(Document *document, size_t line_number,
                       const char *str) {
  assert(document);
  assert(str);
  assert(line_number > 0);
  size_t index = line_number - 1;
  Vector_set(document->vector, index, (void *)str);
}

const char *Document_get_line(Document *document, size_t line_number) {
  assert(document);
  assert(line_number > 0);
  size_t index = line_number - 1;
  return (const char *)Vector_get(document->vector, index);
}

void Document_insert_line(Document *document, size_t line_number,
                          const char *str) {
  assert(document);
  assert(str);
  assert(line_number > 0);
  size_t index = line_number - 1;
  Vector_insert(document->vector, index, (void *)str);
}

void Document_delete_line(Document *document, size_t line_number) {
  assert(document);
  assert(line_number > 0);
  size_t index = line_number - 1;
  Vector_delete(document->vector, index);
}
