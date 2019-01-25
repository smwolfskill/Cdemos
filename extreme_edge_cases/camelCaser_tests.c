/**
 * Chatroom Lab
 * CS 241 - Fall 2016
 */
#include "camelCaser_tests.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camelCaser.h"

/*
 * Testing function for various implementations of camelCaser.
 *
 * @param  camelCaser   A pointer to the target camelCaser function.
 * @return              Correctness of the program (0 for wrong, 1 for correct).
 */
int test_camelCaser(char **(*camelCaser)(const char *)) {
  // TODO: Return 1 if the passed in function works properly; 0 if it doesn't.
  // Only allow <= 16 different tests! return 1 if success, 0 else.
	int success = 1;
	char ** output; 
	//Test 1: "" -> [NULL]
	output = (*camelCaser)("");
	if(*output != NULL) { 
		printf("Test 1 FAILED ("""")\n");
		success = 0;
	}
	printOutput(output);
	freeOutput(output);
	//Test 2: NULL -> NULL
	output = (*camelCaser)(NULL);
	if(output != NULL) {
		printf("Test 2 FAILED (NULL)\n");
		success = 0;
	}
	//Test 3: "." -> ['', NULL]
	output = (*camelCaser)(".");
	if(output == NULL || *output == NULL || strcmp(*output, "") || output[1] != NULL) { 
		printf("Test 3 FAILED (""."")\n");
		success = 0;
	}
	printOutput(output);
	freeOutput(output);
	//Test 4: Test for multiple sentences and multiple spaces
	output = (*camelCaser)("hi. I am bob.   K BYE .");
	int i = 0;
	int failed = 0;
	if(output == NULL || *output == NULL) failed = 1;
	else {
		while(output[i]) {
			switch(i) {
				case 0: if(strcmp(output[i], "hi")) failed = 1;
					break;
				case 1: if(strcmp(output[i], "iAmBob")) failed = 1;
					break;
				case 2: if(strcmp(output[i], "kBye")) failed = 1;
					break;
				default: failed = 1; //more strings than expected -> FAILED
			}
			if(failed == 1) break;
			i++;
		}
	}
	if(failed) {
		printf("Test 4 FAILED (""hi. I am bob.   K BYE ."")\n");
		success = 0;
	}
	printOutput(output);
	freeOutput(output);
	//Test 5: Test for valid but non-alphanumeric characters (e.g. '1')
	//        as well as capitalizing the first ALPHANUMERIC letter in a word
	output = (*camelCaser)("A 74a.");
	if(output == NULL || *output == NULL || strcmp(*output, "a74A")) {
		printf("Test 5 FAILED (""A 74a."")\n");
		success = 0;
	}
	printOutput(output);
	freeOutput(output);
	//
	return success;
}

void printOutput(char ** output) {
	puts("YOUR OUTPUT:");
	if(output != NULL) {
		if(*output == NULL) printf("(empty (NULL) *output!)\n");
	 	while(*output) {
	 		printf("'%s'\n", *output);
	 		output++;
	 	}
	} else puts("(char ** output was NULL!!)");
}

void freeOutput(char ** output) {
	int i = -1;
	while(output[++i]) {
		//printf("\tfreeOutput(): i = %d; %s\n", i, output[i]);
		if(*output[i] != '\0') free(output[i]);
	}
	free(output);
}
