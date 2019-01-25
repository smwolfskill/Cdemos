/**
 * Parallel Make
 * CS 241 - Fall 2016
 */
#include <unistd.h> //for getopt
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h> //for stat
#include <sys/stat.h> //..
#include <unistd.h> //..
#include <time.h> //for difftime
#include <stdint.h> //for intptr_t (only for debug)
#include "parmake.h"
#include "parser.h"
#include "queue.h"

 //to prevent race conditions w/ r/w rule->state, and reader-writer problems
typedef struct myRule {
	rule_t * rule;
	Vector * myDependencies; //NULL if circular dependency; state will be -1
	size_t numDependencies;
	struct stat * info;
	pthread_mutex_t m;
	pthread_cond_t cv;
	short writing; //boolean
	int readers;
	struct myRule * next;
} myRule_t;

myRule_t * ruleHead = NULL;
myRule_t * ruleTail = NULL;

void* myRule_copy_ctor(void* source) { //shallow copy
	/*myRule_t * src = (myRule_t*) source;
	myRule_t * copy = malloc(sizeof(myRule_t));
	rule_t * ruleCpy = malloc(sizeof(rule_t));
	rule_soft_copy(ruleCpy, src->rule);
	copy->myDependencies = src->myDependencies;
	copy->numDependencies = src->numDependencies;
	copy->info = src->info;
	copy->writing = src->writing;
	copy->readers = src->readers;
	pthread_mutex_init(&copy->m, NULL);
	pthread_cond_init(&copy->cv, NULL);
	return (void*) copy;*/
	return source;
}

void myRule_dtor(void* toFree) { //don't free dependencies!
	/*myRule_t * myRule = (myRule_t*) toFree;
	rule_destroy(myRule->rule);
	free(myRule->info);
	free(myRule);*/
	toFree = toFree;
}

//NOT Thread-safe! Shouldn't need to be!
myRule_t * myRule_create(rule_t * rule) {
	//1. Check to see if we've already created a myRule for this rule
	myRule_t * cur = ruleHead;
	while(cur) {
		if(cur->rule == rule) return cur; //Yes; found it
		cur = cur->next;
	}
	//2. Either list is empty or we haven't created it yet. Create it
	//printf("\tmyRule_create(): malloc'ing...\n");
	myRule_t * myRule = malloc(sizeof(myRule_t));
	myRule->rule = rule;
	myRule->info = NULL; //allocate before use!
	pthread_mutex_init(&myRule->m, NULL);
	pthread_cond_init(&myRule->cv, NULL);
	myRule->writing = 0;
	myRule->readers = 0;
	myRule->next = NULL;
	//2.1 Create/find dependencies
	size_t size = Vector_size(rule->dependencies);
	myRule->numDependencies = size;
	myRule->myDependencies = Vector_create(myRule_copy_ctor, myRule_dtor);
	Vector_resize(myRule->myDependencies, size);
	size_t i = 0;
	while(i < size) {
		rule_t* cur = (rule_t*) Vector_get(rule->dependencies, i);
		Vector_set(myRule->myDependencies, i, (void*)myRule_create(cur));
		i++;
	}
	//printf("\tmyRule_create(): set myDependencies vector. Finishing up...\n");
	//3. LL things
	if(!ruleHead) ruleHead = myRule;
	if(ruleTail) ruleTail->next = myRule;
	ruleTail = myRule;
	return myRule;
}

void myRule_freeList() {
	myRule_t * cur = ruleHead;
	myRule_t * next;
	while(cur) {
		next = cur->next;
		rule_destroy(cur->rule);
		free(cur->rule);
		free(cur->info);
		if(cur->myDependencies) Vector_destroy(cur->myDependencies);
		free(cur);
		cur = next;
	}
}

//(cycleHead) is the myRule_t that we're comparing against to see if it has 
//a dependency cycle, and (toCheck) is the current dependency of (cycleHead) 
//that we're checking
/*short dependencyCycleChecker(myRule_t * cycleHead, myRule_t * toCheck) {
	//Recursively determines if there is a dependency cycle, and returns 1
	//if so. Otherwise, returns 0.
	if(toCheck->numDependencies == 0) return 0; //base case 1: no dependencies (so no cycle)
	myRule_t * cur;
	size_t i = 0;
	short foundCycle = 0;
	while(i < toCheck->numDependencies) {
		cur = (myRule_t*) Vector_get(toCheck->myDependencies, i);
		if(cur == cycleHead) return 1; //found a circular dependency!!
		// Else, check the current dependency of (toCheck) against a 
		// cycleHead of (toCheck), and if that doesn't detect a cycle,
		// check the cur. dependency against (cycleHead).
		foundCycle = dependencyCycleChecker(toCheck, cur);
		if(!foundCycle && cycleHead != toCheck) //2nd part is to make sure don't check same stuff twice
			foundCycle = dependencyCycleChecker(cycleHead, cur);
		else break;
		i++;
	}
	return foundCycle;
}*/

short dependencyCycleChecker(rule_t * cycleHead, rule_t * toCheck) {
	//Recursively determines if there is a dependency cycle, and returns 1
	//if so. Otherwise, returns 0.
	if(toCheck->state == -1) return 1; //base case 0: we already detected a cycle leading out of/involving (toCheck) previously
	size_t numDependencies = Vector_size(toCheck->dependencies);
	if(numDependencies == 0) return 0; //base case 1: no dependencies (so no cycle)
	rule_t * cur;
	size_t i = 0;
	short foundCycle = 0;
	while(i < numDependencies) {
		cur = (rule_t*) Vector_get(toCheck->dependencies, i);
		if(cur == cycleHead) return 1; //found a circular dependency!!
		/* Else, check the current dependency of (toCheck) against a 
		 * cycleHead of (toCheck), and if that doesn't detect a cycle,
		 * check the cur. dependency against (cycleHead).  */
		foundCycle = dependencyCycleChecker(toCheck, cur);
		if(!foundCycle && cycleHead != toCheck) //2nd part is to make sure don't check same stuff twice
			foundCycle = dependencyCycleChecker(cycleHead, cur);
		else break;
		i++;
	}
	return foundCycle;
}

//TODO: Don't need?
/*short myRule_allSatisfied() { //thread-safe
	myRule_t * cur = ruleHead;
	short toReturn = 1;
	while(cur) {
		pthread_mutex_lock(&cur->m);
		if(cur->writing || cur->readers) toReturn = 0; 
		pthread_mutex_unlock(&cur->m);
		if(cur->rule->state == 0) toReturn = 0;
		if(!toReturn) break;
		cur = cur->next;
	}
	return toReturn;
}*/

/*typedef struct myQueue { //expanded upon queue_t; like inheritance
	queue_t * queue;
	size_t items; //# items currently has. DO NOT r/w directly!
	short doneInsert; //boolean for if we're done inserting to queue; all items have been added
	pthread_mutex_t m;
} myQueue_t;

myQueue_t * myQueue_create(void *(*copy_constructor_type)(void *), void (*destructor_type)(void *)) {
	myQueue_t * newQueue = malloc(sizeof(myQueue_t));
	newQueue->queue = queue_create(0, copy_constructor_type, destructor_type); //assume maxItems=0 means no limit
	newQueue->items = 0;
	newQueue->doneInsert = 0;
	pthread_mutex_init(&newQueue->m, NULL);
	return newQueue;
}

//Will pull ONLY if items > 0. 
//  If items == 0 and doneInsert == 1, returns NULL.
//	  This is to prevent infinite waiting/blocking on regular queue_t pull.
//	Else, wait to pull.
//	  This is regular queue behavior; want wait to pull if haven't added
//	  everything to the queue yet.
void* myQueue_tryPull(myQueue_t * myQueue) {
	//keep locked whole time to prevent race conditions
	pthread_mutex_lock(&myQueue->m);
	if(myQueue->items == 0 && myQueue->doneInsert) {
		pthread_mutex_unlock(&myQueue->m);
		return NULL; //no items left and done inserting; do nothing
	}
	void * data = queue_pull(myQueue->queue);
	myQueue->items--;
	pthread_mutex_unlock(&myQueue->m);
	return data;
}

//Call w/ doneInsert=0 unless know it's the last thing to insert to queue!
void myQueue_push(myQueue_t * myQueue, void* data, short doneInsert) {
	queue_push(myQueue->queue, data);
	if(doneInsert) {
		pthread_mutex_lock(&myQueue->m);
		myQueue->doneInsert = doneInsert;
		pthread_mutex_unlock(&myQueue->m);
	}
}

//Destroys the inner queue and frees (myQueue)
void myQueue_destroy(myQueue_t * myQueue) {
	queue_destroy(myQueue->queue);
	free(myQueue);
}*/

typedef struct queueNode {
	void* data;
	struct queueNode * next;
} queueNode_t;

typedef struct myQueue { //NEW: different from original above
	queueNode_t * head;
	queueNode_t * tail;
	pthread_mutex_t m;
	pthread_cond_t cv;
} myQueue_t;

myQueue_t * myQueue_create() {
	myQueue_t * newQueue = malloc(sizeof(myQueue_t));
	newQueue->head = NULL;
	newQueue->tail = NULL;
	pthread_mutex_init(&newQueue->m, NULL);
	pthread_cond_init(&newQueue->cv, NULL);
	return newQueue;
}

void myQueue_destroy(myQueue_t * myQueue) {
	queueNode_t * cur = myQueue->head;
	queueNode_t * next;
	while(cur) {
		next = cur->next;
		free(cur);
		cur = next;
	}
	free(myQueue);
}

void myQueue_push(myQueue_t * myQueue, void* data) {
	queueNode_t * newNode = malloc(sizeof(queueNode_t));
	newNode->data = data;
	newNode->next = NULL;
	pthread_mutex_lock(&myQueue->m);
	if(!myQueue->head) myQueue->head = newNode;
	if(myQueue->tail) myQueue->tail->next = newNode;
	myQueue->tail = newNode;
	pthread_mutex_unlock(&myQueue->m);
	pthread_cond_signal(&myQueue->cv);
}

void* myQueue_pull(myQueue_t * myQueue) {
	void* data;
	pthread_mutex_lock(&myQueue->m);
	while(!myQueue->head) pthread_cond_wait(&myQueue->cv, &myQueue->m);
	queueNode_t * front = myQueue->head;
	myQueue->head = myQueue->head->next;
	if(!myQueue->head) myQueue->tail = NULL; //queue empty again
	pthread_mutex_unlock(&myQueue->m);
	data = front->data;
	free(front);
	return data;
}

void myQueue_print(myQueue_t * myQueue) {
	printf("\tListing myQueue...\n");
	int i = 0;
	pthread_mutex_lock(&myQueue->m);
	queueNode_t * cur = myQueue->head;
	while(cur) {
		printf("\t\tNode %d: '%s'\n", i, ((myRule_t*)cur->data)->rule->target);
		i++;
		cur = cur->next;
	}
	pthread_mutex_unlock(&myQueue->m);
}

int numThreads = 1; //default for opt. -j
char* makefile = "./makefile"; //default for opt. -f. If DNE, try "./Makefile"
char** targets = NULL; //last option
int numTargets = 0;
myQueue_t * queue = NULL;
volatile int rulesRemaining = 0;
volatile int possibleQueues = 0;
pthread_mutex_t rulesRemMtx = PTHREAD_MUTEX_INITIALIZER;
//pthread_barrier_t barrier;
pthread_cond_t rulesRemCv = PTHREAD_COND_INITIALIZER;


int stringToInt(char* str) { //only works for positive ints, but that's all we need
	int len = strlen(str);
	int i = len - 1;
	int mul = 1;
	int num = 0;
	while(i >= 0) {
		num += mul * ((int) str[i] - (int) '0');
		i--;
		mul *= 10;
	}
	return num;
}

//SINGLE-THREADED! Whew!
void myParser(rule_t * rule) {
	//Dependencies: pointers to actual rules! Very good; not just copies!
	//DEBUG
	/*size_t size = Vector_size(rule->dependencies);
	size_t i = 0;
	printf("myParser: rule = %s @ %p; %ld dependencies\n", rule->target, rule, size);
	while(i < size) {
		void* cur = Vector_get(rule->dependencies, i);
		printf("\tdependencies[%lu] = %s @ %p\n", i, ((rule_t*)cur)->target, cur);
		i++;
	}*/
	//END DEBUG
	rulesRemaining++;
	myRule_t * myRule;
	if(dependencyCycleChecker(rule, rule)) {
		//fprintf(stderr, "myParser: rule '%s': detected circular dependency! Satisfied, FAILED\n", rule->target);
		rule->state = -1;
		myRule = malloc(sizeof(myRule_t));
		myRule->rule = rule;
		myRule->myDependencies = NULL;
		myRule->numDependencies = 0;
		myRule->info = NULL;
		pthread_mutex_init(&myRule->m, NULL);
		pthread_cond_init(&myRule->cv, NULL);
		myRule->writing = 0;
		myRule->readers = 0;
		if(!ruleHead) ruleHead = myRule;
		if(ruleTail) ruleTail->next = myRule;
		ruleTail = myRule;
	} else {
		//printf("myParser: creating/fetching myRule for '%s'...\n", rule->target);
		myRule = myRule_create(rule);
		//printf("myParser: created/fetched myRule success!\n");
	}
	myQueue_push(queue, (void*) myRule);
	//printf("myParser: pushed rule '%s' onto queue...\n", rule->target);
}

void* work(void* arg) { //worker thread
	arg = arg; //for stupid compiler
	//We'll never have duplicate rules (and if our code works, never have duplicate myRule_t's).
	//A rule can be run IFF:
	// 1.1 All rules it depends on have been satisfied
	//		Mark only once we've run all rule's commands
	// 1.2 None of them have failed
	//Treat state 0 as unprocessed, 1 as run successfully, and -1 as fail
	
	// If rule is regular file:
	//		If has a dependency that's a reg. file
	//	 	 whose timestamp is newer by > 1 sec, run commands.
	//		Else, DO NOT run commands. Mark satisfied.
	int id = (int) ((intptr_t) arg);
	id = id;
	size_t i = 0;
	short cont;
	myRule_t * rule = NULL;
	myRule_t * curDep = NULL; //current dependency
	/*pthread_mutex_lock(&rulesRemMtx);
	//printf("Thread %d: RR = %d. ", id, rulesRemaining);
	cont = rulesRemaining > 0;
	if(cont) rulesRemaining--;
	pthread_mutex_unlock(&rulesRemMtx);*/
	int TEMP = 0;
	int TMAX = 0;
	//printf("Thread %d started\n", id);
	while(1/*!myRule_allSatisfied()*/) {
		TEMP++;
		/*if(!cont) {
			printf("Thread %d: !cont. Barrier wait...\n", id);
			short done;
			pthread_barrier_wait(&barrier);
			pthread_mutex_lock(&rulesRemMtx);
			done = rulesRemaining == 0;
			if(!done) rulesRemaining--;
			pthread_mutex_unlock(&rulesRemMtx);
			if(done) break;
		}*/
		//printf("Thread %d: Fetching next rule...\n", id);
		pthread_mutex_lock(&rulesRemMtx);
		if(TEMP < TMAX) printf("Thread %d: RR = %d; PQ = %d; (maybe sleeping...)\n", id, rulesRemaining, possibleQueues);
		//OLD pull 
		//OLD break
		while(rulesRemaining <= 0 && possibleQueues > 0) pthread_cond_wait(&rulesRemCv, &rulesRemMtx); //wait for possibleQueues if no remaining
		cont = rulesRemaining > 0;
		if(TEMP < TMAX) printf("Thread %d: woken up! cont = %d\n", id, cont);
		if(cont > 0) { //might have been an unfruitful possibleQueue
			rule = (myRule_t*) myQueue_pull(queue); //TODO: Fix so can't wait forever
			if(TEMP < TMAX) printf("Thread %d: Fetched '%s'.\n", id, rule->rule->target);
			rulesRemaining--;
			possibleQueues++;
		}
		pthread_mutex_unlock(&rulesRemMtx);
		if(!cont) break;
		//1. Update rwinfo; wait if readers
		pthread_mutex_lock(&rule->m);
		//if(TEMP < TMAX && rule->readers) printf("Thread %d: readers! Sleep...\n", id);
		rule->writing = 1;
		while(rule->readers) pthread_cond_wait(&rule->cv, &rule->m);
		pthread_mutex_unlock(&rule->m);
		//if(TEMP < TMAX) printf("\tThread %d: Writing set to 1 (done waiting)...\n", id);
		//2. Get file info. Note if it's a regular file or not.
		if(!rule->info) {
			//printf("Thread %d: '%s' being processed for 1st time!\n", id, rule->rule->target);
			rule->info = calloc(1, sizeof(struct stat));
			/*int statRes = */stat(rule->rule->target, rule->info);
		}
		short diskFile = S_ISREG(rule->info->st_mode);
		short newerDep = !diskFile; //1 if diskFile && there exists newer dependency
		//printf("\tThread %d: Rule '%s' is%s a disk file.\n", id, rule->rule->target, (diskFile ? "" : " NOT"));
		//3. Check dependencies for cond 1. If false, put rule back in queue
		if(rule->rule->state == -1) { //-1 indicates myParser marked it as FAILED b/c of circular dependency!
			pthread_mutex_lock(&rulesRemMtx);
			possibleQueues--;
			pthread_mutex_unlock(&rulesRemMtx);
			pthread_cond_broadcast(&rulesRemCv);
		} else {
		short depSuccess = 1;
		i = 0;
		while(i < rule->numDependencies) {
			curDep = (myRule_t*) Vector_get(rule->myDependencies, i);
			//a. check if writing to dependency. If so, put rule back in queue (try again later).
			short writing = 1;
			short didReading = 0;
			short condfail = 0;
			pthread_mutex_lock(&curDep->m);
			if(!curDep->writing) {
				writing = 0;
				curDep->readers++;
				didReading = 1;
			}
			pthread_mutex_unlock(&curDep->m);
			//b. No writing, so check curDep state to see if finished.
			if(!writing) {
				if(curDep->rule->state == 0) {
					condfail = 1;
				} else if(curDep->rule->state == -1) {
					//if(TEMP < TMAX) printf("\tThread %d: Rule '%s': dependency '%s' is failed.\n", id, rule->rule->target, curDep->rule->target);
					//dependency failed. So this rule failed too
					rule->rule->state = -1;
					depSuccess = 0;
					/*pthread_mutex_lock(&rulesRemMtx);
					rulesRemaining--;
					pthread_mutex_unlock(&rulesRemMtx);*/
				} else { //state 1: dependency satisfied.
					if(diskFile && !newerDep && S_ISREG(curDep->info->st_mode)) {
						if(difftime(curDep->info->st_mtim.tv_sec, rule->info->st_mtim.tv_sec) > 1.0) {
							//printf("\tThread %d: Rule '%s': found more recently modified dependency regular file '%s'.\n", id, rule->rule->target, curDep->rule->target);
							newerDep = 1;
						}
					}
				}
			}
			if(didReading) {
				pthread_mutex_lock(&curDep->m);
				curDep->readers--;
				pthread_mutex_unlock(&curDep->m);
				pthread_cond_signal(&curDep->cv); //wake up writer
			}
			if(writing || condfail) { //put rule back in queue
				//if(TEMP < TMAX) printf("\tThread %d: Rule '%s': some dependencies not satisfied. Go on queue...\n", id, rule->rule->target);
				depSuccess = 0;
				pthread_mutex_lock(&rulesRemMtx);
				rulesRemaining++;
				pthread_mutex_unlock(&rulesRemMtx);
				myQueue_push(queue, (void*) rule);
				//if(TEMP < TMAX) myQueue_print(queue);
			}
			if(!depSuccess) break;
			i++;
		}
		pthread_mutex_lock(&rulesRemMtx);
		possibleQueues--;
		pthread_mutex_unlock(&rulesRemMtx);
		pthread_cond_broadcast(&rulesRemCv);
		//4. If dependency success, run commands and set state if succeed
		if(depSuccess) {
			//4.1 Before run commands, check for no circular dependency.
			/*if(dependencyCycleChecker(rule, rule)) {
				fprintf(stderr, "Thread %d: FAILURE: rule '%s': cycle dependency detected!\n", id, rule->rule->target);
				newerDep = 0;
				rule->rule->state = -1;
			}*/
			if(newerDep) { //will be 1 automatically for non-disk file tgts.
				i = 0;
				size_t numCmds = Vector_size(rule->rule->commands);
				char* curCmd;
				while(i < numCmds) {
					curCmd = (char*)Vector_get(rule->rule->commands, i);
					if(WEXITSTATUS(system(curCmd)) != 0) {
						//fprintf(stderr, "\tRule '%s': Command '%s' failed.\n", rule->rule->target, curCmd);
						rule->rule->state = -1; //command failed. So this rule failed
						/*pthread_mutex_lock(&rulesRemMtx);
						rulesRemaining--;
						pthread_mutex_unlock(&rulesRemMtx);*/
						break;
					}
					i++;
				}
			}
			if(rule->rule->state != -1) { //RULE SATISFIED!
				//printf("\tThread %d: Rule '%s' satisfied!=================\n", id, rule->rule->target);
				rule->rule->state = 1;
				/*pthread_mutex_lock(&rulesRemMtx);
				rulesRemaining--;
				pthread_mutex_unlock(&rulesRemMtx);*/
			}
		}
		}
		//5.1 Cleanup:
		pthread_mutex_lock(&rule->m); 
		rule->writing = 0;
		pthread_mutex_unlock(&rule->m);
		//5.2 Next:
		/*pthread_mutex_lock(&rulesRemMtx);
		//printf("Thread %d: RR = %d. ", id, rulesRemaining);
		cont = rulesRemaining > 0;
		if(cont) rulesRemaining--;
		pthread_mutex_unlock(&rulesRemMtx);*/
	}
	if(TMAX > 0) printf("\nThread %d: done!\n", id);
	return NULL;
}

// Treat this as main
int parmake(int argc, char **argv) {
	//opterr = 0; //set to 0 if don't want getopt to print err msgs
	//1. Get arguments using getopt
	int c;
	short argF = 0;
	while ((c = getopt (argc, argv, "f:j:")) != -1) {
		switch (c) {
			case 'f': makefile = optarg;
					argF = 1;
					break;
			case 'j': //numThreads = (int) optarg[0] - ((int) '0');
					numThreads = stringToInt(optarg);
					break;
		}
	}
	//1.1 Get optional target arg(s) manually
	int i = optind;
	numTargets = argc-optind;
	int targetIndex = 0;
	while(i < argc) {
		if(!targets) targets = malloc(numTargets*sizeof(char*));
		targets[targetIndex] = argv[i];
		//printf("target %d: %s;\n", targetIndex, targets[targetIndex]);
		i++;
		targetIndex++;
	}
	//printf("input: numThreads = %d; makefile = %s; numTargets = %d\n", numThreads, makefile, numTargets);
	//1.2 Error-checking: Make sure can open/read Makefile
	FILE* makeFd = fopen(makefile, "r");
	if(!makeFd) {
		if(argF) {
			//fprintf(stderr, "Cannot open/read specified Makefile '%s'. Done.\n", makefile);
			return 1; //cannot open/read user-specified Makefile
		}
		makefile = "./Makefile";
		makeFd = fopen(makefile, "r");
		if(!makeFd) {
			//fprintf(stderr, "'./makefile' and './Makefile' not found; did you mean to specify one?\nUsage: ./parmake -f <Makefile> -j <numThreads> <target1> ...\n");
			return 2; //"./makefile" and "./Makefile" DNE
		}
	}
	//printf("Success opening '%s'.\n", makefile);
	fclose(makeFd);
	//2. Create threads and parse makefile target(s)
	//queue = queue_create(0, myRule_copy_ctor, myRule_dtor);
	queue = myQueue_create();
	//pthread_barrier_init(&barrier, NULL, numThreads);
	parser_parse_makefile(/*(const char*)*/makefile, targets, myParser);
	pthread_t tids[numThreads];
	i = 0;
	while(i < numThreads) {
		pthread_create(&tids[i], NULL, work, (void*)((intptr_t) i) );
		i++;
	}
	//3. Wait for threads to finish; done
	i = 0;
	while(i < numThreads) {
		pthread_join(tids[i], NULL);
		i++;
	}
	//TODO: Free more mem (myRule_t's)!
	myQueue_destroy(queue);
	myRule_freeList();
	return 0;
}
