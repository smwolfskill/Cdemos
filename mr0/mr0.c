/**
 * Map Reduce 0 Lab
 * CS 241 - Fall 2016
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include "utils.h"

void printUsage() { printf("usage: ./mr0 <input_file> <output_file> <mapper_executable> <reducer_executable>\n"); }

int main(int argc, char **argv) {
	if(argc != 5) { printUsage(); return 1; } //not required
  // Open the input file.
	FILE* fIn = fopen(argv[1], "r");
	if(!fIn) { fprintf(stdout, "ERROR: could not open input file '%s'! Quitting.\n", argv[1]); printUsage(); return 1; }
  // Create a pipe to connect the mapper to the reducer.
	int fds[2]; //ALWAYS read from 0 and write to 1 !
	pipe(fds);
  // Open the output file.
	FILE* fOut = fopen(argv[2], "w");
	if(!fOut) {fprintf(stdout, "ERROR: could not create output file '%s'! Quitting.\n", argv[1]); return 1; }
  // Start the mapper.
	pid_t mapper = fork();
	if(mapper == -1) { fprintf(stdout, "ERROR: fork failed to create mapper child!\n"); exit(1); }
	else if(mapper == 0) { //MAPPER:
		printf("mapper started!\n");
		fclose(fOut);
		close(fds[0]); //don't need to read anything
		dup2(fIn->_fileno, STDIN_FILENO); //stdin now points to input file
		dup2(fds[1], STDOUT_FILENO); //stdout now points to our pipe out
		execlp(argv[3], argv[3], (const char*) NULL);
		fprintf(stderr, "ERROR: mapper: exec failed to run '%s'!\n", argv[3]);
		exit(1);
	} else { //parent
  // Start the reducer.
  		close(fds[1]); //don't need to write anything
  		fclose(fIn);
		pid_t reducer = fork();
		if(reducer == -1) { kill(mapper, SIGKILL); fprintf(stdout, "ERROR: fork failed to create reducer child!\n"); exit(1); }
		else if(reducer == 0) { //REDUCER:
			printf("reducer started!\n");
			dup2(fds[0], STDIN_FILENO); //stdin now points to our pipe in
			dup2(fOut->_fileno, STDOUT_FILENO); //stdout now points to output file
			execlp(argv[4], argv[4], (const char*) NULL);
			fprintf(stderr, "ERROR: reducer: exec failed to run '%s'!\n", argv[4]);
			exit(1);
  // Wait for the reducer to finish. (me: also wait for mapper (order not matter?))
  		} else { //parent
  			close(fds[0]); //don't need to read anything
  			fclose(fOut);
  			int status = 0;
			waitpid(mapper, &status, 0);
  // Print nonzero subprocess exit codes.
			if(WEXITSTATUS(status) != 0) fprintf(stdout, "%s exited with status %d\n", argv[3], WEXITSTATUS(status));
  			status = 0;
			waitpid(reducer, &status, 0);
  // Print nonzero subprocess exit codes.
			if(WEXITSTATUS(status) != 0) fprintf(stdout, "%s exited with status %d\n", argv[4], WEXITSTATUS(status));
  // Count the number of lines in the output file.
			fOut = fopen(argv[2], "r");
			if(!fOut) { fprintf(stdout, "ERROR: could not open output file '%s'! Quitting.\n", argv[2]); printUsage(); return 1; }
			size_t count = 0;
			char * buf = NULL;
			size_t capacity = 0;
			ssize_t result;
			while( (result=getline(&buf, &capacity, fOut)) > 0) {count++;}
			free(buf);
			fprintf(stdout, "output pairs in %s: %lu\n", argv[2], count);
			fclose(fOut);
			return 0;
		}
	}
}
