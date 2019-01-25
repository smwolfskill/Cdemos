/**
 * Overworked Interns Lab
 * CS 241 - Fall 2016
 */
#include "company.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct rwinfo { //necessary vars. for the reader-writer problem
	pthread_mutex_t m;
	pthread_cond_t rcv; //for readers
	pthread_cond_t wcv; //for writers
	int writers; //# writers waiting to write
	short writing; //boolean
	short reading;
} rwinfo_t;

rwinfo_t * rwinfo_init() {
	rwinfo_t rwinfo = malloc(sizeof(rwinfo_t));
	pthread_mutex_init(rwinfo->m, NULL);
	pthread_cond_init(rwinfo->rcv, NULL);
	pthread_cond_init(rwinfo->wcv, NULL);
	rwinfo->writers = 0;
	rwinfo->writing = 0;
	rwinfo->reading = 0;
}

typedef struct queueNode { //queue of integers
	int data;
	struct queueNode * next;
} queueNode_t;

typedef struct linkedQueue {
	short empty; //bool
	queueNode_t * head;
	queueNode_t * tail;
} queue_t;

//TODO: make this thread safe!! Reader-Writer problem!
queue_t * queue_insert(queue_t * queue, int data) { //insert and malloc to tail of queue. Creates queue if DNE, and 
	queue_t * newQueue = queue;
	if(!queue) { //create queue
		newQueue = malloc(sizeof(queue_t)); 
		newQueue->empty = 0;
	}
	queueNode_t * newNode = malloc(sizeof(queueNode_t));
	newNode->data = data;
	newNode->next = NULL;
	if(!newQueue->head) newQueue->head = newNode;
	if(newQueue->tail) newQueue->tail->next = newNode;
	newQueue->tail = newNode;
	return newQueue;
}

//TODO: make this thread safe!! Reader-Writer problem!
void queue_remove(queue_t * queue) { //removes and frees front of queue
	if(!queue->head->next) { queue->tail = NULL; queue->empty = 1; } //queue now empty
	queueNode_t * newHead = queue->head->next;
	free(queue->head);
	queue->head = newHead;
}

void queue_destroy(queue_t * queue) { //frees all nodes. NOT THREAD SAFE
	//should just be called by the mutexControl fxn.
	queueNode_t * cur = queue->head;
	queueNode_t * next;
	while(cur) {
		next = cur->next;
		free(cur);
		cur = next;
	}
}

typedef struct mutexControl {
	int numThreads; //#threads sharing this intern (need?)
	//pthread_mutex_t m; //controller
	//pthread_cond_t cv;
	pthread_mutex_t * intern; //ptr to intern we're controlling
	queue_t * queue;
	struct mutexControl * next;
} mutexControl_t;

typedef struct linkedMutexControl {
	rwinfo_t * rwinfo; //TODO: need to free at end!
	mutexControl_t * head;
	mutexControl_t * tail;
} mutexControlList_t;

mutexControlList_t * mutexControlList_init() {
	mutexControlList_t * newList = malloc(sizeof(mutexControlList_t));
	rwinfo = rwinfo_init();
	newList->head = NULL;
	newList->tail = NULL;
	return newList;
}


pthread_mutex_t init = PTHREAD_MUTEX_INITIALIZER;
mutexControlList_t * controlList = NULL;
short initialized = 0; //1 if we initialized controlList

//Writer priority
void* safeReader(pthread_mutex_t * m, pthread_cond_t * readercv, pthread_cond_t * writercv, short * reading, int * numWriters, void* (*readFxn)(void*), void* arg) {
	pthread_mutex_lock(m);
	while(*numWriters) pthread_cond_wait(readercv, m);
	*reading = 1;
	pthread_mutex_unlock(m);
	//Now call the reading fxn., readFxn
	void* toReturn = readFxn(arg);
	pthread_mutex_lock(m);
	*reading = 0;
	if(*numWriters) pthread_cond_signal(writercv);
	pthread_mutex_unlock(m);
	return toReturn;
}

//Writer priority
void safeWriter(pthread_mutex_t * m, pthread_cond_t * readercv, pthread_cond_t * writercv, short * reading, short * writing, int * numWriters, void (*writeFxn)(void*), void* arg) {
	pthread_mutex_lock(m);
	(*numWriters)++;
	while(*reading || *writing) pthread_cond_wait(writercv, m);
	*writing = 1;
	pthread_mutex_unlock(m);
	//Now call the writing fxn., writeFxn
	writeFxn(arg);
	pthread_mutex_lock(m);
	*writing = 0;
	(*numWriters)--;
	if(*numWriters) pthread_cond_signal(writercv);
	else pthread_cond_broadcast(readercv);
	pthread_mutex_unlock(m);
}

//Inserts the mutexControl in the tail of the mutexControlList
void insertMutexControl(void * mutexControl) {
	mutexControl_t * control = (mutexControl_t*) mutexControl;
	if(!controlList->head) controlList->head = control;
	if(controlList->tail) controlList->tail->next = control;
	controlList->tail = control;
}

//Returns address of mutexControl that has (intern) as its intern, or NULL if not found.
void * mutexControlExists(void * intern) {
	pthread_mutex_t * intern1 = (pthread_mutex_t*) intern;
	mutexControl_t * cur = controlList->head;
	while(cur) {
		//Compare addresses
		if(cur->intern == intern1) return cur;
		cur = cur->next;
	}
	return NULL; //didn't find it
}

//Either inserts a new mutexControl into the list, or if one corresponding to the intern already exists, return that mutexControl. Think done.
mutexControl_t * mutexControlList_tryInsert(mutexControlList_t * controlList, pthread_mutex_t * intern) {
	//1. Search LL to see if intern is already in it, and if so, return the mutexControl that has it
	mutexControl_t * exists = safeReader(&controlList->m, &controlList->rcv, &controlList->wcv, &controlList->reading, &controlList->writers, mutexControlExists, (void*) intern); //mutexControlExists returns NULL if DNE
	if(exists) return exists;
	//2.1 Doesn't exist already, so create it:
	mutexControl_t * control = malloc(sizeof(mutexControl_t));
	pthread_mutex_init(&control->m, NULL);
	pthread_cond_init(&control->cv, NULL);
	control->intern = intern;
	//2.2 (LL things: add the new mutexControl, set head, tail->next, tail):
	safeWriter(&controlList->m, &controlList->rcv, &controlList->wcv, &controlList->reading, &controlList->writing, &controlList->writers, insertMutexControl, (void*) control);
	return control;
}

//Should NOT allow destruction of mutexControl if queue not empty!
mutexControl_t * mutexControlList_tryRemove(mutexControlList_t * controlList, mutexControl_t * toDestroy) {
	//TODO: destroy (toDestroy) if not already destroyed!
	//to prevent multiple threads from attempting to destroy the same mutexControlList, obtain lock before destroying. Other threads simply return when call destroy (nothing happens).
	pthread_mutex_lock(&toDestroy->m);
	queue_destroy(toDestroy->queue);
	pthread_mutex_unlock(&toDestroy->m);
}

//If frozen in terminal, try CTRL + \  (backslash)
//MAKE SURE not to unlock mutexes that aren't locked! BAD
void *work_interns(void *p) {
	if(!initialized && !pthread_mutex_trylock(&init)) {
		controlList = mutexControlList_init();
		initialized = 1;
		pthread_mutex_unlock(&init);
	}
	Company * company = (Company*) p;
	int id = Company_get_company_number(company);
	pthread_mutex_t * Lintern = ;
    pthread_mutex_t * Rintern = ;
    //TODO: consider not having ref. to L,Rintern and using solely mutexControl.
    //		(would need to add more fxns.)
    mutexControl_t * Lintern_ctrl = mutexControlList_tryInsert(controlList, Company_get_left_intern(company));
    mutexControl_t * Rintern_ctrl = mutexControlList_tryInsert(controlList, Company_get_right_intern(company));
	int notAcquired;
	while(running) {
    	notAcquired = 1;//1 means haven't acquired BOTH interns
		//  Deadlock sol'n: Can't break Mutual Exclusion. Can't break Circular Wait.
		//	  Can't foreseeably break No pre-emption. So MUST BREAK Hold&Wait.
		//		Use pthread_mutex_trylock(): returns 0 if lock is acquired, else
		//		returns other int and doesn't wait.
		//	  So try locking left, and if success, try lock right. 
		//	  If can't lock right, release left.
		//		-This should solve deadlock; I just fear for livelock.
		// Livelock sol'n: waiting queue for fairness
		queue_insert(Lintern_ctrl->queue, id);
		queue_insert(Rintern_ctrl->queue, id);
		//TODO: reader lock
		if(queue_readHead(Lintern_ctrl->queue) == id) {
			//We're in line for the left one
			//PROBLEM: Livelock: since 2 queues for L, R are independent,
			// possibility that we're at front of one queue but not in other! Hold&Wait is bad!
		} else {
		
		}
		
		//
		if(!pthread_mutex_trylock(Lintern)) { //locked left
			notAcquired = pthread_mutex_trylock(Rintern); //put acquired last to prevent compiler optimization not evaulating RHS of || if LHS == 1
			if(notAcquired) {
				pthread_mutex_unlock(Lintern);
				//TODO: to prevent starvation, add to queue? Arbitrator?
				
			}
		}
		
		if(!notAcquired) { //Acquired both interns
			Company_hire_interns(company); //call once locked both interns
			pthread_mutex_unlock(Rintern);
			pthread_mutex_unlock(Lintern);
			
		}
	}
	//TODO: destroy mutexControls
	return NULL;
}
