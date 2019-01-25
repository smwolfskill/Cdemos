/**
 * Luscious Locks Lab 
 * CS 241 - Fall 2016
 */
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "queue.h"
queue_t * myQueue = NULL;
pthread_t tid1, tid2;

void * action1(void * data) {
	int * intData = (int*) data;
	int i = 0;
	while(i < 4) {
		printf("tid1: Pushing data[%d]=%d...\n", i, intData[i]);
		queue_push(myQueue, &intData[i]); //wait forever when i==3 if no thread pulled
		printf("\ttid1: Finished pushing data[%d]\n", i);
		i++;
	}
	return NULL;
}

void * action2(void * data) {
	printf("tid2: sleeping for 5!\n");
	sleep(5);
	printf("tid2: woke up!\n");
	printf("tid2: pulled data w/ value %d\n", *((int*)queue_pull(myQueue)));
	return NULL;
}


int main(int argc, char **argv) {
	//Test 1: 2 threads. TRY put all of data into it, and destroy it. Check leaks.
	//	if CPU only has one core, then if tid1 is executed first, should wait forever
	int * data = malloc(4 * sizeof(int));
	data[0] = 10; data[1] = 11; data[2] = 12; data[3] = 13;
	myQueue = queue_create(3); //not big enough for everything
	pthread_create(&tid1, NULL, action1, data);
	pthread_create(&tid2, NULL, action2, NULL);
	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);
	queue_destroy(myQueue);
	free(data);
	//
  return 0;
}
