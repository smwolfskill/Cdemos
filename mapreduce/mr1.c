/**
 * MapReduce
 * CS 241 - Fall 2016
 */

#include "common.h"
#include "utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int stringToInt(char* str) { //only works for positive ints, but that's all we need
	int len = strlen(str);
	int i = len - 1;
	int mul = 1;
	int num = 0;
	while(i >= 0) {
		num += mul * ((int) str[i] - (int) '0');
		i--;
		mul *= 10;
	}
	return num;
}

char* digitToString(int digit) { //converts int in [0, 9] to string
	if(digit < 0 || digit > 9) {
		fprintf(stderr, "\tdigitToInt: called w/ (digit) = %d. Expected [0, 9]. Done.\n", digit);
		return NULL;
	}
	char* result = malloc(2);
	result[1] = '\0';
	result[0] = (char) (digit + (int) '0');
	return result;
}

void printExitStatus(const char* name, int id, int exitStatus) { //(id): -1 indicates id not applicable
	if(WEXITSTATUS(exitStatus) != 0) {
		if(id >= 0)
			fprintf(stdout, "%s %d exited with status %d\n", name, id, WEXITSTATUS(exitStatus));
		else fprintf(stdout, "%s exited with status %d\n", name, WEXITSTATUS(exitStatus));
	}
}

void usage() { print_mr1_usage(); }

int main(int argc, char **argv) {
	if(argc != 6) { usage(); return 1; }
  // Create an input pipe for each mapper.
	int mapper_count = stringToInt(argv[5]);
	int mapperPipes[mapper_count][2];
  // Create one input pipe for the reducer.
	int reducerPipe[2];
	pipe(reducerPipe);
  // Open the output file.
	FILE * fOut = fopen(argv[2], "w+");
  // Start a splitter process for each mapper.
	//TODO: CLOSE ALL UNUSED FILE DESCRIPTORS!
	pid_t splitters[mapper_count];
	int i = 0;
	while(i < mapper_count) {
		pipe(mapperPipes[i]);
		splitters[i] = fork();
		if(splitters[i] == -1) {
			fprintf(stderr, "Fork failed! Done.\n"); return 1;
		} else if(splitters[i] == 0) { //splitter child
			//a. Convert id (i) from int to string
			char* temp = digitToString(i);
			if(!temp) return 3;
			char id[2]; id[0] = temp[0]; id[1] = '\0'; //to prevent mem leak (a little silly).
			free(temp);
			//b. Set fds, open splitter
			close(reducerPipe[0]); //no interaction w/ reducer pipe
			close(reducerPipe[1]); // ^
			close(mapperPipes[i][0]); //not reading
			int j = 0;
			while(j < i) { //close EXISTING pipes for other mappers (only created up thru i'th)
				close(mapperPipes[j][0]);
				close(mapperPipes[j][1]);
				j++;
			}
			dup2(mapperPipes[i][1], STDOUT_FILENO); //stdout now points to i'th mapper pipe write
			execl("./splitter", "./splitter", argv[1], argv[5], id, (const char*)NULL);
			return 1; //exec failed
		} else i++; //main
	}
  // Start all the MAPPER processes:
	pid_t mappers[mapper_count];
	i = 0;
	while(i < mapper_count) {
		mappers[i] = fork();
		if(mappers[i] == -1) {
			fprintf(stderr, "Fork failed! Done.\n"); return 1;
		} else if(mappers[i] == 0) { //mapper child
			close(reducerPipe[0]); //not reading from reducer pipe
			close(mapperPipes[i][1]); //not writing
			int j = i + 1;
			while(j < mapper_count) { //close pipes for other mappers (that aren't already closed)
				close(mapperPipes[j][0]);
				close(mapperPipes[j][1]);
				j++;
			}
			dup2(mapperPipes[i][0], STDIN_FILENO); //stdin now points to i'th mapper pipe read
			dup2(reducerPipe[1], STDOUT_FILENO); //stdout now points to reducer pipe write
			execlp(argv[3], argv[3], (const char*)NULL);
			return 1; //exec failed
		} //else, main:
		close(mapperPipes[i][0]); //main won't read from or write to mapper pipes
		close(mapperPipes[i][1]);
		i++;
	}
  // Start the REDUCER process. (at this point, all mapper pipes are closed in main):
	pid_t reducer = fork();
	if(reducer == -1) {
			fprintf(stderr, "Fork failed! Done.\n"); return 1;
	} else if(reducer == 0) { //reducer child
		close(reducerPipe[1]); //not writing
		dup2(reducerPipe[0], STDIN_FILENO); //stdin now points to reducer pipe read
		dup2(fOut->_fileno, STDOUT_FILENO); //stdout now points to output file
		execlp(argv[4], argv[4], (const char*)NULL);
		return 1; //exec failed
	} //else, main:
	close(reducerPipe[0]);	//main won't read from or write to reducer pipe
	close(reducerPipe[1]);
  // Wait for the reducer to finish. (at this point, ALL pipes are closed in main):
  	int reducerStatus;
	waitpid(reducer, &reducerStatus, 0);
  // Print nonzero subprocess exit codes.
	i = 0;
	while(i < mapper_count) {
		int splitterStatus, mapperStatus;
		waitpid(splitters[i], &splitterStatus, 0);
		waitpid(mappers[i], &mapperStatus, 0);
		printExitStatus("splitter", i, splitterStatus);
		printExitStatus(argv[3], i, mapperStatus);
		i++;
	}
	printExitStatus(argv[4], -1, reducerStatus);
  // Count the number of lines in the output file.
  	fseek(fOut, 0, SEEK_SET);
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
