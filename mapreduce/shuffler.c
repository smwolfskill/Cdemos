/**
 * MapReduce
 * CS 241 - Fall 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "utils.h"

void usage() {
  fprintf(stderr, "shuffler destination1 destination2 ...\n");
  fprintf(stderr, "where destination1..n are files on the filesystem to which "
                  "the shuffler will write its output\n");
}

int main(int argc, char *argv[]) {
  // read from stdin
  // hash the key for the input line
  // send them to the correct output file (output files are given as command
  // line arguments
  if (argc < 2) {
    usage();
    exit(1);
  }
	//First, open all output files:
	int reducer_count = argc - 1;
	FILE * outputFiles[reducer_count];
	int i = 0;
	while(i < reducer_count) {
		outputFiles[i] = fopen(argv[i+1], "w");
		if(!outputFiles[i]) {
			fprintf(stderr, "shuffler: failed to open '%s'! Done.\n", argv[i+1]);
			return 1;
		}
		i++;
	}
	//Shuffler work
	char * key; //assume won't be larger than this.
	char * value;
	while(scanf("%ms", &key) != EOF) {
		key[strlen(key)-1] = '\0'; //to get rid of annoying ':'
		scanf("%ms", &value);
		printf("\tshuffler: (key, value) = (%s, %s)\n", key, value);
		FILE * outf = outputFiles[ hashKey(key) % reducer_count ];
		fprintf(outf, "%s: %s\n", key, value);
		free(key);
		free(value);
	}
	//Cleanup:
	i = 0;
	while(i < reducer_count) {
		fclose(outputFiles[i]);
		i++;
	}
	//printf("shuffler: done!\n");
	return 0;
}
