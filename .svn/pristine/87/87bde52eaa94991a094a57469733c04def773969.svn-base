/**
 * Scheduler Lab
 * CS 241 - Fall 2016
 */
#include "libpriqueue.h"
#include "libscheduler.h"
//#include <time.h>

typedef struct _job_t job_t;
typedef struct _core_t core_t;
/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job
  queue elements.
*/
struct _job_t {
  int id;		 //aka job number
  int priority;
  /* Add whatever other bookkeeping you need into this struct: */
  int arrivalTime; //for consistency, we use our getCurTime and not their provided arrivalTime param. TODO: is this bad?
  int startTime; //time job first started executing
  int curStartTime; //time job either first started/resumed executing. Only needed for pre-emption!
  int burstTime; //est. time it will take to execute
  int burstTime_remaining; //burstTime - timeRunningOnCPU. Only needed for pre-emption!
};

struct _core_t {
  int free;   // boolean representing whether the core is in use or not
  job_t *job; // the job that the core is currently running, if any
};

priqueue_t pqueue;
int numCores = 0;
core_t *cores; //array of cores, dynamically allocated. id is index!
scheme_t scheme;
short preEmpt = 0; //1 if choose a scheme involving pre-emption
//Statistics helper vars:
int jobsFinished = 0;
int turnaroundTimeSum = 0; //sum of turnaround times for all finished jobs
int burstTimeSum = 0; //sum of burst times for all finished jobs
int responseTimeSum = 0; //sum of response times for all finished jobs

/*double getCurTime() {
	struct timespec curTime;
	clock_gettime(CLOCK_MONOTONIC, &curTime);
	return ((double) curTime.tv_nsec) / 1000000000.0 + (double) curTime.tv_sec;
}*/

//Pass in job_t * !
int comparer_fcfs(const void *a, const void *b) { //maybe right? in theory?
	int arrivalA = ((job_t*)a)->arrivalTime;
	int arrivalB = ((job_t*)b)->arrivalTime;
	if(arrivalA == arrivalB) return 0; //tie
	return (arrivalA < arrivalB ? -1 : 1); //return -1 if choose 0th param
}

//In a tie, choose fcfs
int break_tie(const void *a, const void *b) { return comparer_fcfs(a, b); }

int comparer_ppri(const void *a, const void *b) {
  //Pre-emptive Priority. Complete as is
  return comparer_pri(a, b);
}

int comparer_pri(const void *a, const void *b) {
	//Priority. Higher value = LOWER priority
	int priorityA = ((job_t*)a)->priority;
	int priorityB = ((job_t*)b)->priority;
	if(priorityA == priorityB) return break_tie(a, b);
	return ( priorityA < priorityB ? -1 : 1); //return -1 if choose 0th param
	return 0;
}

int comparer_psjf(const void *a, const void *b) {
	//Pre-emptive shortest-job first. Choose job w/ shortest REMAINING burst time!
	//	Interrupted jobs that are shorter than every other placed at queue head!
	int remainingA = ((job_t*)a)->burstTime_remaining;
	int remainingB = ((job_t*)b)->burstTime_remaining;
	if(remainingA == remainingB) return break_tie(a, b);
	return ( remainingA < remainingB ? -1 : 1); //return -1 if choose 0th param
	//return comparer_sjf(a, b); //OLD
}

int comparer_rr(const void *a, const void *b) { //TODO?
  // Casting to void to silence compiler warning
  (void)a;
  (void)b;
  // Picking 1 arbitarily.
  return 1;
}

int comparer_sjf(const void *a, const void *b) { //think correct. untested
	//Shortest job first. The currently running job will NOT be interrupted.
	int burstA = ((job_t*)a)->burstTime;
	int burstB = ((job_t*)b)->burstTime;
	if(burstA == burstB) return break_tie(a, b);
	return ( burstA < burstB ? -1 : 1); //return -1 if choose 0th param
}

void scheduler_start_up(int numcores, scheme_t s) {
  switch (s) {
  case FCFS:
    priqueue_init(&pqueue, comparer_fcfs);
    break;
  case PRI:
    priqueue_init(&pqueue, comparer_pri);
    break;
  case PPRI:
    priqueue_init(&pqueue, comparer_ppri);
    preEmpt = 1;
    break;
  case PSJF:
    priqueue_init(&pqueue, comparer_psjf);
    preEmpt = 1;
    break;
  case RR:
    priqueue_init(&pqueue, comparer_rr);
    //preEmpt = 1; //I think? Depends on how I implement
    break;
  case SJF:
    priqueue_init(&pqueue, comparer_sjf);
    break;
  default:
    printf("Did not recognize scheme\n");
    exit(1);
  }
  //My code: init cores:
  cores = malloc(numcores * sizeof(core_t));
  numCores = numcores;
  int i = 0;
  while(i < numcores) { cores[i].free = 1; cores[i].job = NULL; i++; }
}

//Generalized version of new_job: assigns head of priqueue to a core.
//	If core >= 0, will attempt to ONLY run on that core.
//	If can't run (must wait; no pre-emption), ret. -1.
//	If newJob != NULL and can run, ret. index of core running on.
int runNextJob(int curTime, job_t * newJob, int core) {
	//1. If there is a free core(s), run head of queue on the lowest index/id core. (No waiting, conflicts, or interrupts; easy, ideal case).
	job_t * runNext = (job_t*) priqueue_peek(&pqueue);
	if(!runNext) return -1; //no jobs left!
	short ranHead = 0; //1 if we start executing runNext now
	//	Check if there is a free core:
	int i = 0; //core index
	if(core >= 0) { //will attempt to run on ONLY that core.
		//Since only called for RR and for task finished, don't need pre-emption (b/c that core will be free anyway, right?)
		if(cores[core].free) {
			cores[core].free = 0;
			cores[core].job = runNext;
			priqueue_poll(&pqueue);
			ranHead = 1;
			i = core;
		}
	} else {
	while(i < numCores) {
		if(cores[i].free) {
			cores[i].free = 0;
			cores[i].job = runNext;
			priqueue_poll(&pqueue);
			ranHead = 1;
			break;
		}
		i++;
	}
	if(preEmpt && i == numCores) {
	//2. No free cores. If no pre-emption, done; ret. -1
		//Else, pre-empt a job on a core if runNext is more desirable > currently running least-desirable job
		//How select which core? Idea: Use comparator fxn to find least-desirable job running on one of the cores
		i = 1;
		int lesser = 0; //index of core w/ least-desirable job (that we'll pre-empt)
		cores[0].job->burstTime_remaining -= curTime - cores[0].job->curStartTime;
		while(i < numCores) {
			cores[i].job->burstTime_remaining -= curTime - cores[i].job->curStartTime;
			if(pqueue.comparer((void*)cores[lesser].job,(void*)cores[i].job) <= 0) lesser = i; //if ret. -1, chose lesser as superior. So new lesser is i
			i++;
		}
		job_t * preEmptJob = cores[lesser].job;
		if(pqueue.comparer((void*)runNext,(void*)preEmptJob) <= 0) {
			//runNext is more desirable than currently running least-desirable job: so pre-empt it
			cores[lesser].job = runNext;
			priqueue_poll(&pqueue); //remove runNext from queue
			priqueue_offer(&pqueue, (void*) preEmptJob); //put job we pre-empted back on queue
			ranHead = 1;
			i = lesser;
		}
	}
	}
	if(ranHead) {
		if(runNext->startTime == -1) runNext->startTime = curTime; //startTime wasn't set yet
		runNext->curStartTime = curTime;
		if(runNext == newJob) //newJob running now on core i
			return i;
	}
	return -1; //-1 if can't run right now (wait)
}

int scheduler_new_job(int job_number, int time, int running_time, int priority) {
	/*double curTime = getCurTime();
	printf("\tnew_job: curTime = %f\n", curTime);*/
	job_t * newJob = malloc(sizeof(job_t));
	newJob->id = job_number;
	newJob->arrivalTime = time;
	newJob->burstTime = running_time;
	newJob->burstTime_remaining = running_time;
	newJob->startTime = -1; //not set yet
	newJob->curStartTime = -1; //not set yet
	newJob->priority = priority;
	priqueue_offer(&pqueue, (void*) newJob);
	return runNextJob(time, newJob, -1);
}

int scheduler_job_finished(int core_id, int job_number, int time) { //think done?
	/*double curTime = getCurTime();
	printf("\tjob_finished: curTime = %f\n", curTime);*/
	job_t * finished = cores[core_id].job;
	jobsFinished++;
	turnaroundTimeSum += time - finished->arrivalTime;
	burstTimeSum += finished->burstTime;
	responseTimeSum += finished->startTime - finished->arrivalTime;
	cores[core_id].free = 1;
	free(finished);
	//If any job should be scheduled to run on the core free'd up by the finished job, return the job_number of the job that should be scheduled to run on core core_id.
	int tmp = runNextJob(time, (job_t*)priqueue_peek(&pqueue), core_id);
	if(tmp == core_id) return cores[core_id].job->id;
	return -1;
}

//For Round Robin ONLY!
int scheduler_quantum_expired(int core_id, int time) {
	//I assume will only be called on cores whose jobs are not yet finished
	//cores[core_id].job->burstTime_remaining -= time - cores[core_id].job->curStartTime; //this is true, but NOT needed for RR
	priqueue_offer(&pqueue, (void*)cores[core_id].job);
	cores[core_id].free = 1;
	//If any job should be scheduled to run on the core free'd up by the quantum expiration, return the job_number of the job that should be scheduled to run on core core_id.
	int tmp = runNextJob(time, (job_t*)priqueue_peek(&pqueue), core_id);
	if(tmp == core_id) return cores[core_id].job->id;
	return -1;
}

float scheduler_average_waiting_time() {
	//WaitTime = turnaroundTime - runTime (aka burstTime?)
	printf("\t(turnaroundTimeSum - burstTimeSum) / jobsFinished = (%d - %d) / %d\n", turnaroundTimeSum, burstTimeSum, jobsFinished);
	return ((float)(turnaroundTimeSum - burstTimeSum) / jobsFinished);
}

float scheduler_average_turnaround_time() {
	//Avg. time from when process starts to when it ends
	return (float) turnaroundTimeSum / jobsFinished;
}

float scheduler_average_response_time() {
	//ResponseTime = startTime - arrivalTime
	return (float) responseTimeSum / jobsFinished;
}

void scheduler_clean_up() {
  //Do more cleaning up here if needed ?
  void * cur = priqueue_poll(&pqueue);
  while(cur != NULL) { free(cur); cur = priqueue_poll(&pqueue); }
  priqueue_destroy(&pqueue);
  free(cores);
}

void scheduler_show_queue() {
  // This function is left entirely to you! Totally optional.
  job_t * head = priqueue_peek(&pqueue);
  if(head) printf("head = id %d, pri %d, bt_rem %d; (don't show rest, if any)", head->id, head->priority, head->burstTime_remaining);
}
