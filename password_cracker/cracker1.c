/**
 * Machine Problem: Password Cracker
 * CS 241 - Fall 2016
 */
#include "cracker1.h"
#include "format.h"
#include "utils.h"
#include "thread_status.h" //For thread debugging
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <crypt.h> //for crypt_r
#include <stdlib.h>
#include <stdio.h>

//Allows for thread-safe reading and writing.
// One must pass in the non-thread safe reading and writing functions.
typedef struct rwinfo { //necessary vars. for the reader-writer problem
	pthread_mutex_t m;
	pthread_cond_t rcv; //for readers
	pthread_cond_t wcv; //for writers
	int writers; //# writers waiting to write
	int readers; //# readers reading
	short writing; //boolean
	short reading;
} rwinfo_t;

rwinfo_t * rwinfo_init() {
	rwinfo_t * rwinfo = malloc(sizeof(rwinfo_t));
	pthread_mutex_init(&rwinfo->m, NULL);
	pthread_cond_init(&rwinfo->rcv, NULL);
	pthread_cond_init(&rwinfo->wcv, NULL);
	rwinfo->writers = 0;
	rwinfo->readers = 0;
	rwinfo->writing = 0;
	rwinfo->reading = 0;
	return rwinfo;
}

//Writer priority
void* safeReader(rwinfo_t * rwinfo, void* (*readFxn)(void*), void* arg) {
	pthread_mutex_lock(&rwinfo->m);
	while(rwinfo->writers) pthread_cond_wait(&rwinfo->rcv, &rwinfo->m);
	rwinfo->reading = 1;
	rwinfo->readers++;
	pthread_mutex_unlock(&rwinfo->m);
	//Now call the reading fxn., readFxn
	void* toReturn = readFxn(arg);
	pthread_mutex_lock(&rwinfo->m);
	rwinfo->reading = 0;
	rwinfo->readers--;
	if(rwinfo->writers) pthread_cond_signal(&rwinfo->wcv);
	pthread_mutex_unlock(&rwinfo->m);
	return toReturn;
}

//Writer priority
void * safeWriter(rwinfo_t * rwinfo, void* (*writeFxn)(void*), void* arg) {
	pthread_mutex_lock(&rwinfo->m);
	rwinfo->writers++;
	while(rwinfo->reading || rwinfo->writing) pthread_cond_wait(&rwinfo->wcv, &rwinfo->m);
	rwinfo->writing = 1;
	pthread_mutex_unlock(&rwinfo->m);
	//Now call the writing fxn., writeFxn
	void* toReturn = writeFxn(arg);
	pthread_mutex_lock(&rwinfo->m);
	rwinfo->writing = 0;
	rwinfo->writers--;
	if(rwinfo->writers) pthread_cond_signal(&rwinfo->wcv);
	else pthread_cond_broadcast(&rwinfo->rcv);
	pthread_mutex_unlock(&rwinfo->m);
	return toReturn;
}

typedef struct task {
	char * username, * hash, * guess;
	int guessIndex;
} task_t;

typedef struct queueNode {
	void * data;
	struct queueNode * next;
} queueNode_t;

//List of which threads have finished so that we can join w/ main thread
typedef struct queue {
	short empty; //1 if empty, 0 if not
	pthread_mutex_t m;
	pthread_cond_t cv;
	queueNode_t * head;
	queueNode_t * tail;
} queue_t;

queue_t * queue_init() {
	queue_t * newQueue = malloc(sizeof(queue_t));
	newQueue->empty = 1; //true
	pthread_mutex_init(&newQueue->m, NULL);
	pthread_cond_init(&newQueue->cv, NULL);
	newQueue->head = NULL;
	newQueue->tail = NULL;
	return newQueue;
}

//NOT thread safe--call only from main thread! Frees all nodes and the queue itself. Does NOT free nodes' data variables!
void queue_destroy(queue_t * queue) {
	queueNode_t * cur = queue->head;
	queueNode_t * next;
	while(cur != NULL) {
		next = cur->next;
		free(cur);
		cur = next;	
	}
	free(queue);
}

//Should be used sparingly -- very easy to cause a double free
void queue_freeData(queue_t * queue) {
	queueNode_t * cur = queue->head;
	while(cur != NULL) {
		free(cur->data);
		cur = cur->next;	
	}
}

void queue_print(queue_t * list) { //TEMP. Only for queues w/ task_t* data!
	puts("--------------------------------");
	printf("queue_print: Printing all nodes of taskList...\n");
	int i = 0;
	pthread_mutex_lock(&list->m);
	queueNode_t * cur = list->head;
	while(cur) {
		task_t * task = (task_t*) cur->data;
		printf("Node %d: %s; %s; %s\n", i, task->username, task->hash, task->guess);
		cur = cur->next;
		i++;
	}
	pthread_mutex_unlock(&list->m);
	puts("--------------------------------");
}

//Add a node(data) to the queue. (Should call right before a thread returns)
void queue_push(queue_t * queue, void * data) {
	queueNode_t * newNode = malloc(sizeof(queueNode_t));
	newNode->data = data;
	newNode->next = NULL;
	//printf("queue_push: &data = %p. BEFORE: ", data); //TEMP
	//queue_print(queue); //TEMP
	pthread_mutex_lock(&queue->m);
	if(!queue->head) queue->head = newNode; //new queue
	if(queue->tail) queue->tail->next = newNode;
	queue->tail = newNode;
	queue->empty = 0; //not empty anymore
	pthread_mutex_unlock(&queue->m);
	//printf("queue_push: AFTER: "); //TEMP
	//queue_print(queue);
	pthread_cond_broadcast(&queue->cv);
}

//Removes the head/front of the queue and returns data of the queueNode_t
void * queue_remove(queue_t * queue) {
	void * data;
	queueNode_t * newHead = NULL;
	pthread_mutex_lock(&queue->m);
	while(queue->empty == 1) {
		//passive wait until woken up when list not empty
		pthread_cond_wait(&queue->cv, &queue->m);
	}
	data = queue->head->data;
	newHead = queue->head->next;
	free(queue->head);
	queue->head = newHead;
	if(!newHead) {
		queue->tail = NULL; //removed last node in list
		queue->empty = 1;
	}
	pthread_mutex_unlock(&queue->m);
	return data;
}

int remaining = 0; //#passwords remaining that need to decrypt
//pthread_mutex_t lineMtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gettingTasks = PTHREAD_MUTEX_INITIALIZER; //for our purposes, a boolean that can't have race conditions.
pthread_cond_t gettingTasks_cv = PTHREAD_COND_INITIALIZER; //same as above
queue_t * doneList = NULL;
queue_t * taskQueue = NULL;
rwinfo_t * rwinfo = NULL;

//Recursively evaluates all possible passwords. (only lowercase chars a..z)
//Returns NULL if incorrect guess, or (guess) if the guess was correct
char * checkChar(char * hashed, char * guess, int index, int passLen, struct crypt_data * cdata, int * attempts) {
	/*static unsigned int count = 0;
	count++;
	if(count % 100000 == 0) printf("\tcheckChar: called w/ index = %d; passLen = %d\n", index, passLen);*/
	if(index == passLen) { //base case: have a filled-in guess
		char * result = crypt_r((const char*) guess, "xx", cdata);
		//crypt_r returns based on allocated cdata, so don't need to free
		(*attempts)++;
		if(!strcmp(result, hashed)) {
			//printf("\tcheckChar: match! pass = %s\n", guess);
			return result; //success!
		}
		return NULL; //incorrect guess
	}
	for(int i = 0; i < 26; i++) { //recursive case: not done filling in our guesses
		guess[index] = (char) (((int) 'a') + i);
		char * attempt = checkChar(hashed, guess, index+1, passLen, cdata, attempts);
		if(attempt != NULL) return attempt;
		//else, keep going
	}
	//printf("\tcheckChar: nothing matched!!\n");
	return NULL; //somehow, nothing we tried was correct (bug?)
}

//Thread-safe call to read the next line of input from (fd), and return it.
/*char * nextLine(FILE * fd) {
	char * buf = NULL;
	size_t capacity = 0;
	ssize_t result = 1;
	pthread_mutex_lock(&lineMtx);
	result = getline(&buf, &capacity, fd);
	pthread_mutex_unlock(&lineMtx);
	if(result>0 && buf[result-1]=='\n') {
		buf[result-1] = '\0'; //replace terminating ‘\n’ with ‘\0’ which is much more useful
		numPasswords++;
	} else {
		free(buf);
		return NULL;
	}
	return buf;
}*/

//Returns 1st index in which the char (toFind) appears. -1 means not found.
int indexOf(char * toSearch, char toFind) {
	int i = 0;
	while(toSearch[i]) {
		if(toSearch[i] == toFind) return i;
		i++;
	}
	return -1; //not found
}

void* incRemaining(void * remainingInc) {
	int inc = (int) ((intptr_t) remainingInc);
	remaining += inc;
	if(inc > 1) pthread_cond_broadcast(&gettingTasks_cv);
	else pthread_cond_signal(&gettingTasks_cv);
	return NULL;
}

//Pass to it pointer of rwinfo which is the same rwinfo
//  passed into safeReader
void* maybeWait(void* rwinfo_v) {
	rwinfo_t * rwinfo = (rwinfo_t*) rwinfo_v;
	if(remaining == 0) {
		//pthread_mutex_t dummy;
		//pthread_mutex_init(&dummy, NULL);
		while(remaining == 0) {
			if(pthread_mutex_trylock(&gettingTasks) != 0) {
				//MAIN is getting more tasks. Wait:
				pthread_mutex_lock(&rwinfo->m);
				rwinfo->readers--;
				if(rwinfo->readers == 0) rwinfo->reading = 0;
				pthread_cond_wait(&gettingTasks_cv, &rwinfo->m);
				rwinfo->reading = 1;
				rwinfo->readers++;
				pthread_mutex_unlock(&rwinfo->m);
			} else {
				pthread_mutex_unlock(&gettingTasks); //didn't really want to lock it, just wanted to see if MAIN was still getting tasks. If not and remaining == 0, we quit.
				break;
			}
		}
	}
	return NULL;
}

//Must ONLY call this w/ safeWriter!
// Call safeReader w/ maybeWait beforehand too!
void* nextTask(void * isNull) {
	isNull = isNull; //for compiler to shut up
	if(remaining > 0) {
		remaining--;
		//printf("nextTask: now remaining = %d\nBEFORE remove:", remaining);
		//queue_print(taskQueue);
		return queue_remove(taskQueue);
	}
	return NULL;
	//return (void*) ((intptr_t) remaining);
}

//Work done by thread. Returns # successful decryptions (recovered)
void * crack(void * args) {
	threadStatusSet("initializing...");
	int id = (int) ((intptr_t) args);
	int recovered = 0; //successful decryptions
	/*char * username;
	char * hash;
	char * guess;
	int startIndex = 0; //index of first unknown char of guess*/
	int attempts = 0;
	threadStatusSet("getting next task...");
	//printf("\tThread %d: getting next task...\n", id);
	//char * line = nextLine(stdin);
	//task_t * task = (task_t*) queue_remove(taskQueue); //NEW
	task_t * task; //newER
	struct crypt_data * cdata = malloc(sizeof(struct crypt_data));
	cdata->initialized = 0;
	safeReader(rwinfo,maybeWait,rwinfo);
	while( (task = (task_t*)safeWriter(rwinfo,nextTask,NULL)) != NULL){
		//queue_print(taskQueue);
		//printf("\t\t\tThread %d: got it!\n", id);
		attempts = 0;
		//safeWriter(rwinfo, decRemaining, NULL);
		/*char * save = line;
		username = strtok_r(save, " ", &save);
		hash = strtok_r(save, " ", &save);
		guess = strtok_r(save, " ", &save);
		startIndex = indexOf(guess, '.');
		//printf("Thread %d: user = %s; hash = %s; guess = %s; startIndex = %d\n", id, username, hash, guess, startIndex);*/
		if(task->guessIndex == -1) { fprintf(stderr, "Thread %d: guess did not contain '.'! Quitting\n", id); break;}
		v1_print_thread_start(id, task->username);
		threadStatusSet("cracking...");
		//printf("\tThread %d: cracking...\n", id);
		double t0 = getThreadCPUTime();
		char * result = checkChar(task->hash, task->guess, task->guessIndex, strlen(task->guess), cdata, &attempts);
		double tDone = getThreadCPUTime();
		if(result != NULL) recovered++;
		v1_print_thread_result(id, task->username, result, attempts, tDone-t0, (result ? 0 : 1));
		//free(line);
		free(task->username);
		free(task); //NEW
		threadStatusSet("getting next task...");
		//printf("\t\tThread %d: getting next line...\n", id);
		//line = nextLine(stdin);
		//task = (task_t*) queue_remove(taskQueue); //NEW
		safeReader(rwinfo,maybeWait,rwinfo); //newER
	}
	threadStatusSet("done!");
	free(cdata);
	int * idPtr = malloc(sizeof(int));
	*idPtr = id;
	queue_push(doneList, idPtr);
	//printf("Thread %d: done!\n", id);
	return (void*) ((intptr_t) recovered);
}

int start(size_t thread_count) { //input piped in to stdin!
	//thread_count = 2; //TODO: TEMP! Get rid of when parallelize!!!
	doneList = queue_init();
	taskQueue = queue_init();
	rwinfo = rwinfo_init();
  int numRecovered = 0;
  pthread_t tids[thread_count];
  int recovered[thread_count];
  //remaining = 1; //so no race cond. for worker threads to try to read before main thread has read and assigned any tasks
  pthread_mutex_lock(&gettingTasks);
  for(size_t i = 0; i < thread_count; i++) {
  	pthread_create(&tids[i], NULL, crack, (void*) (intptr_t) (i+1));
  }
  //2. Now add tasks to the queue:
  int numPasswords = 0;
  size_t capacity = 0;
  ssize_t result = 1;
  while(result > 0) {
  	char * buf = NULL;
  	result = getline(&buf, &capacity, stdin);
  	if(result > 0 && buf[result-1]=='\n') {
  		buf[result-1] = '\0'; //replace terminating ‘\n’ with ‘\0’ which is much more useful
  			//printf("MAIN: read %s\n", buf);
  			numPasswords++;
  			/*if(numPasswords > 1)*/ safeWriter(rwinfo, incRemaining, (void*) ((intptr_t) (1)));
			task_t * newTask = malloc(sizeof(task_t));
			char * save = strdup(buf);
			newTask->username = strtok_r(save, " ", &save);
			newTask->hash = strtok_r(save, " ", &save);
			newTask->guess = strtok_r(save, " ", &save);
			//printf("MAIN:\t = %s;%s;%s\n&newTask = %p\n", newTask->username, newTask->hash, newTask->guess, newTask);
			newTask->guessIndex = indexOf(newTask->guess, '.');
			queue_push(taskQueue, (void*) newTask);
			//queue_print(taskQueue);
		}
		free(buf);
	}
	//safeWriter(rwinfo, incRemaining, (void*) ((intptr_t) (numPasswords-1)));
	pthread_mutex_unlock(&gettingTasks);
	//printf("MAIN: Updated remaining (all tasks pushed).\n");
	//3. Wait to join with threads:
  /*for(size_t i = 0; i < thread_count; i++) {
  	void * ret;
  	pthread_join(tids[i], &ret);
  	recovered[i] = (int) ((intptr_t) ret);
  	numRecovered += recovered[i];
  	printf("tid %ld returned w/ val. %d\n", i+1, recovered[i]);
  }*/
  size_t joined = 0; //#threads that are done and we've joined with
  while(joined < thread_count) {
  	void * ret;
  	int * idPtr = queue_remove(doneList);
  	pthread_join(tids[*idPtr-1], &ret);
  	recovered[*idPtr-1] = (int) ((intptr_t) ret);
  	numRecovered += recovered[*idPtr-1];
  	//printf("tid %d returned w/ val. %d\n", *idPtr, recovered[*idPtr-1]);
  	free(idPtr);
  	joined++;
  }
  //free(buf); //don't need?
  free(rwinfo);
  queue_destroy(doneList);
  queue_freeData(taskQueue); //free user data first
  queue_destroy(taskQueue); //(b/c doesn't free user data)
  v1_print_summary(numRecovered, numPasswords - numRecovered);
  return 0;
}