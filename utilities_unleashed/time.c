/**
 * Lab: Utilities Unleashed
 * CS 241 - Fall 2016
 */
#include <unistd.h> //for exec
#include <time.h>
#include <stdlib.h> //for exit
#include <sys/wait.h> //for waitpid
#include "format.h"

//#include <stdio.h> //TEMP
//#include <string.h> //TEMP

int main(int argc, char *argv[]) { 
	if(argc == 1) { //time is a program that runs other programs -- needs an argument ./time <program name> <args> ...
		print_time_usage();
	}
	char * target = argv[1];
	while(1) {
		struct timespec start;
		clock_gettime(CLOCK_MONOTONIC, &start);
		pid_t child = fork();
		if(child == -1)
			print_fork_failed();
		else if(child == 0) { //child
			/*char * hi = "n\0";
			strcat(hi, "two");*/
			//exit(0);
			execvp(target, &argv[1]);
			print_exec_failed(); //exec never returns if successful; exits the process of whatever called it w/ success signal
		} else { //parent
			int status = 0;
			waitpid(child, &status, 0);
			struct timespec end;
			clock_gettime(CLOCK_MONOTONIC, &end);
			if(WIFEXITED(status)==0 || WEXITSTATUS(status) != 0)
				exit(1);
			double diff = ((double)(end.tv_nsec - start.tv_nsec)) / 1000000000.0 + difftime(end.tv_sec, start.tv_sec);
			display_results(argv, diff);
			exit(0);
		}
	}
	return 0; 	
}
