/**
 * Overworked Interns Lab
 * CS 241 - Fall 2016
 */
#include "company.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

typedef struct rwinfo { //necessary vars. for the reader-writer problem
	pthread_mutex_t m;
	pthread_cond_t rcv; //for readers
	pthread_cond_t wcv; //for writers
	int writers; //# writers waiting to write
	short writing; //boolean
	short reading;
} rwinfo_t;

rwinfo_t * rwinfo_init() {
	rwinfo_t * rwinfo = malloc(sizeof(rwinfo_t));
	pthread_mutex_init(&rwinfo->m, NULL);
	pthread_cond_init(&rwinfo->rcv, NULL);
	pthread_cond_init(&rwinfo->wcv, NULL);
	rwinfo->writers = 0;
	rwinfo->writing = 0;
	rwinfo->reading = 0;
	return rwinfo;
}

//Writer priority
void* safeReader(rwinfo_t * rwinfo, void* (*readFxn)(void*), void* arg) {
	pthread_mutex_lock(&rwinfo->m);
	while(rwinfo->writers) pthread_cond_wait(&rwinfo->rcv, &rwinfo->m);
	rwinfo->reading = 1;
	pthread_mutex_unlock(&rwinfo->m);
	//Now call the reading fxn., readFxn
	void* toReturn = readFxn(arg);
	pthread_mutex_lock(&rwinfo->m);
	rwinfo->reading = 0;
	if(rwinfo->writers) pthread_cond_signal(&rwinfo->wcv);
	pthread_mutex_unlock(&rwinfo->m);
	return toReturn;
}

//Writer priority
void safeWriter(rwinfo_t * rwinfo, void (*writeFxn)(void*), void* arg) {
	pthread_mutex_lock(&rwinfo->m);
	rwinfo->writers++;
	while(rwinfo->reading || rwinfo->writing) pthread_cond_wait(&rwinfo->wcv, &rwinfo->m);
	rwinfo->writing = 1;
	pthread_mutex_unlock(&rwinfo->m);
	//Now call the writing fxn., writeFxn
	writeFxn(arg);
	pthread_mutex_lock(&rwinfo->m);
	rwinfo->writing = 0;
	rwinfo->writers--;
	if(rwinfo->writers) pthread_cond_signal(&rwinfo->wcv);
	else pthread_cond_broadcast(&rwinfo->rcv);
	pthread_mutex_unlock(&rwinfo->m);
}



typedef struct mutexControl {
	size_t id;
	pthread_mutex_t * intern;
	struct mutexControl * next;
} mutexControl_t;

int mutexControl_lock(mutexControl_t * toLock) { return pthread_mutex_lock(toLock->intern); }

int mutexControl_trylock(mutexControl_t * toTryLock) { return pthread_mutex_trylock(toTryLock->intern); }

int mutexControl_unlock(mutexControl_t * toUnlock) { return pthread_mutex_unlock(toUnlock->intern); }

typedef struct mutexControlList {
	rwinfo_t * rwinfo;
	mutexControl_t * head;
	mutexControl_t * tail;
	size_t count; //# elements in list
} mutexControlList_t;

volatile short initialized = 0;
pthread_mutex_t init = PTHREAD_MUTEX_INITIALIZER;
mutexControlList_t * list = NULL;

mutexControlList_t * mutexControlList_init() { //NOT thread safe!
	mutexControlList_t * newList = malloc(sizeof(mutexControlList_t));
	newList->rwinfo = rwinfo_init();
	newList->head = NULL;
	newList->tail = NULL;
	newList->count = 0;
	return newList;
}

void mutexControlList_destroy(mutexControlList_t * list) { //NOT thread safe!
	free(list->rwinfo);
	mutexControl_t * cur = list->head;
	mutexControl_t * next;
	while(cur) {
		next = cur->next;
		free(cur);
		cur = next;
	}
	free(list);
}

//Inserts the mutexControl in the tail of the mutexControlList
void insertMutexControl(void * mutexControl) {
	//NOT thread safe; use via safeWriter
	mutexControl_t * control = (mutexControl_t*) mutexControl;
	control->id = list->count;
	list->count++;
	control->next = NULL;
	if(!list->head) list->head = control;
	if(list->tail) list->tail->next = control;
	list->tail = control;
}

//Returns address of mutexControl that has (intern) as its intern, or NULL if not found.
void * mutexControlExists(void * intern) {
	//NOT thread safe; use via safeReader
	pthread_mutex_t * intern1 = (pthread_mutex_t*) intern;
	mutexControl_t * cur = list->head;
	while(cur) {
		//Compare addresses
		if(cur->intern == intern1) return cur;
		cur = cur->next;
	}
	return NULL; //didn't find it
}

//Inserts a mutex (intern) into the list if it's not already in the list.
// else, returns the mutexControl already containing the mutex.
mutexControl_t * mutexControlList_tryInsert(mutexControlList_t * list, pthread_mutex_t * intern) {
	//1. Search LL to see if intern is already in it, and if so, return the mutexControl that has it
	mutexControl_t * exists = safeReader(list->rwinfo, mutexControlExists, (void*) intern); //mutexControlExists returns NULL if DNE
	if(exists) return exists;
	//2.1 Doesn't exist already, so create it:
	mutexControl_t * control = malloc(sizeof(mutexControl_t));
	control->intern = intern;
	//2.2 (LL things: set id, count, add the mutexControl, set head, tail->next, tail):
	safeWriter(list->rwinfo, insertMutexControl, (void*) control);
	return control;
}

//simple struct to help with the organization process
typedef struct companyInterns {
	mutexControl_t * lower;
	mutexControl_t * higher;
} companyInterns_t;

companyInterns_t * companyInterns_init(pthread_mutex_t * lIntern, pthread_mutex_t * rIntern) {
	companyInterns_t * newInterns = malloc(sizeof(companyInterns_t));
	newInterns->lower = mutexControlList_tryInsert(list, lIntern);
	newInterns->higher = mutexControlList_tryInsert(list, rIntern);
	if(newInterns->lower->id > newInterns->higher->id) {
		//Want the mutexControl w/ lowest id as lowerIntern
		mutexControl_t * temp = newInterns->lower;
		newInterns->lower = newInterns->higher;
		newInterns->higher = temp;
	}
	return newInterns;
}

//If frozen in terminal, try CTRL + \  (backslash)
//MAKE SURE not to unlock mutexes that aren't locked! BAD
void *work_interns(void *p) {
	if(!initialized && !pthread_mutex_trylock(&init)) {
		list = mutexControlList_init();
		initialized = 1;
		pthread_mutex_unlock(&init);
	}
	Company * company = (Company*) p;
	companyInterns_t * myInterns = companyInterns_init(Company_get_left_intern(company), Company_get_right_intern(company));
	//printf("Company %d w/ interns %ld, %ld\n", Company_get_company_number(company), myInterns->lower->id, myInterns->higher->id);
	while(running) {
		//Using dijkstra's soln: break circular wait:
		// obtain resources in order (we assigned every intern a #)
		mutexControl_lock(myInterns->lower);
		mutexControl_lock(myInterns->higher);
		Company_hire_interns(company);
		mutexControl_unlock(myInterns->lower);
		mutexControl_unlock(myInterns->higher);
	}
	//Finishing up:
	//TODO: Need barrier for when to destroy? Eh, no(?) In case where a thread is done before another has even started, the one to start can just re-init
	if(initialized && !pthread_mutex_trylock(&init)) {
		mutexControlList_destroy(list);
		initialized = 0;
		pthread_mutex_unlock(&init);
	}
	/*pthread_mutex_lock(&init);
	printf("Company %d: done!\n", Company_get_company_number(company));
	pthread_mutex_unlock(&init);*/
	//pthread_exit(NULL);
	return NULL;
}
