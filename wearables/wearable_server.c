/**
 * Machine Problem: Wearables
 * CS 241 - Fall 2016
 */

#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

#include "utils.h"
#include "vector.h"
#include "wearable.h"

#define T (0) //for testing; indicates whether to print

// The wearable server socket, which all wearables connect to.
int wearable_server_fd;
volatile int done = 0;

typedef struct threadData {
  pthread_t thread;
  int fd;
  long timestamp;
  short running; //0 means done; exited
  //timestamp ts;
  // TODO you might want to put more things here
  pthread_mutex_t m;
  pthread_cond_t cv;
  struct threadData * next;
} thread_data;

bool selector1(timestamp_entry * data) {
	return !strcmp( ((SampleData*)data->data)->type_, TYPE1); }
bool selector2(timestamp_entry * data) {
	return !strcmp( ((SampleData*)data->data)->type_, TYPE2); }
bool selector3(timestamp_entry * data) {
	return !strcmp( ((SampleData*)data->data)->type_, TYPE3); }
void timestamp_entry_dtor(void* elem) {
	if(elem) {
		timestamp_entry * this = (timestamp_entry*)elem;	
		SampleData * sd = (SampleData*)(this->data);
		free(sd->type_);
		free(sd);
		free(this);
	}
}

typedef struct {
	Vector * vec;
	pthread_mutex_t m;
	pthread_cond_t rcv;
	pthread_cond_t wcv;
	pthread_cond_t datacv; //for waiting in certain data circumstances. see search().
	short reading;
	short writing;
	int readers;
	int writers;
} SafeVector;

SafeVector * SafeVector_create() {
	SafeVector * newVec = malloc(sizeof(SafeVector));
	newVec->vec = Vector_create(timestamp_entry_copy_constructor, timestamp_entry_dtor); //use my dtor b/c theirs is useless. fuck course staff
	pthread_mutex_init(&newVec->m, NULL);
	pthread_cond_init(&newVec->rcv, NULL);
	pthread_cond_init(&newVec->wcv, NULL);
	pthread_cond_init(&newVec->datacv, NULL);
	newVec->reading = 0;
	newVec->writing = 0;
	newVec->readers = 0;
	newVec->writers = 0;
	return newVec;
}

void SafeVector_destroy(SafeVector * vec) { //destroys (vec->vec) and frees (vec).
	if(!vec) return;
	if(vec->vec) {
		if(T) printf("Vector had %lu elements.\n", Vector_size(vec->vec));
		Vector_destroy(vec->vec);
		/*size_t len = Vector_size(vec->vec);
		size_t i = 0;
		while(i < len) {
			(timestamp_entry*)Vector_get(vec->vec, i));
			i++;
		}*/
	}
	free(vec);
}

void SafeVector_append(SafeVector * vec, timestamp_entry * data) {
	pthread_mutex_lock(&vec->m);
	vec->writers++;
	while(vec->reading || vec->writing) {
		pthread_cond_wait(&vec->wcv, &vec->m);
	}
	vec->writing = 1;
	pthread_mutex_unlock(&vec->m);
	//critical sect. here:
	Vector_append(vec->vec, (void*)data);
	//(end)
	pthread_mutex_lock(&vec->m); //cleanup:
	vec->writers--;
	vec->writing = 0;
	if(vec->writers) pthread_cond_signal(&vec->wcv); //writer wakeup priority
	else pthread_cond_broadcast(&vec->rcv); //no writers, so readers can go
	pthread_mutex_unlock(&vec->m);
}

//(sizes) is an already-allocated array of at least 3 integers; this sets [0..2].
//returns a 3xn 2D array where each sub-array corresponds to the types.
//allocates 3 pointers for the 1st array, and gather_timestamps allocates the 3 sub-arrays.
timestamp_entry ** SafeVector_search(SafeVector * vec, timestamp start, timestamp end/*, bool (*selector)(timestamp_entry *)*/, int * sizes) {
	timestamp_entry ** listByType = NULL;
	pthread_mutex_lock(&vec->m);
	vec->readers++;
	while(vec->writers) { //used to be writing. Now writer has all priority
		pthread_cond_wait(&vec->rcv, &vec->m);
	}
	/*while(vec->newest < end || !Vector_size(vec->vec)) {
		if(done) { pthread_mutex_unlock(&vec->m); sizes[0] = -1; return NULL; } //received SIGINT, AND not met condition. So quit.
		pthread_cond_wait(&vec->datacv, &vec->m);
	}*/
	vec->reading = 1;
	pthread_mutex_unlock(&vec->m);
	//Critical sect. here:
	//  First, check to ensure that have a timestamp >= (end)
	/*size_t i = 0;
	size_t len = Vector_size(vec->vec);
	while(i < len) {
		if( ((timestamp_entry*)Vector_get(vec->vec, i))->time >= end )
			break; //found
		i++;
	}*/
	//Found; so gather elements in order of type.
	if(T) printf("Request: vector currently has size %lu\n", Vector_size(vec->vec));
	listByType = malloc(3*sizeof(timestamp_entry *));
	ssize_t res = gather_timestamps(vec->vec, start, end, selector1, &listByType[0]);
	sizes[0] = (int) res;
	res = gather_timestamps(vec->vec, start, end, selector2, &listByType[1]);
	sizes[1] = (int) res;
	res = gather_timestamps(vec->vec, start, end, selector3, &listByType[2]);
	sizes[2] = (int) res;
	//(end)
	pthread_mutex_lock(&vec->m); //cleanup
	vec->readers--;
	vec->reading = 0;
	pthread_mutex_unlock(&vec->m);
	pthread_cond_signal(&vec->wcv); //writer might be waiting
	return listByType;
}

//thread_data ** wearable_threads;
thread_data * wearable_thread_head = NULL;
thread_data * wearable_thread_tail = NULL;
pthread_mutex_t tdataMtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t tdataCv = PTHREAD_COND_INITIALIZER;
int wearable_threads_size = 0;
SafeVector * data = NULL;
pthread_t * request_thread = NULL;
int request_socket = -1;
int request_server_fd = -1;
FILE * debugOutput = NULL;


//checks all wearable threads' thread_data to ensure their latest timestamps >= (end).
//If not blocks, unless the thread isn't running (it finished), OR process SIGINT (so done == 1)
void thread_data_wait_timestamps(long end) {
	pthread_mutex_lock(&tdataMtx);
	while(!wearable_thread_head) { //wait until there exists a wearable thread.
		if(done) { pthread_mutex_unlock(&tdataMtx); return; }
		printf("Request: waiting for wearable threads to be created (none exist)...\n");
		pthread_cond_wait(&tdataCv, &tdataMtx);
	}
	thread_data * cur = wearable_thread_head;
	pthread_mutex_unlock(&tdataMtx);
	while(cur) { //check & wait (if nec.) for them in order.
		short printed = 0;
		pthread_mutex_lock(&cur->m);
		while(cur->timestamp < end) {
			if(!cur->running || done) { pthread_mutex_unlock(&cur->m); break; }
			if(T && !printed) { printf("Request: waiting for wearable on %d...\n", cur->fd); printed = 1; } //prevent spam messages for same wearable
			pthread_cond_wait(&cur->cv, &cur->m);
		}
		pthread_mutex_unlock(&cur->m);
		pthread_mutex_lock(&tdataMtx);
		cur = cur->next;
		pthread_mutex_unlock(&tdataMtx);
	}
	return;
}

void thread_data_joinAll() { //mine. wait to join & free all.
	thread_data * cur = wearable_thread_head;
	thread_data * next;
	pthread_mutex_lock(&tdataMtx);
	while(cur) {
		next = cur->next;
		shutdown(cur->fd, SHUT_RDWR);
		close(cur->fd);
		pthread_join(cur->thread, NULL);
		pthread_cond_signal(&cur->cv); //request thread may be in deadlock otherwise
		free(cur);
		cur = next;
	}
	pthread_mutex_unlock(&tdataMtx);
	if(T) write(1, "All wearable threads exited.\n", 29);
	pthread_cond_signal(&tdataCv);
	wearable_thread_head = NULL;
	wearable_thread_tail = NULL;
	
}

void signal_received(int sig) {
  // close server socket, free anything you don't free in main
	if(sig == SIGINT) {
		if(T) write(1, "SIGINT received.\n", 17);
		done = 1;
		close(wearable_server_fd);
		thread_data_joinAll();
		//Cleanup anything we've allocated.
		if(request_thread) {
			pthread_cond_signal(&data->datacv);
			pthread_join(*request_thread, NULL);
			free(request_thread); 
		} else {
			if(request_server_fd != -1) close(request_server_fd);
			if(request_socket != -1) { 
				shutdown(request_socket, SHUT_RDWR);
				close(request_socket);
			}
		}
		SafeVector_destroy(data);
		if(T) { 
			if(debugOutput) fclose(debugOutput);
			write(1, "Handler: done.\n", 15);
		}
		exit(0);
	}
}

void *wearable_processor_thread(void *args) { //for new data (we write). Think done
  thread_data *td = (thread_data *)args;
  int socketfd = td->fd;
  if(T) printf("Wearable: started w/ fd %d\n", socketfd);
  // read data from the socket. <timestamp>:<value>:<type>.
  // Assume 64-byte packets.
	char line[64];
	ssize_t res = read(socketfd, line, 64);
	while(/*res != -1*/ res > 0 && !done) { //TODO: should we wait or? What if they're still going to send data, but it takes time?
		if(res > 0) {
			long stamp = 0;
			SampleData * sdata;
			extract_key(line, &stamp, &sdata);
			if(T) fprintf(debugOutput, "Wearable on %d: received '%ld:%d:%s'\n", socketfd, stamp, sdata->data_, sdata->type_);
			timestamp_entry * newData = malloc(sizeof(timestamp_entry));
			newData->time = (timestamp) stamp;
			if(td->timestamp < stamp) { //have newer timestamp
				pthread_mutex_lock(&td->m);
				td->timestamp = stamp;
				pthread_mutex_unlock(&td->m);
				pthread_cond_signal(&td->cv); //request thread may be waiting
			}
			newData->data = (void*) sdata;
			SafeVector_append(data, newData);
		}
		res = read(socketfd, line, 64);
	}
	pthread_mutex_lock(&td->m);
	td->running = 0;
	pthread_mutex_unlock(&td->m);
	pthread_cond_signal(&td->cv); //request thread may be waiting. Think if a thread exits, then even if it hasn't received an old enough timestamp, still send its data in the interval anyway.
  close(socketfd);
  if(T) { printf("Wearable connection on fd %d closed.\n", socketfd); }
  return NULL;
}

void *user_request_thread(void *args) { //for requests (we read). TODO!
  //int socketfd = *((int *)args);
  int socketfd = (int)((intptr_t)args);
  if(T) printf("Request: started w/ fd %d\n", socketfd);
  // Read data from the socket. Reqs in form "<timestamp1>:<timestamp2>"
  // Write out statistics for data between those timestamp ranges
	//1. Read timestamp1 and timestamp2 as two strings separated by a ':'
	int * sizes = calloc(3, sizeof(int));
	char * strStart = calloc(1024, 1);
	char * strEnd = calloc(1024, 1);
	int start = 0; //Inclusive
	int end = 0; //Exclusive
	int i;
	ssize_t res = 1;
	while(!done && res > 0) {
		i = 0;
		/*pthread_mutex_lock(&threadCountMtx);
		while(!wearable_threads_size) //
			pthread_cond_wait(&threadCountCv, &threadCountMtx);
		pthread_mutex_unlock(&threadCountMtx);*/
		res = read(socketfd, strStart, 64);
	/*ssize_t res = read(socketfd, strStart, 1);
	while(res) {
		i++;
		if(strStart[i] == ':') {
			strStart[i] = '\0'; //read start
		}
		res = read(socketfd, strStart + i, 1);
	}
	if(T) printf("\tRequest: word 1: read %d chars\n", i);
	i = 0;
	res = read(socketfd, strEnd, 1);
	while(res) { //assume only 1 incoming request at a time
		i++;
		res = read(socketfd, strEnd + i, 1);
	}
	printf("\tRequest: word 2: read %d chars\n", i);
	strEnd[i+1] = '\0'; //read end
	//2. Parse the strings into integers
	start = 0;
	end = 0;
	sscanf(strStart, "%d", &start);
	sscanf(strEnd, "%d", &end);*/
		if(res > 0) {
			sscanf(strStart, "%d:%d", &start, &end);
			if(T) printf("Request '%s:%s' = [%d, %d) received\n", strStart, strEnd, start, end);
			//3. Check, and wait if don't have newest data yet
			// If any active wearable connection has not received data up to this end timestamp, you must wait until that wearable thread receives data that has a timestamp >= the end timestamp before replying the client’s request. 
			//    Furthermore, if a wearable thread has finished sending data and closed its connection, you still need to include its data in your reply. You will need to use some sort of synchronization to ensure that you do not send out the data too early.
			thread_data_wait_timestamps((long)end);
			//if(T) printf("Request: search failed; >=1 wearable thread doesn't have data up to %d\n", end);
			//break;
			timestamp_entry ** listByType = SafeVector_search(data, (timestamp)start, (timestamp)end, sizes);
			if(T) printf("Request: finished search.\n");
			//4. Search the list and write results
			if(sizes[0] == -1 || sizes[1] == -1 || sizes[2] == -1) { if(T) fprintf(stderr, "gather_timestamps failed!\n"); break; }
			write_results(socketfd, TYPE1, listByType[0], sizes[0]);	
			write_results(socketfd, TYPE2, listByType[1], sizes[1]);
			write_results(socketfd, TYPE3, listByType[2], sizes[2]);
			write(socketfd, "\r\n", 2); //signifies end of message apparently
			if(T) {
				printf("=======OUTPUTTING=======>>\n");
				/*write_results(1, TYPE1, listByType[0], sizes[0]);	
				write_results(1, TYPE2, listByType[1], sizes[1]);
				write_results(1, TYPE3, listByType[2], sizes[2]);
				printf("====================\n");*/
			}
			free(listByType); //free just 1st array of 3 elements (not sub-arrays)
		} else if(T) printf("Request: read failed %ld!\n", res);
	}
	shutdown(socketfd, SHUT_RDWR);
  close(socketfd);
  free(sizes);
  free(strStart);
  free(strEnd);
  if(T) printf("Request server connection on fd %d closed.\n", socketfd);
  return NULL;
}

//Mine: Create new thread_data as well as start the thread on wearable_processor_thread
thread_data * thread_data_create(int fd) {
	thread_data * newtdata = malloc(sizeof(thread_data));
	newtdata->fd = fd;
	newtdata->next = NULL;
	newtdata->timestamp = 0L;
	newtdata->running = 1;
	pthread_mutex_init(&newtdata->m, NULL);
	pthread_cond_init(&newtdata->cv, NULL);
	//LL things:
	pthread_mutex_lock(&tdataMtx);
	if(!wearable_thread_head) wearable_thread_head = newtdata;
	if(wearable_thread_tail) wearable_thread_tail->next = newtdata;
	wearable_thread_tail = newtdata;
	pthread_mutex_unlock(&tdataMtx);
	pthread_cond_signal(&tdataCv); //request thread may be waiting if this is first wearable created
	pthread_create(&newtdata->thread, NULL, wearable_processor_thread, (void*)newtdata);
	//pthread_mutex_lock(&threadCountMtx);
	wearable_threads_size++;
	//pthread_mutex_unlock(&threadCountMtx);
	//pthread_cond_signal(&threadCountCv);
	return newtdata;
}

int open_server_socket(const char *port) { //think done
  // given a string with the port value, set up a passive socket file
  // descriptor and return it
	errno = 0;
	struct addrinfo hints, * result;
	memset(&hints, 0, sizeof(hints)); //make sure no garbage
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	int g = getaddrinfo(NULL, port, &hints, &result);
	if(g != 0) { fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(g)); exit(1); }
  /*QUESTION 1, 2, 3*/
	int sockFd = socket(result->ai_family, result->ai_socktype, IPPROTO_TCP); //create endpoint for IPv4 TCP conn.
	if(sockFd == -1) { perror(NULL); exit(1); }
	int optval = 1;
    if(setsockopt(sockFd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) != 0) { perror(NULL); exit(1); } //want port to be reusable instantaneously
	if(bind(sockFd, result->ai_addr, result->ai_addrlen) != 0) { perror(NULL); exit(1); }
	freeaddrinfo(result);
  return sockFd;
}

int wearable_server(const char *wearable_port, const char *request_port) {
  // setup signal handler for SIGINT
	errno = 0;
	signal(SIGINT, signal_received);
  request_server_fd = open_server_socket(request_port);
  wearable_server_fd = open_server_socket(wearable_port);
	if(T) printf("(req_server_fd, wear_server_fd) = (%d, %d)\n", request_server_fd, wearable_server_fd);
	if(listen(request_server_fd, 1) != 0) { perror(NULL); exit(1); }
	if(listen(wearable_server_fd, 128) != 0) { perror(NULL); exit(1); } //TODO:128 right?
  //pthread_t request_thread;
  request_socket = accept(request_server_fd, NULL, NULL);
  if(T) printf("MAIN: Accepted request server on fd %d.\n", request_socket);
  request_thread = malloc(sizeof(pthread_t));
  pthread_create(request_thread, NULL, user_request_thread, (void*)((intptr_t)request_socket));
  close(request_server_fd);
	
	data = SafeVector_create();
	if(T) { debugOutput = fopen("wearablesInput.txt", "w+"); }
	while(!done) {
		// accept continous requests on the wearable port
		//if(T) printf("MAIN: Waiting for new wearable connection...\n");
		//TODO: problem: accept blocks! How make so signal handler works w/this?
		int fd = accept(wearable_server_fd, NULL, NULL); //TODO NULL right?
		if(fd == -1) { perror(NULL); exit(1); }
		if(T) printf("MAIN: Accepted new wearable conn. on fd %d. (#%d)\n", fd, wearable_threads_size + 1);
		thread_data_create(fd);
	}
	if(T) printf("MAIN:b Done!\n");
  // join all threads we spawned from the wearables.
	thread_data_joinAll();
  // Cleanup anything we've allocated.
  pthread_join(*request_thread, NULL);
	SafeVector_destroy(data);
  return 0;
}
