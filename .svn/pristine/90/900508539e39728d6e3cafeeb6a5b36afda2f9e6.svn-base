/**
 * Luscious Locks Lab 
 * CS 241 - Fall 2016
 */
#include "barrier.h"

// The returns are just for errors if you want to check for them.
int barrier_destroy(barrier_t *barrier) {
  int error = pthread_cond_destroy(&barrier->cv) || pthread_mutex_destroy(&barrier->mtx);
  barrier->n_threads = 0; //callee can no longer use barrier w/o problems
  return error;
}

int barrier_init(barrier_t *barrier, unsigned int num_threads) {
  int error = pthread_cond_init(&barrier->cv, NULL) || pthread_mutex_init(&barrier->mtx, NULL) || !num_threads;
  if(!error) {
 	barrier->n_threads = num_threads; //should be > 0
  	barrier->count = 0;
  	barrier->times_used = 1; //1 or 0?
  }
  return error;
}

int barrier_wait(barrier_t *barrier) {
	pthread_mutex_lock(&barrier->mtx);
	barrier->count++;
	if(barrier->count == barrier->n_threads * barrier->times_used) {
		barrier->times_used++;
	} else {
		unsigned int cont = barrier->n_threads * barrier->times_used;
		//need cont for the conditional, else we might change times_used while a thread is still waiting, causing it to be stuck
		while(barrier->count < cont) { 
			//have < instead of != because if one thread passes barrier there's race condition for other threads to leave the loop before a thread calls wait on barrier again.
			pthread_cond_wait(&barrier->cv, &barrier->mtx);
		}//not even necessary to have barrier->times_used; can just decrement. But w/e
	}
	pthread_mutex_unlock(&barrier->mtx);
	pthread_cond_broadcast(&barrier->cv); //wake all up
	return 0;
}
