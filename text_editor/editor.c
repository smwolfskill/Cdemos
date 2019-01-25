/**
 * Machine Problem: Text Editor
 * CS 241 - Fall 2016
 */

#include "document.h"
#include "editor.h"
#include "format.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> //for isdigit()

#define MAX(A, B) (A > B ? A : B)
#define MIN(A, B) (A < B ? A : B)

char *get_filename(int argc, char *argv[]) { //think done
  // TODO implement get_filename
  // take a look at editor_main.c to see what this is used for
  if(argc != 2) { print_usage_error(); return NULL; }
  return argv[1];
}

void handle_display_command(Document *document, const char *command) { //think done
  // TODO implement handle_display_command  if(command[0] != 'p') ;
  if(document == NULL || Document_size(document) == 0) { 
  	print_document_empty_error(); return; 
  }
  short err = 1;
  short hasLineNum = 0;
  size_t i = 0;
  char * lineNum = malloc(strlen(command));
  while(command[i]) { //check command formatting and record the line#, if any
  	if(i == 0 && command[i] != 'p') break;
  	if(i == 1 && command[i] != ' ') { err = 1; break; }
  	if(i > 1) {
  		if(!isdigit((int) command[i])) { err = 1; break; }
  		lineNum[i-2] = command[i];
  		hasLineNum = 1;
  	}
  	err = 0;
  	i++;
  }
  //Return due to errors, or execute command
  if(err) { invalid_command(command); free(lineNum); return; }
  if(hasLineNum) { //print specific line
  	long lineNum_l = strtol(lineNum, (char**) NULL, 10);
  	//fprintf(stderr, "LINE#%ld\n", lineNum_l);
  	if(lineNum_l < 1 || lineNum_l > (long) Document_size(document)) {
  		invalid_line(); free(lineNum); return;
  	}
  	i = MAX(1, lineNum_l - 5); //print previous 5 lines if exist
  	size_t end = MIN((long) Document_size(document), lineNum_l + 5); //print next 5 lines if exist
  	while(i <= end) {
  		//fprintf(stderr, "\ti = %ld\n", i);
  		print_line(document, (int) i); 
  		i++; 
  	}
  } else { //print all lines
  	i = 1;
  	while(i <= Document_size(document)) { print_line(document, (int) i); i++;}
  }
  free(lineNum);
}

void handle_write_command(Document *document, const char *command) {
  // TODO implement handle_write_command
  if(document == NULL) { print_document_empty_error(); return; }
  short err = 1;
  size_t i = 0;
  size_t numStartIndex = 0; //inclusive
  size_t numEndIndex = 2;  //excl.
  size_t textStartIndex = 0; //incl.
  char * lineNum, *text;
  while(command[i]) { //check command formatting and record the line#, if any
  	if(i == 0 && command[i] != 'w') break;
  	if(i == 1 && command[i] != ' ') { err = 1; break; }
  	//if(i == 2 && !isdigit((int) command[i])) { err = 1; break; }
  	if(i > 1) {
  		if(isdigit((int) command[i])) { numStartIndex = 2; numEndIndex++; }
  		else {
  			if(numStartIndex == 0) {err = 1; break; } //never found line#
  			if(isspace((int)command[i])) { textStartIndex = i + 1; break; }
  			err = 1; break; //found line#, but garbage after it (no space)
  		}
  	}
  	err = 0;
  	i++;
  }
  //Return if command formatting errors; else parse again to get lineNum
  if(err || textStartIndex == 0) { invalid_command(command); return; }
  lineNum = malloc(numEndIndex - numStartIndex + 1);
  i = numStartIndex;
  while(i < numEndIndex) { lineNum[i-numStartIndex] = command[i]; i++;}
  lineNum[numEndIndex-numStartIndex] = '\0';
  //Now convert to find line# and error check it
  long lineNum_l = strtol(lineNum, (char**) NULL, 10);
  free(lineNum);
     //fprintf(stderr, "\tLINE#%ld\n", lineNum_l);
  if(lineNum_l < 1) { //if lineNum_l is greater than Document_size(document), still write to that line and insert blank lines up to that line
  	invalid_line(); return;
  }
  //Parse again to find text to write or append
  text = malloc(strlen(command));
  i = textStartIndex;
  while(command[i]) { text[i-textStartIndex] = command[i]; i++; } 
  text[i-textStartIndex] = '\0';
  //fprintf(stderr, "\tParsed everything successfully:\n\t\t%s[;]\n\n", text);
  //Finally, perform the command. Account for multi-line:
  size_t tokens = 0;
  char ** splits = strsplit(text, "$", &tokens);
  i = 0;
  while(i < tokens) {//have "$"
  	if(i > 0 || lineNum_l > (long) Document_size(document))
  		Document_insert_line(document, ((size_t) lineNum_l) + i, splits[i]);
  	else Document_set_line(document, (size_t) lineNum_l, splits[i]);
  	free(splits[i]);
  	i++;
  }
  //Cleanup:
  free(text);
  free(splits);
}

void handle_append_command(Document *document, const char *command) {//think done
  // TODO implement handle_append_command
  if(document == NULL) { print_document_empty_error(); return; }
  short err = 1;
  size_t i = 0;
  size_t numStartIndex = 0; //inclusive
  size_t numEndIndex = 2;  //excl.
  size_t textStartIndex = 0; //incl.
  char * lineNum, *text;
  while(command[i]) { //check command formatting and record the line#, if any
  	if(i == 0 && command[i] != 'a') break;
  	if(i == 1 && command[i] != ' ') { err = 1; break; }
  	//if(i == 2 && !isdigit((int) command[i])) { err = 1; break; }
  	if(i > 1) {
  		if(isdigit((int) command[i])) { numStartIndex = 2; numEndIndex++; }
  		else {
  			if(numStartIndex == 0) {err = 1; break; } //never found line#
  			if(isspace((int)command[i])) { textStartIndex = i + 1; break; }
  			err = 1; break; //found line#, but garbage after it (no space)
  		}
  	}
  	err = 0;
  	i++;
  }
  //Return if command formatting errors; else parse again to get lineNum
  if(err || textStartIndex == 0) { invalid_command(command); return; }
  lineNum = malloc(numEndIndex - numStartIndex + 1);
  i = numStartIndex;
  while(i < numEndIndex) { lineNum[i-numStartIndex] = command[i]; i++;}
  lineNum[numEndIndex-numStartIndex] = '\0';
  //Now convert to find line# and error check it
  long lineNum_l = strtol(lineNum, (char**) NULL, 10);
  free(lineNum);
     //fprintf(stderr, "\tLINE#%ld\n", lineNum_l);
  if(lineNum_l < 1) { //if lineNum_l is greater than Document_size(document), still write to that line and insert blank lines up to that line
  	invalid_line(); return;
  }
  //Parse again to find text to write or append
  text = malloc(strlen(command));
  i = textStartIndex;
  while(command[i]) { text[i-textStartIndex] = command[i]; i++; } 
  text[i-textStartIndex] = '\0';
  //fprintf(stderr, "\tParsed everything successfully:\n\t\t%s[;]\n\n", text);
  //Finally, perform the command. LATER: ACCOUNT FOR MULTI-LINE
  size_t tokens = 0;
  char ** splits = strsplit(text, "$", &tokens);
  i = 0;
  while(i < tokens) {//have "$"
  	if(i > 0 || lineNum_l > (long) Document_size(document)) //either 1st input and line DNE (so create it), or it's a multi-input, so insert
  		Document_insert_line(document, ((size_t) lineNum_l) + i, splits[i]);
  	else { //only can happen on i==0
  		char * pretext = splits[i];
  		short freePretext = 0;
  		size_t docLine_len = 0; 
  		docLine_len = strlen(Document_get_line(document, (size_t) lineNum_l)); //the length of the line in the document
  		if(docLine_len > 0) { 
  			//text = realloc(text, docLine_len + strlen(command) - textStartIndex + 1);
  			pretext = malloc(docLine_len + strlen(command) - textStartIndex + 1);
  			freePretext = 1;
 	 		pretext[0] = '\0';
 	 		pretext = strcat(pretext, Document_get_line(document, (size_t) lineNum_l));
 	 		pretext = strcat(pretext, splits[i]);
 	 		//free(text);
 	 		pretext[docLine_len+strlen(command)-textStartIndex] = '\0';
		}
  		Document_set_line(document, (size_t) lineNum_l, pretext);
  		if(freePretext) free(pretext);
  		//Document_set_line(document, (size_t) lineNum_l, splits[i]);
  	}
  	free(splits[i]);
  	i++;
  }
  //Cleanup:
  free(text);
  free(splits);
}

void handle_delete_command(Document *document, const char *command) {//think done
  // TODO implement handle_delete_command
  if(document == NULL || Document_size(document) == 0) { 
  	print_document_empty_error(); return; 
  }
  short err = 1;
  size_t i = 0;
  char * lineNum = malloc(strlen(command));
  while(command[i]) { //check command formatting and record the line#, if any
  	if(i == 0 && command[i] != 'd') break;
  	if(i == 1 && command[i] != ' ') { err = 1; break; }
  	if(i > 1) {
  		if(!isdigit((int) command[i])) { err = 1; break; }
  		lineNum[i-2] = command[i]; 
  		lineNum[i-1] = '\0'; //not sure if nec.
  	}
  	err = 0;
  	i++;
  }
  //Return due to errors, or delete line
  if(err) { invalid_command(command); free(lineNum); return; }
  	long lineNum_l = strtol(lineNum, (char**) NULL, 10);
  	//fprintf(stderr, "LINE#%ld\n", lineNum_l);
  	if(lineNum_l < 1 || lineNum_l > (long) Document_size(document)) {
  		invalid_line(); free(lineNum); return;
  	}
  	Document_delete_line(document, (size_t) lineNum_l);
  	free(lineNum);
}

void handle_search_command(Document *document, const char *command) {//think done
  // TODO implement handle_search_command
  if(document == NULL || Document_size(document) == 0) {
  	print_document_empty_error(); return;
  }
  if(*command != '/') {
  	invalid_command(command); return;
  }
  if(!strcmp(command, "/")) return; //do nothing if command = "/"
  size_t line = 1;
  const char * cur;
  while(line <= Document_size(document)) {
  	cur = Document_get_line(document, line);
  	//fprintf(stderr, "\tline %lu:%s[;]\n", line, cur);
  	char * found = strstr(cur, &command[1]); //&command[1] is what searching for
  	if(found != NULL) print_search_line((int) line, cur, found, &command[1]);
  	line++;
  }
}

void handle_save_command(Document *document, const char *filename) {//think done
  // TODO implement handle_save_command
  Document_write_to_file(document, filename);
}