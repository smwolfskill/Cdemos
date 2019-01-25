/**
 * Machine Problem: Malloc
 * CS 241 - Fall 2016
 */

#include <stdio.h>
#include <stdlib.h>

#define NUM_CYCLES 1000000/*0*/

int main() {
  int i;
  for (i = 0; i < NUM_CYCLES; i++) {
    //if(i % 1000000 == 0) printf("i = %d\n", i);
    int *ptr = malloc(sizeof(int));

    if (ptr == NULL) {
      printf("Memory failed to allocate!\n");
      return 1;
    }

    *ptr = 4;
    free(ptr);
  }

  printf("Memory was allocated, used, and freed!\n");
  return 0;
}
