/**
 * Machine Problem 0
 * CS 241 - Spring 2016
 */

#include "part2-functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * (Edit this function to print out the "Illinois" lines in
 * part2-functions.c in order.)
 */
int main() {
  // your code here
  //1:
  first_step(81);
  
  //2:
  int * sec = malloc(sizeof(int));
  *sec = 132;
  second_step(sec);
  free(sec);
  
  //3:
  int ** ds = malloc(sizeof(int *));
  *ds = malloc(sizeof(int));
  *ds[0] = 8942;
  double_step(ds);
  free(*ds);
  free(ds);
  
  //4:
  char * strange = malloc(7 * sizeof(int)); strange[0] = '\0';
  strcat(strange, "     ");
  strange[5] = (char) 15;
  strange[6] = '\0';
  for(unsigned int i = 7; i < 7 * sizeof(int); i++)
  	strange[i] = '\0'; //tired of annoying mem errors
  
  strange_step(strange);
  free(strange);
  
  //5:
  char * empty = malloc(4);
  empty[3] = 0;
  empty_step(empty);
  free(empty);
  
  //6:
  char * two = malloc(4);
  two[3] = 'u';
  two_step(two, two);
  free(two);
  
  //7:
  char * three = malloc(5 * sizeof(char *)); //not sure about this
  three_step(&three[0], &three[2], &three[4]);
  //for(int i = 0; i < 5; i++) printf("&three[%d] = %p\n", i, &three[i]);
  //printf("test: &three[2] - &three[0] = %ld\n", (&three[2] - &three[0]));
  free(three);
  
  //8: third[3] == second[2] + 8 && second[2] == first[1] + 8
  char * first = malloc(2);
  char * second = malloc(3);
  char * third = malloc(4);
  first[1] = 'a';
  second[2] = (char)((int) first[1] + 8);
  third[3] = (char)((int) second[2] + 8);
  step_step_step(first, second, third); //hopefully
  free(first);
  free(second);
  free(third);
  
  //9:
  int b = 1;
  char * a = malloc(1);
  *a = b;
  it_may_be_odd(a, b);
  free(a);
  
  //10:
  char * str = malloc(8);
  str[0] = '\0';
  strcat(str, "3,CS241");
  tok_step(str); //hopefully/maybe
  free(str);
  
  //11:
  // orange == blue && ((char *)blue)[0] == 1 && 
  //    *((int *)orange) % 3 == 0
  char * orange = malloc(sizeof(int));
  for(unsigned int i = 0; i < sizeof(int) - 1; i++) {
  	orange[i] = (char) 1;
  }
  orange[sizeof(int) - 1] = '\0';
  //orange[0] = (char) 1;
  //orange[1] = '\0';
  //printf("the_end: %c = %d  mod 3 = %d\n", orange[0], *((int*)orange), *((int*)orange) % 3);
  the_end(orange, orange); //doubt it. But it worked! :D
  free(orange);
  return 0;
}
