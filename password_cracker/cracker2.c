/**
 * Machine Problem: Password Cracker
 * CS 241 - Fall 2016
 */

#include "cracker2.h"
#include "format.h"
#include "utils.h"
#include "thread_status.h" //For thread debugging
#include <stdint.h> //for intptr_t
#include <pthread.h>
#include <crypt.h> //for crypt_r
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

//Like safeWriter, but if there is already another writer, does nothing and returns (void*) -1
void * safeTryWriter(rwinfo_t * rwinfo, void* (*writeFxn)(void*), void* arg) {
	pthread_mutex_lock(&rwinfo->m);
	if(rwinfo->writers) {
		pthread_mutex_unlock(&rwinfo->m);
		return (void*) ((intptr_t) -1);
	}
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

rwinfo_t * rwinfo = NULL;
int done = 0; //1 if done with all tasks
size_t numThreads = 0;
pthread_barrier_t barrier_read; //for making sure no worker thread starts a new task until main has read and parsed one, and if DNE, writes signal (done) for workers to quit 
pthread_barrier_t barrier_sync; //for making sure no worker thread continues until all have finished their partition of the work
//pthread_mutex_t m_success = PTHREAD_MUTEX_INITIALIZER; //for use by worker threads if successfully cracked //OLD
char * username = NULL;
char * hash = NULL;
char * guess = NULL;
int guessIndex = 0;
int guessLen = 0;
double * CPUTimes = NULL; //total CPU time devoted to cracking current for each thread
int * hashCounts = NULL; //array of hashCounts for each thread
int result = 1;//0 for success, 1 for fail.0 also indicates done w/ cur task
char * pass = NULL; //correct pass

void getCPUTimeAndHashCount(double * CPUTimeResult, int * hashCountResult, size_t thread_count) { //sum CPUTimes and hashCounts arrays and put results in ptr args.
	*CPUTimeResult = 0.0;
	*hashCountResult = 0;
	size_t i = 0;
	while(i < thread_count) {
		*CPUTimeResult += CPUTimes[i];
		*hashCountResult += hashCounts[i];
		i++;
	}
}

void initCPUTimesAndHashCounts(size_t thread_count) { //set all to 0. Must be malloc'd first!
	size_t i = 0;
	while(i < thread_count) {
		CPUTimes[i] = 0.0;
		hashCounts[i] = 0;
		i++;
	}
}

//Returns 1st index in which the char (toFind) appears. -1 means not found.
int indexOf(char * toSearch, char toFind) {
	int i = 0;
	while(toSearch[i]) {
		if(toSearch[i] == toFind) return i;
		i++;
	}
	return -1; //not found
}

void* readDone(void* isNull) { //NEEDS to be used thread-safely! Use safeReader
	isNull = isNull;
	return (void*) ((intptr_t) done);
}

void* readResult(void* isNull) { //NEEDS to be used thread-safely! Use safeReader
	isNull = isNull;
	return (void*) ((intptr_t) result);
}

void* setDone1(void* isNull) { //NEEDS to be used thread-safely! Use safeWriter
	isNull = isNull;
	done = 1;
	return NULL;
}

void* setResult0(void* isNull) { //NEEDS to be used thread-safely! Use safeWriter
	isNull = isNull;
	result = 0;
	return NULL;
}

//Returns 0 if found, 1 if exited early b/c other thread found, or 2 if not found.
//count is how many iterations this should try. Sets it to num iterations we did
int checkChar(int threadId, char * myGuess, int guessIndex, long * count, struct crypt_data * cdata) {
	long i = 0;
	int earlyStopWeird = 0; //if 241 admin's fxns work, should never be 1
	int thisThreadFound = 0;
	//Yes, race condition to loop and read result again before successful
	//	thread sets result=0, but worst case is just run another iteration.
	//	 I think that's faster than always using safeReader
	//	  which must lock and unlock a mutex twice per call.
	while(/*result && */!earlyStopWeird && i < *count) {
		if( ((int)((intptr_t)safeReader(rwinfo,readResult,NULL)))==0) break;
		earlyStopWeird = !incrementString(myGuess+guessIndex) && i >= *count;
		char * cryptResult = crypt_r((const char*) myGuess, "xx", cdata);
		if(/*result && */!strcmp(cryptResult, hash)) { //success!
			//printf("\tThread %d: found it at i = %ld!\n", threadId, i);
			if( ((int)((intptr_t) safeTryWriter(rwinfo, setResult0, NULL))) != -1) { //NEW
				thisThreadFound = 1;
				free(pass);
				pass = strdup(myGuess);
			} //END NEW
			//OLD:
			/*if(pthread_mutex_trylock(&m_success) == 0) {
				//Hmm...but shouldn't need a mutex b/c only 1 thread should
				// be able to find correct pass, right?
				//printf("\t\tThread %d: locked success mutex\n", threadId);
				if(result) {
					//printf("\t\t\tThread %d: set result to 0, etc.\n", threadId);
					result = 0; //success. Signal for other threads to stop 
					thisThreadFound = 1;
					free(pass);
					pass = strdup(myGuess);
				}
				pthread_mutex_unlock(&m_success);
			} //else if can't lock, already success
			//END OLD*/
		}
		i++;
	}
	//printf("\t\tThread %d: checkChar stopped at i = %ld\n", threadId, i);
	if(earlyStopWeird) fprintf(stderr, "Thread %d: ERROR/weird: incremented myGuess all the way at %ld/%ld", threadId, i, *count);
	*count = i; //set it to num iterations/hashes we ran
	if(result) return 2; //no thread found it (best case: yet, worst case: at all)
	return !thisThreadFound; //0 if this thread found, 1 if another thread found
}

void* crack(void* args) {
	threadStatusSet("initializing...");
	int id = (int) ((intptr_t) args);
	long start = 0L;
	long count = 0L;
	double tStart, tEnd;
	char * myGuess = NULL;
	struct crypt_data * cdata = malloc(sizeof(struct crypt_data));
	cdata->initialized = 0;
	//When a thread starts processing a task, it should print v2_print_thread_start(int threadId, char *username, long offset, char *startPassword)
	while(1) {
		threadStatusSet("getting next task...");
		pthread_barrier_wait(&barrier_read); //wait for main to have another task ready
		//if((int) ((intptr_t)safeReader(rwinfo,readDone,NULL)) ) break; //don't think need safeReader below b/c main can't change done until all worker threads done w/ cur task.
		if(done) break;
		//int done = (int) ((intptr_t) safeReader(rwinfo, readDone, NULL););
		tStart = getThreadCPUTime();
		myGuess = strdup(guess);
		getSubrange(guessLen-guessIndex, numThreads, id, &start, &count);
		setStringPosition(myGuess + guessIndex, start);
		v2_print_thread_start(id, username, start, myGuess);
		//printf("guess = %s; myGuess = %s; (guessIndex = %d)\n", guess, myGuess, guessIndex);
		threadStatusSet("cracking...");
		int result = checkChar(id, myGuess, guessIndex, &count, cdata);
		//printf("\tThread %d: myGuess = %s; result = %d\n", id, myGuess, result);
		hashCounts[id-1] = (int) count;
		v2_print_thread_result(id, (int) count, result);
		free(myGuess);
		tEnd = getThreadCPUTime();
		CPUTimes[id-1] = tEnd-tStart;
		threadStatusSet("waiting for all threads to sync...");
		pthread_barrier_wait(&barrier_sync); //wait for all worker threads to be done w/ cur task
	}
	threadStatusSet("done!");
	free(cdata);
	return NULL; //don't think we need ret. val this time
}

int start(size_t thread_count) {
	rwinfo = rwinfo_init();
	pthread_barrier_init(&barrier_read, NULL, thread_count+1); //barrier for all worker threads and main
	pthread_barrier_init(&barrier_sync, NULL, thread_count+1); //barrier for all worker threads and main
	
	numThreads = thread_count;
  	pthread_t tids[thread_count];
  	CPUTimes = malloc(thread_count * sizeof(double));
  	hashCounts = malloc(thread_count * sizeof(int));
  	initCPUTimesAndHashCounts(thread_count);
	for(int i = 0; i < (int) thread_count; i++) {
		pthread_create(&tids[i], NULL, crack, (void*) ((intptr_t) (i+1)));
	}
	//2. Now read tasks:
	short start = 1;
	size_t capacity = 0;
	ssize_t glResult = 1;
	//char * save;
	double tStart = 0.0;/*, CPUStart;*/
	double CPUTime = 0.0;
	int hashCount = 0;
	while(glResult > 0) {
		char * buf = NULL;
  		glResult = getline(&buf, &capacity, stdin);
  		if(glResult > 0 && buf[glResult-1]=='\n') {
  			buf[glResult-1] = '\0'; //replace terminating ‘\n’ with ‘\0’ which is much more useful
  			char * save = buf;
			char * newUsername = strtok_r(save, " ", &save);
			char * newHash = strtok_r(save, " ", &save);
			char * newGuess = strtok_r(save, " ", &save);
			//printf("MAIN:\t = %s;%s;%s\n&newTask = %p\n", newTask->username, newTask->hash, newTask->guess, newTask);
			int newGuessIndex = indexOf(newGuess, '.');
			int newGuessLen = strlen(newGuess);
  			if(!start) { //don't do on 1st iteration
  				pthread_barrier_wait(&barrier_sync); //don't assign globals for next task until all worker threads done w/ cur task global data (maybe copy it?). BUT, don't wait on 1st iteration or deadlock!
  				double tEnd = getTime();
  				//double CPUEnd = getCPUTime();
  				//TODO: Don't need below?? (or assoc. vars)
  				getCPUTimeAndHashCount(&CPUTime,&hashCount,thread_count);
  				//printf("MAIN: result = %d\n", result);
  				//TODO: Fix time!!!
  				v2_print_summary(username, pass, hashCount, tEnd-tStart,/*CPUEnd-CPUStart*/CPUTime, result);
  				free(pass);
  				pass = NULL;
            }
  			v2_print_start_user(newUsername);
  			free(username);
  			result = 1;
  			initCPUTimesAndHashCounts(thread_count);
  			username = newUsername;
  			hash = newHash;
  			guess = newGuess;
  			guessIndex = newGuessIndex;
  			guessLen = newGuessLen;
  			pthread_barrier_wait(&barrier_read); //all workers may now read next task
  			tStart = getTime();
  			//CPUStart = getCPUTime();
  		} else {
  			//done = 1; //don't think need below b/c ONLY main will be executing here; workers are waiting on barrier_read. Additionally, main can't change this UNTIL all threads are done w/ cur task
  			//safeWriter(rwinfo, setDone, NULL);
  			if(!start) {
  				pthread_barrier_wait(&barrier_sync); //so workers can continue to read exit signal
  				done = 1;
  				double tEnd = getTime();
  				//double CPUEnd = getCPUTime();
  				getCPUTimeAndHashCount(&CPUTime,&hashCount,thread_count);
  				//TODO: Fix time!!!
  				v2_print_summary(username, pass, hashCount, tEnd-tStart,/*CPUEnd-CPUStart*/CPUTime, result);
  				free(username);
  				free(pass);
  				free(buf);
  				pass = NULL;
  				pthread_barrier_wait(&barrier_read); //all worker threads may now read exit signal
  			} else { //1st iteration
  				done = 1;
  				free(buf);
  				pthread_barrier_wait(&barrier_read); //all worker threads may now read exit signal
  				//pthread_barrier_wait(&barrier_task); //since workers break and DON'T call barrier, we would deadlock
  			}
  		}
  		start = 0;
  	}
	//After all worker threads finish each task, the main thread will print the password (if found), the total number of hashes, the wall clock and CPU time spent on that task, and the ratio of CPU time to wall clock time. Note that we have not provided any of the timing print statements in cracker2.
	for(size_t i = 0; i < thread_count; i++) {
		pthread_join(tids[i], NULL);
	}
	free(rwinfo);
	free(CPUTimes);
	free(hashCounts);
	return 0;
}
