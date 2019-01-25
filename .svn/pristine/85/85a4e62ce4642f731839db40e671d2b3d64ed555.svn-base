/**
 * Luscious Locks Lab 
 * CS 241 - Fall 2016
 */

#include "semamore.h"

#include <stdio.h>

/**
 * Initializes the Semamore. Important: the struct is assumed to have been
 * allocated by the user.
 * Example:
 * 	Semamore *s = malloc(sizeof(Semamore));
 * 	semm_init(s, 5, 10);
 *
 */
void semm_init(Semamore *s, int value, int max_val) { //think done
	s->value = value;
	s->max_val = max_val;
	if(0 != pthread_mutex_init(&s->m, NULL)) printf("semm_init(): mutex initialization failed!\n");
	if(0 != pthread_cond_init(&s->cv, NULL)) printf("semm_init(): condition variable initialization failed!\n");
}

/**
 *  Should block when the value in the Semamore struct (See semamore.h) is at 0.
 *  Otherwise, should decrement the value.
 */
void semm_wait(Semamore *s) { //think done
	pthread_mutex_lock(&s->m);
	while(s->value == 0)
		pthread_cond_wait(&s->cv, &s->m);
	s->value--;
	pthread_mutex_unlock(&s->m);
	pthread_cond_broadcast(&s->cv); //thread(s) may be on semm_post; wake up!
}

/**
 *  Should block when the value in the Semamore struct (See semamore.h) is at
 * max_value.
 *  Otherwise, should increment the value.
 */
void semm_post(Semamore *s) { //think done
	pthread_mutex_lock(&s->m);
	while(s->value == s->max_val)
		pthread_cond_wait(&s->cv, &s->m);
	s->value++;
	pthread_mutex_unlock(&s->m);
	pthread_cond_broadcast(&s->cv); //thread(s) may be on semm_wait; wake up!
}

/**
 * Takes a pointer to a Semamore struct to help cleanup some members of the
 * struct.
 * The actual Semamore struct must be freed by the user.
 */
void semm_destroy(Semamore *s) { //think done
	pthread_mutex_destroy(&s->m);
	pthread_cond_destroy(&s->cv);
}
