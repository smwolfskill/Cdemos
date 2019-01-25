/**
 * Parallel Map Lab
 * CS 241 - Fall 2016
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "map.h"
#include "mappers.h"

/* You should create a struct that will get passed in by reference to your
 * start_routine. */
typedef struct threadArgs {
	double * list; //the entire list passed in by ref
	size_t start; //inclusive
	size_t end; //EXCLUSIVE, i.e. while(++start < end)
	mapper map_func;
} threadArgs_t;

threadArgs_t * threadArgs_create(double * list, size_t start, size_t end, mapper map_func) { //ctor
	threadArgs_t * newThreadArgs = malloc(sizeof(threadArgs_t));
	newThreadArgs->list = list;
	newThreadArgs->start = start;
	newThreadArgs->end = end;
	newThreadArgs->map_func = map_func;
	return newThreadArgs;
}

/* You should create a start routine for your threads. */
void * action(void * arg) { //arg should be (threadArgs_t *)
	threadArgs_t * input = (threadArgs_t *) arg;
	while(input->start < input->end) { //start inclusive, end exclusive
		input->list[input->start] = input->map_func(input->list[input->start]);
		input->start++; //assume don't need to retain this for later use!!
	}
	return (void*) NULL; 
	//we do all the work on the list here; no return val necessary
}


double *par_map(double *list, size_t list_len, mapper map_func,
                size_t num_threads) {
  /* Your implementation goes here */
	/* Think of some of the edge cases: 
	 *   More threads than elements and the # of elements 
	 *   not being multiple of # of threads. */
	if(num_threads == 0) return NULL; //bad arg
	double * ret_list = malloc(list_len * sizeof(double));
	size_t i = 0;
	//1. Copy list into ret_list (we'll modify ret_list directly as we go)
	while(i < list_len) {
		ret_list[i] = list[i];
		i++;
	}
	//2. Create the threads. They'll do the work
	pthread_t * ids = malloc(num_threads * sizeof(pthread_t));
	threadArgs_t ** args = malloc(num_threads * sizeof(threadArgs_t*));
	int quotient = list_len / num_threads; //assume truncate, ex: 1/2 = 0
	i = 0;
	while(i < num_threads) { //NEED: MAPPING OF START, END !!
		size_t end = (i + 1) * quotient;
		if(i == num_threads - 1) end += list_len - (quotient * num_threads); //for the last thread: if there was a remainder for quotient, assign to work on it as well
		args[i] = threadArgs_create(ret_list, i * quotient, end, map_func);
		pthread_create(&ids[i], NULL/*notsure*/, action, (void*) args[i]);
		i++;
	}
	//3. Now wait for them to be done
	i = 0;
	while(i < num_threads) {
		void * result; //should be NULL
		pthread_join(ids[i], &result);
		i++;
	}
	//4. Free dynamic mem. we used (other than ret_list):
	free(ids);
	i = 0;
	while(i < num_threads) {
		free(args[i]);
		i++;
	}
	free(args);
	//5. Done
	return ret_list; //user will have to free ret_list
}
