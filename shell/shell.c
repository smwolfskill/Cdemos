/**
 * Machine Problem: Shell
 * CS 241 - Fall 2016
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h> //for getpid, access, exec
#include <stdlib.h> //for getenv, exit
#include <sys/types.h> //for DIR
#include <dirent.h> //for DIR
#include <sys/wait.h> //for waitpid
#include <signal.h>

#include "format.h"
#include "log.h"
#include "shell.h"

extern char ** environ;

void catch(int signum) { return; }

void printHistory(Log * log) { //simple helper fxn to make doCommand more readable
	size_t logSize = Log_size(log);
	size_t i = 0;
	while(i < logSize) {
		print_history_line(i, Log_get_command(log, i));
		i++;
	}
}

/* Returns greatest line# the command with the search term is found on,
 * or if not found, prints "no match" msg and returns -1 */
int searchHistory(Log * log, char * input) {
	int logSize = (int) Log_size(log);
	if(logSize > 0) {
		if(!strcmp(input, "")) { //run last command; easy
			return (logSize - 1);
		} else { //have to search
			int i = logSize - 1;
			const char * cur;
			while(i >= 0) {
				cur = Log_get_command(log, (size_t) i);
				if(strstr(cur, input)) return i;
				i--;
			}
		}
	}
	print_no_history_match();
	return -1;
}

void cd(char ** args, size_t numArgs) {
	//"cd" = "cd ~" = go to home directory
	//"cd ." does nothing
	//"cd .." go back one directory
	if(numArgs == 1 || !strcmp(args[1], "~")) {
		setenv("PWD", "/home", 1);
		return;
	}
	if(!strcmp(args[1], ".")) return;
	if(!strcmp(args[1], "..")) {
		char newPath[strlen(getenv("PWD"))+1];
		newPath[0] = '\0';
		strcat(newPath, getenv("PWD"));
		newPath[strlen(getenv("PWD"))] = '\0';
		int i = strlen(getenv("PWD")) - 1;
		while(i >= 0) {
			if(newPath[i] == '/') {
				break;
			} else if(i == 0) {print_no_directory(args[1]); return;}
			i--;
		}
		char out[i+1];
		int j = 0;
		while(j < i) {
			out[j] = newPath[j];
			//printf("\tnewPath[%d] = %c\n", j, newPath[j]);
			j++;
		}
		out[i] = '\0';
		setenv("PWD", out, 1);
		return;
	}
	char * newPath = args[1];
	short freeNewPath = 0;
	if(*args[1] != '/') { //'/' means NOT relative to cd
		newPath = strdup(getenv("PWD"));
		size_t len = strlen(newPath) + strlen(args[1]) + 2;
		newPath = realloc(newPath, len);
		strcat(newPath, "/");
		strcat(newPath, args[1]);
		newPath[len-1] = '\0';
		freeNewPath = 1;
	}
	DIR * dir = opendir(newPath);
	if(!dir) print_no_directory(args[1]);
	else { closedir(dir); setenv("PWD", newPath, 1); }
	if(freeNewPath) free(newPath);
}

//Returns true if should add current input to log
short doCommand(Log * log, char * input) {
	size_t numArgs = 0;
	char ** args = strsplit(input, " ", &numArgs);//for delimiting input  	
	short addToLog = 1;
		if(numArgs == 1 && !strcmp(args[0], "!history")) { //built-in cmd: Print out history in order
			printHistory(log);
			addToLog = 0;
		} else {
			//printf("#args = %lu;%s;%s\n", numArgs, args[0], args[1]);
			if((numArgs == 1 || numArgs == 2) && !strcmp(args[0], "cd")) { //build-in cd (path). Don't say need to implement "..", so...
				cd(args, numArgs);
			} else if(numArgs == 1 && *args[0] == '#') { //built-in #(num)
				addToLog = 0;
				if(!strcmp(args[0], "#")) print_invalid_index();
				else {
					long lineNum = strtol(&args[0][1], (char**) NULL, 10);
					if(lineNum < 0 || lineNum >= (long) Log_size(log))
						print_invalid_index();
					else {
						const char * cmd = Log_get_command(log, lineNum);
						print_command(cmd);
						doCommand(log, (char*) cmd);
						Log_add_command(log, cmd);
					}
				}
			} else if(numArgs == 1 && *args[0] == '!') {//built-in !(search)
				//"!" will simply run the last cmd
				addToLog = 0;
				int lineNum = searchHistory(log, &args[0][1]);
				if(lineNum != -1) {
					const char * cmd = Log_get_command(log, lineNum);
					print_command(cmd);
					doCommand(log, (char*) cmd);
					Log_add_command(log, cmd);
				}
			} else { //not a recognized built-in cmd; fork and run execvp
				fflush(stdout);
				char * pwd = getenv("PWD");
				pwd[strlen(pwd)] = '\0';
				pid_t child = fork();
				if(!child) { //i'm child
					print_command_executed(getpid());
					chdir(pwd);
					execvp(args[0], args);
					print_exec_failed(args[0]);
					exit(1);
				} else if (child == -1) print_fork_failed();
				else { //I'm parent
					int status = 0;
					pid_t result = waitpid(child, &status, 0);
					if(result == -1) print_wait_failed();
				}
			}
		}
	if(args != NULL) free_args(args);
	return addToLog; //only don't when command is "!history" or special things for #(num) and !(search)
}

//Returns false if EOF. Gets the next input (usually stdin) and calls doCommand.
short getInputAndDo(Log * log, FILE * fd, short print) { 
	print_prompt(getenv("PWD"), getpid());
	short success = 1; //no exit signal
	char * input = NULL; //for current line of input
  	size_t capacity = 0;
  	//1. Get current line of input:
  	ssize_t result = getline(&input, &capacity, fd);
  	if(result == -1) { //quit if EOF
		//printf("EOF!\n");
		success = 0;
	} else if(result > 0) {
		if(input[result-1]=='\n') input[result-1] = '\0'; //replace terminating '\n' with '\0' which is much more useful
		if(print) print_command(input);
		if(doCommand(log, input)) Log_add_command(log, input);
	}
	//Clean-up:
	free(input);
	return success;
}

int shell(int argc, char *argv[]) {
	signal(SIGINT, catch);
  print_shell_owner("wolfski2");
	int exitSignal = 0;
  Log * log = NULL;
  char * histFile = NULL;
  short hCmd = 0; //if used that command before (don't want it twice!)
  short fCmd = 0;
  if(argc != 1 && argc != 3 && argc != 5) { print_usage(); exit(1); }
  //Check for -h first so can load -f given a possible existing history.
  //Also will check for invalid args.
  	int i = 1;
  	while(i < argc) {
  		if(!strcmp(argv[i], "-h") && !hCmd) {
  			if(access(argv[i+1], F_OK) == -1) print_history_file_error(); //file DNE
  			else histFile = argv[i+1];
  			log = Log_create_from_file(argv[i+1]);
  			hCmd = 1;
 	 	} else if(!strcmp(argv[i], "-f") && !fCmd) fCmd = 1;
 	 	else { print_usage(); exitSignal = 1;}
  		i += 2;
  	}
  fCmd = 0;
  if(!hCmd) log = Log_create();
  //Now given either a loaded history or blank one, check for -f:
  i = 1;
  	while(i < argc && !exitSignal) {
		if (!strcmp(argv[i], "-f") && !fCmd) {
  			if(access(argv[i+1], F_OK) == -1) { //file DNE
					print_script_file_error();
					exitSignal = 1;
				} else { //parse file argv[i+1]; each line is treated as a command
  					FILE * fd = fopen(argv[i+1], "r");
					while(getInputAndDo(log, fd, 1));
					fclose(fd);
				}
  			fCmd = 1;
  		}
  		i += 2;
  	}
  //printf("log size = %lu\n", Log_size(log));F
  while(!exitSignal && !fCmd && getInputAndDo(log, stdin, 0)); //don't want to run if already ran command file or error
  //Finishing up:
  if(!exitSignal && histFile) Log_save(log, histFile);
  Log_destroy(log);
  print_command(""); //not sure if necessary, just looks better. Hope no points lost...
  return exitSignal;
}
