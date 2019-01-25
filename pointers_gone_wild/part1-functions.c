/**
 * Machine Problem 0
 * CS 241 - Spring 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Assign the sum of a^2 and b^2 to c and print the results. If a = 3,
 * b = 4, then you should print "3^2 + 4^2 = 5^2" on its own line.
 *
 * @param a
 *     Input parameter a.
 *
 * @param b
 *     Input parameter b.
 */
void one(const int a, const int b) { //I think done
  int c = (a * a) + (b * b);
  printf("%d^2 + %d^2 = ", a, b);
  /*if(a == 3 && b == 4) printf("5^2\n");
  else*/ printf("%d\n", c);
  //cout << a << "^2 + " << b << "^2 = " << ((a == 3 && b == 4) ? "5^2" : c) << endl;
}

/**
 * Checks to see if the input parameter is a passing grade and prints out
 * if the grade is passing using the provided format specifiers. Note that
 * 'grade' is a char pointer pointing to the beginning of a C string. A grade
 * is considered passing if the numeric interpretation is above a 70. Suppose
 * that 'grade' pointed to the string "73.58", then 'grade' would be passing in
 * this example. If 'grade' pointed to "it's over 9000!!!!", then that is still
 * not
 * a passing grade. Hint: man strtof()
 *
 * @param grade
 *     The grade to check.
 */
void two(const char *grade) { //Works 100%
	float theGrade = strtof(grade, 0);
	printf("%f ", theGrade);
  if (theGrade <= 70)
    printf("not ");
  printf("passed!\n");
}

/**
 * Have the integer pointer p point to the integer x which is on the stack.
 * Then print the value that 'p' points to and format it as an integer
 */
void three() { //Works 100%
  int x = 4;
  int *p = &x;

  printf("The value of p is: %d\n", *p);
}

/**
 * Prints out a specific message based on if the number is
 * between (0, 1) (EXCLUSIVE for both).
 *
 * @param value
 *     Value to test.
 */
void four(const float value) { //I think works
  if (value > 0 && value < 1)
    printf("The value is between zero and one.\n");
  else
    printf("The value is not between zero and one.\n");
}

/**
 * Prints "x and y are equal." iff the values x and y point to are equal
 * where equality is defined by integer equality.
 * Else print "x and y are different".
 *
 * @param x
 *     First input parameter.
 *
 * @param y
 *     Second input parameter.
 */
void five(const int *x, const int *y) { //Works 100%
  if (*x == *y)
    printf("x and y are equal.\n");
  else
    printf("x and y are different.\n");
}

/**
 * Returns a pointer to a float that points to memory on the heap,
 * which contains a copy of the value that the integer pointer 'x' is pointing
 * to.
 *
 * @param x
 *     Input parameter, whose value will be returned as a (float *).
 *
 * @returns
 *     A new pointer, allocated on the heap and not freed, that
 *     contains the value of the input parameter.
 */
float *six(const int *x) { //works 100%
  float *p = malloc(sizeof(float));
  *p = (float) *x;
  return p;
}

/**
 * Takes a char pointer 'a' and checks to see if the first char
 * that it points to is alphabetic (upper or lower case).
 *
 * @param a
 *     Input parameter a, which is a char*
 *
 */
void seven(const char *a) { //I think works
	//printf("'A' = %d; 'a' = %d; 'Z' = %d; 'z' = %d\n", 'A', 'a', 'Z', 'z'); //turns out (int) 'Z' = 90 but (int) 'a' = 97
  if ((*a >= 'A' && *a <= 'Z') || (*a >= 'a' && *a <= 'z'))
    printf("a is a letter.\n");
  else
    printf("a is not a letter.\n");
}

/**
 * Allocates memory on the heap large enough to hold the C-string "Hello",
 * assigns the string character by character, prints out the full string "Hello"
 * on its own line, then frees the memory used on the heap.
 */
void eight() { //Works 100%
  char *s = malloc(6);

  s[0] = 'H';
  s[1] = 'e';
  s[2] = 'l';
  s[3] = 'l';
  s[4] = 'o';
  s[5] = '\0';
  printf("%s\n", s);

  free(s);
}

/**
 * Creates a float pointer 'p' that points to memory on the heap and
 * writes the numeric value 12.5 into it, prints out the float representation
 * of the value that p points to, and finally frees p.
 */
void nine() { //works 100%
  float *p = malloc(sizeof(float));
  *p = 12.5f;

  printf("The value of p is: %f\n", *p);
  free(p);
}

/**
 * Zeros out the value that x points to.
 *
 * @param x
 *     Pointer to reset to 0.
 */
void ten(int *x) { *x = 0; }  //works 100%. "x=0" sets the ptr to NULL! Not what we want!

/**
 * Concatenates "Hello " and the parameter str, which is guaranteed to be a
 * valid c string, and
 * prints the concatenated string.
 */
void eleven(const char *str) { //works 100%
	//Need to allocate more memory first
  char *s = malloc(7 + strlen(str)); //7 b/c len("Hello ")=6 and +1 for '\0'
  //s = "Hello "; //not allowed b/c it changes address of s to data
  s[0] = 'H'; s[1] = 'e'; s[2] = 'l'; s[3] = 'l'; s[4] = 'o'; s[5] = ' ';
  s[6] = '\0';
  strcat(s, str);
  printf("%s\n", s);
  free(s);
}

/**
 * Creates an array of values containing the values {0.0, 0.1, ..., 0.9}.
 */
void twelve() { //works 100%
	int i;
	float n = 10.0f;
  float *values = malloc(sizeof(float) * n);
  for (i = 0; i < n; i++)
    values[i] = ((float) i) / n;

  for (i = 0; i < n; i++)
    printf("%f ", values[i]);
  printf("\n");
  free(values);
}

/**
 * Creates a 2D array of values and prints out the values on the diagonal.
 */
void thirteen(int a) { //works 100%
  int **values;

  int i, j;
  values = malloc(10 * sizeof(int *));
  for (i = 0; i < 10; i++) {
  	values[i] = malloc(10 * sizeof(int));
    for (j = 0; j < 10; j++)
      values[i][j] = i * j * a;
  }

  for (i = 0; i < 10; i++)
    printf("%d ", values[i][i]);
  printf("\n");
  for(int i = 0; i < 10; i++)
  	free(values[i]);
  free(values);
}

/**
 * If s points to the string "blue", then print "Orange and BLUE!". If s points
 * to the string
 * "orange", then print "ORANGE and blue!" else just print "orange and blue!". Use
 * strcmp() and
 * friends to test for string equality.
 *
 * @param s
 *     Input parameter, used to determine which string is printed.
 */
void fourteen(const char *s) { //works 100%. strcmp returns 0 if equal
	if(!strcmp(s, "blue")) printf("Orange and BLUE!\n");
	else if(!strcmp(s, "orange")) printf("ORANGE and blue!\n");
	else printf("orange and blue!\n");
}

/**
 * Prints out a specific string based on the input parameter (value).
 *
 * @param value
 *     Input parameter, used to determine which string is printed.
 */
void fifteen(const int value) { //works 100%
  switch (value) {
  case 1:
    printf("You passed in the value of one!\n");
	break;
  case 2:
    printf("You passed in the value of two!\n");
	break;
  default:
    printf("You passed in some other value!\n");
  }
}

/**
 * Returns a newly allocated string on the heap with the value of "Hello".
 * This should not be freed.
 *
 * @returns
 *     A newly allocated string, stored on the heap, with the value "Hello".
 */
char *sixteen() { //works 100%
  char *s = malloc(6);
  strcpy(s, "Hello");
  return s;
}

/**
 * Prints out the radius of a circle, given its diameter.
 *
 * @param d
 *     The diameter of the circle.
 */
void seventeen(const int d) { //I think works
  printf("The radius of the circle is: %f.\n", ((float) d) / 2.0f);
}

/**
 * Manipulates the input parameter (k) and prints the result.
 *
 * @param k
 *     The input parameter to manipulate.
 */
void eighteen(const int k) { //works 100%. can't modify k b/c const
	int x = (k * k);
	x += x;
	x *= x;
	x -= 1;
  printf("Result: %d\n", x);
}

/**
 * @brief
 *     Clears the bits in "value" that are set in "flag".
 *
 * This function will apply the following rules to the number stored
 * in the input parameter "value":
 * (1): If a specific bit is set in BOTH "value" and "flag", that
 *      bit is NOT SET in the result.
 * (2): If a specific bit is set ONLY in "value", that bit IS SET
 *      in the result.
 * (3): All other bits are NOT SET in the result.
 *
 * Examples:
 *    clear_bits(value = 0xFF, flag = 0x55): 0xAA
 *    clear_bits(value = 0x00, flag = 0xF0): 0x00
 *    clear_bits(value = 0xAB, flag = 0x00): 0xAB
 *
 * @param value
 *     The numeric value to manipulate.
 *
 * @param flag
 *     The flag (or mask) used in order to clear bits from "value".
 */
long int clear_bits(long int value, long int flag) { //works 100%
  // TODO clear_bits
  return (value & ~flag);
}

/**
 * @brief
 *     A little finite automaton in C.
 *
 * This function will use a provided transition function to simulate a DFA over
 * an input string. The function returns the final state.
 *
 * The DFA starts in state 0. For each character in input_string, call the
 * transition function with the current state and the current character. The
 * current state changes to the state the transition function returns.
 *
 * So, in pseudo code:
 *
 * state = 0
 * for character in input_string:
 *     state = transition_function(state, character)
 *
 * NOTE: the transition_function parameter is a "function pointer." For more
 * information about these fun creatures, see this:
 * http://www.cprogramming.com/tutorial/function-pointers.html
 *
 * @param transition - function pointer to a transition function to use
 * @param input_string - string to run the automaton over
 *
 * @return the final state
 */
int little_automaton(int (*transition)(int, char), const char *input_string) { //works 100%
  int state = 0;
  // put something here
  	int i = 0;
	while(input_string[i]) {
		state = transition(state, input_string[i++]);
	}
  return state;
}
