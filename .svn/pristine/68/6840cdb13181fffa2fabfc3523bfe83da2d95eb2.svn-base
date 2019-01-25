/**
 * Luscious Locks Lab 
 * CS 241 - Fall 2016
 */
#include "semamore.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //for sleep
#include <pthread.h>

Semamore * s;

void * action1(void * args) {
	puts("tid1: removing a cookie...");
	semm_wait(s);
	puts("tid1: did it! Removing 2nd cookie...");
	semm_wait(s);
	puts("tid1: did it! Removing 3rd cookie...");
	semm_wait(s);
	puts("tid1: done!");
	return NULL;
}

void * action2(void * args) {
	puts("tid2: sleeping for 5!");
	sleep(5);
	puts("tid2: woke up! Adding a cookie...");
	semm_post(s);
	puts("tid2: done!");
	return NULL;
}

int main(int argc, char **argv) {
	s = malloc(sizeof(Semamore));
	semm_init(s, 2, 2);
  //Test 1: thread waits
  	pthread_t tid1, tid2;
  	pthread_create(&tid1, NULL, action1, NULL);
  	pthread_create(&tid2, NULL, action2, NULL);
  	pthread_join(tid1, NULL);
  	pthread_join(tid2, NULL);
  	semm_destroy(s);
  	//Test 2, 3: should wait forever (semm_post)
  	puts("=====================\nTEST2: tid1, tid2 should wait forever! CTRL-C to exit");
  	semm_init(s, 0, 0);
  	pthread_create(&tid2, NULL, action2, NULL);
  	//Test 3: should wait forever (semm_wait)
  	pthread_create(&tid1, NULL, action1, NULL);
  	pthread_join(tid1, NULL);
  	pthread_join(tid2, NULL);
  	//
  	semm_destroy(s);
  	free(s);
  return 0;
}
