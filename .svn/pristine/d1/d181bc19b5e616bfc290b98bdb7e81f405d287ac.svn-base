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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h> //for kill

#define MODE (S_IRUSR | S_IWUSR | S_IWRITE | S_IREAD | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

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

//Returns 1 if non-zero exit status
int printExitStatus(const char* name, int id, int exitStatus) { //(id): -1 indicates id not applicable
	if(WEXITSTATUS(exitStatus) != 0) {
		if(id >= 0)
			fprintf(stdout, "%s %d exited with status %d\n", name, id, WEXITSTATUS(exitStatus));
		else fprintf(stdout, "%s exited with status %d\n", name, WEXITSTATUS(exitStatus));
		return 1;
	}
	return 0;
}

void usage() { print_mr2_usage(); }
//mr2: MULTIPLE REDUCERS
int main(int argc, char **argv) {
	if(argc != 7) { usage(); return 1; }
  // setup pipes
	int mapper_count = stringToInt(argv[5]);
	int reducer_count = stringToInt(argv[6]);
	int mapperPipes[mapper_count][2];
	int shufflerPipe[2];
	pipe(shufflerPipe);
	//Create or reset output file.
	int fOut = open(argv[2], O_CREAT | O_TRUNC | O_APPEND | O_RDWR, MODE);
	//close(fOut);
  // Start a splitter process for each mapper. (same as pt1)
	//Make sure to CLOSE ALL UNUSED FILE DESCRIPTORS!
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
			close(shufflerPipe[0]); //no interaction w/ shuffler pipe
			close(shufflerPipe[1]); // ^
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
			//diff from pt1: redirect stdout to shuffler pipe write
			close(shufflerPipe[0]); //not reading from shuffler pipe
			close(mapperPipes[i][1]); //not writing
			int j = i + 1;
			while(j < mapper_count) { //close pipes for other mappers (that aren't already closed)
				close(mapperPipes[j][0]);
				close(mapperPipes[j][1]);
				j++;
			}
			dup2(mapperPipes[i][0], STDIN_FILENO); //stdin now points to i'th mapper pipe read
			dup2(shufflerPipe[1], STDOUT_FILENO); //stdout now points to shuffler pipe write
			execlp(argv[3], argv[3], (const char*)NULL);
			return 1; //exec failed
		} //else, main:
		close(mapperPipes[i][0]); //main won't read from or write to mapper pipes
		close(mapperPipes[i][1]);
		i++;
	}
  // start shuffler. (at this point, all mapper pipes are closed in main):
    //a. Create argv for shuffler which includes the names of all fifo files
	char * shufflerArgv[reducer_count+2];
	shufflerArgv[0] = "./shuffler";
	shufflerArgv[reducer_count+1] = NULL;
	i = 1;
	while(i <= reducer_count) {
		if(asprintf(&shufflerArgv[i], "./fifo_%d", i) == -1) {
			write(2, "asprintf failed!\n", 17);
		} else {
			if(mkfifo(shufflerArgv[i], S_IRWXU) == -1) {
				fprintf(stderr, "mkfifo failed to create '%s'!\n", shufflerArgv[i]);
			}
		}
		i++;
	}
	//b. Fork
	pid_t shuffler = fork();
	if(shuffler == -1) {
			fprintf(stderr, "Fork failed! Done.\n"); return 1;
	} else if(shuffler == 0) { //shuffler child
		close(shufflerPipe[1]); //not writing
		dup2(shufflerPipe[0], STDIN_FILENO); //stdin now points to shuffler pipe read
		//don't redirect stdout, b/c shuffler writes to output files
		execv("./shuffler", shufflerArgv);
		return 1; //exec failed
	} //else, main:
	close(shufflerPipe[0]);	//main won't read from or write to shuffler pipe
	close(shufflerPipe[1]);
  // start reducers. (at this point, ALL pipes closed)
	//fOut = open(argv[2], O_WRONLY | O_APPEND);
	pid_t reducers[reducer_count];
	i = 0;
	while(i < reducer_count) {
		reducers[i] = fork();
		if(reducers[i] == -1) {
			fprintf(stderr, "Fork failed! Done.\n"); return 1;
		} else if(reducers[i] == 0) { //reducer child
			//fOut = open(argv[2], O_WRONLY | O_APPEND);
			int fIn = open(shufflerArgv[i+1], O_RDONLY | O_CLOEXEC); //blocks until shuffler opens for writing
			//printf("\treducer %d: opened fIn...\n", i);
			dup2(fIn, STDIN_FILENO); //stdin now points to i'th fifo file
			dup2(fOut, STDOUT_FILENO); //stdout now points to output file
			execlp(argv[4], argv[4], (const char*)NULL);
			return 1; //exec failed
		} //else, main:
		i++;
	}
  // wait for everything to finish
	short fail = 0;
	i = 0;
	while(i < mapper_count) {
		int splitterStatus, mapperStatus;
		waitpid(splitters[i], &splitterStatus, 0);
		waitpid(mappers[i], &mapperStatus, 0);
		fail = printExitStatus("splitter", i, splitterStatus) | fail;
		fail = printExitStatus(argv[3], i, mapperStatus) | fail;
		i++;
	}
	//printf("all splitters, mappers returned...\n");
	if(fail) kill(shuffler, SIGUSR1);
	int shufflerStatus = 0;
	waitpid(shuffler, &shufflerStatus, 0);
	printExitStatus("shuffler", -1, shufflerStatus);
	//printf("shuffler returned...\n");
	i = 0;
	while(i < reducer_count) {
		if(fail) kill(reducers
[i], SIGUSR1);
		int reducerStatus = 0;
		waitpid(reducers[i], &reducerStatus, 0);
		printExitStatus(argv[4], i, reducerStatus);
		i++;
	}
	//printf("ALL CHILDREN RETURNED\n");
	//After: delete fifo files!
	i = 1;
	while(i <= reducer_count) {
		if(unlink(shufflerArgv[i]) == -1) {
			fprintf(stderr, "ERROR: Could not delete fifo file '%s'!\n", shufflerArgv[i]);
		}
		free(shufflerArgv[i]);
		i++;
	}
	// Count the number of lines in the output file.
	//close(fOut);
	//fOut = open(argv[2], O_RDONLY);
  	lseek(fOut, 0, SEEK_SET);
	size_t count = 0;
	char buf;
	ssize_t result;
	while( (result=read(fOut, &buf, 1)) == 1) {
		if(buf == '\n') count++;
	}
	fprintf(stdout, "output pairs in %s: %lu\n", argv[2], count);
	close(fOut);
	return 0;
}
