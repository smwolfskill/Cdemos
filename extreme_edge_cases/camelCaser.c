/**
 * Chatroom Lab
 * CS 241 - Fall 2016
 */
#include "camelCaser.h"
#include <stdlib.h>
#include <stdio.h> //for test printing
#include <stdbool.h> //I shouldn't have to do this...but I do...
#include <string.h>
#include <ctype.h> //for isalpha(), ...

char **camel_caser(const char *input_str) { 
	if(input_str == NULL) return NULL;
	printf("%s\n", input_str); //TEMP
	int numSentences = 0; //preliminary check to see how many arrays to allocate for output
	int readPos = -1; //position of the counter for reading input_str
	while(input_str[++readPos]) {
		if(ispunct(input_str[readPos])) numSentences++;
	}
	readPos = -1;
	//Now for the actual stuff:
	char ** output = malloc((numSentences + 1) * sizeof(char *));
	output[numSentences] = NULL;
	int outputPos = 0; //position of the counter for writing into output
	int sentStart = 0; //position of the start of the current sentence
	int sentLen = 0; //length of current sentence so far (EXCLUDING spaces, punct!))
	while(input_str[++readPos]) {
		//printf("readPos = %d; %c\n", readPos, input_str[readPos]);
		if(ispunct(input_str[readPos])) { //If done with current sentence, perform operations and write it to output
			//printf("\tgetCamelCase(.., %d, %d)\n", sentStart, sentLen);
			//output[outputPos++] = getCamelCase(input_str, sentStart, sentLen);
			output[outputPos] = malloc(sentLen+1);
			//former getCamelCase routine
			if(sentLen != 0) {
				int i = 0;
				bool nextIsUpper = false;
				while(i < sentLen) {
					//printf("\thelper: @ pos %d = %c\n", start, input_str[start]);
					if(isalpha(input_str[sentStart])) {
						output[outputPos][i++] = (nextIsUpper ? toupper(input_str[sentStart]) : tolower(input_str[sentStart]));
						nextIsUpper = false;
					} else {
						nextIsUpper = (i > 0); //i>0 implies writtenThings	
						if(!isspace(input_str[sentStart])) //account for non-alphanu	meric valid chars, e.g. '1'
							output[outputPos][i++] = input_str[sentStart];
						}
						sentStart++;
				}
				output[outputPos][sentLen] = '\0';
			}
			//(end getCamelCase)
			outputPos++;
			sentStart = readPos + 1;
			sentLen = 0;
		} else if(!isspace(input_str[readPos])) sentLen++;
	}
	/*char * TEST = getCamelCase("Y es hi. Bob.", 0, 5);
	printf("%s\n", TEST);
	free(TEST);
	puts("end\n");*/
	
	/*int i = 0;
	while(output[i]) {
		printf("MYOUTPUT: '%s'\n", output[i++]);
	}*/
	return output;
}

/* Gets the correct camelCase format for the sentence between [start, end].
 * sentLen will be the length of the sentence afterwards.
 * Assumes no punctuation; that [start, end] is just one sentence.
 * EX: given ("Y es. Bob.", 0, 3, 3) want return "yEs" */
/*char * getCamelCase(const char * input_str, int start, int sentLen) {
	if(sentLen == 0) return "";
	char * output = malloc(sentLen+1);
	int i = 0;
	bool nextIsUpper = false;
	while(i < sentLen) {
		//printf("\thelper: @ pos %d = %c\n", start, input_str[start]);
		if(isalpha(input_str[start])) {
			output[i++] = (nextIsUpper ? toupper(input_str[start]) : tolower(input_str[start]));
			nextIsUpper = false;
		} else {
			nextIsUpper = (i > 0); //i>0 implies writtenThings
			if(!isspace(input_str[start])) //account for non-alphanumeric valid chars, e.g. '1'
				output[i++] = input_str[start];
		}
		start++;
	}
	output[sentLen] = '\0';
	//printf("\thelper: return...\n");
	return output;
}*/