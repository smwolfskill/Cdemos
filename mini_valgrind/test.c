/**
 * Mini Valgrind Lab
 * CS 241 - Fall 2016
 */

#include "mini_valgrind.h"
#include <stdio.h>
#include <stdlib.h>

#include <sys/wait.h> //for waitpid
#include <unistd.h> //for exec
#include <stdlib.h> //for exit

void printTest(int testNum) {
	printf("======================\nTEST %d:\n", testNum);
}

void printUsages() {
	printf("total_usage = %lu\ntotal_free = %lu\nbad_frees = %lu\n", total_usage, total_free, bad_frees);
}

void resetGlobals() { //reset global vars. in mini_valgrind
	total_usage = 0;
	total_free = 0;
	bad_frees = 0;
}

void theirPrintReport() {
	print_report();
	pid_t child = fork();
	if(!child) { //i'm child
		printf("\nPrinting contents of result.txt...\n");
		execlp("cat", "cat", "result.txt", (const char*)NULL);
		fprintf(stderr, "exec failed!\n");
		exit(1);
	}
	int status = 0;
	waitpid(child, &status, 0);
	if(!WIFEXITED(status) || WEXITSTATUS(status) != 0) exit(1);
}

int main() {
  // your tests here using malloc and free
  	int * hi;
  	//Test 1: detect totally lost: not enough frees
  	printTest(1);
	hi = malloc(3);
	hi = malloc(4); //totally lost 3 bytes
	*hi = 4;
	//printf("\tBEFORE free(): & = %p\n", hi);
	free(hi);
	printUsages();
	theirPrintReport(); //weird. outputs to result.txt. see print.c
	resetGlobals();
	//Test 2: detect double free
	printTest(2);
	hi = malloc(5);
	free(hi);
	free(hi);
	printUsages();
	resetGlobals();
	//Test 3: try filename > max ?
	//
  // Do NOT modify this line
  atexit(print_report);
  return 0;
}
