/**
 * Lab: Utilities Unleashed
 * CS 241 - Fall 2016
 */
#include <stdio.h>
#include <unistd.h> //for exec
#include <sys/wait.h> //for waitpid
#include <stdlib.h> //for exit
#include <string.h>
#include <ctype.h> //for isalpha, etc.
#include "format.h"

extern char ** environ;

void mysetenv(char * dest, char * value, int overwrite) {
	if(-1 == setenv(dest, value, overwrite)) //Set and error check
		print_environment_change_failed();
}

int main(int argc, char *argv[]) { 
	if(argc == 2) { //ERROR
		print_env_usage();
	} else if(argc == 1) { //Mode 1: No arguments; list out all current environment vars.
		size_t i = 0;
		while(environ[i]) printf("%s\n", environ[i++]);
	} else { //Mode 2: ./env <var-list> <command-name> <command args> ...
		pid_t child = fork();
		if(child == -1) print_fork_failed();
		if(child == 0) { //is child. Point is to change env variables only for the lifetime of the child
			char * cur = strsep(&argv[1], ",");
			char * dest = NULL;
			char * value = NULL;
			while(cur != NULL) { //parse argv[1] to change the env. vars
				dest = strsep(&cur, "=");
				value = strsep(&cur, ",");
				//printf("%s;\n", cur);
				//printf("\tdest:%s; value:%s;\n", dest, value);
				if(value == NULL) { //means no "=" present in cur. ERROR
					free(cur);
					exit(1);
				}
				//NEW: Replace %<source> with source value in the middle of the string! Ex: %PWD/hi. Difficult!
				/*int i = 0;
				while(value[i]) {
					if(!isalpha(value[i]) && !isdigit(value[i]) && value[i] != '_') {
						//printf("%c\n", value[i]);
						if(value[i] == '%') {
							
						}
					}
					i++;
				}*/
				if(*value == '%') { //OLD: If starts with %, treat as env var. name
					value = getenv(&value[1]);
					
				}
				mysetenv(dest, value, 1);
				//Next:
				cur = strsep(&argv[1], ",");
			}
			//free(cur);
			//free(dest); causes segfault !! why?
			//free(value);
			execvp(argv[2], &argv[2]);
			print_exec_failed();
		} //Parent:
		int status = 0;
		waitpid(child, &status, 0);
		if(WIFEXITED(status)==0 || WEXITSTATUS(status) != 0) { 
			//printf("Child exited weirdly!!\n");
			exit(1); 
		}
	}
	return 0; 
}
