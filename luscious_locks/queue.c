/**
 * Luscious Locks Lab 
 * CS 241 - Fall 2016
 */
#include "queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Struct representing a node in a queue_t
 */
typedef struct queue_node_t {

  struct queue_node_t *next;
  void *data; //desc. says user/callee will have to deal with free'ing this
} queue_node_t;

/**
 * Struct representing a queue
 */
struct queue_t {

  queue_node_t *head, *tail;
  int size;
  int maxSize; //0 indicates no max size
  pthread_cond_t cv;
  pthread_mutex_t m;
};

/**
 *  Given data, place it on the queue.  Can be called by multiple threads.
 *  BLOCKS if the queue is full.
 */
void queue_push(queue_t *queue, void *data) { //think done
	//1st in, 1st out: head is front of the queue
	pthread_mutex_lock(&queue->m); //not sure if any operation here is thread safe..so rather safe than sorry
	while(queue->maxSize > 0 && queue->size == queue->maxSize) //block if full!
		pthread_cond_wait(&queue->cv, &queue->m);
	if(queue->head == NULL) { //1st item inserted into queue
		queue->head = malloc(sizeof(queue_node_t));
		queue->head->data = data;
		queue->tail = queue->head;
	} else { //normal case
		queue->tail->next = malloc(sizeof(queue_node_t));
		queue->tail = queue->tail->next;
		queue->tail->data = data;
	}
	queue->tail->next = NULL;
	queue->size++;
	pthread_mutex_unlock(&queue->m);
	pthread_cond_signal(&queue->cv); //a thread may be waiting to pull; wake up!
}

/**
 *  Retrieve the data from the front/head of the queue.  Can be called by multiple
 * threads.
 *  BLOCKS if the queue is empty.
 */
void *queue_pull(queue_t *queue) { //think done
  /* Your code here */
  void * data;
  pthread_mutex_lock(&queue->m);
  while(queue->size == 0) //block if empty
  	pthread_cond_wait(&queue->cv, &queue->m);
  data = queue->head->data; //queue->head should NEVER be null at this point
  queue_node_t * nextHead = queue->head->next;
  free(queue->head);
  queue->head = nextHead;
  if(!nextHead) queue->tail = NULL; //queue empty again due to pull
  queue->size--;
  pthread_mutex_unlock(&queue->m);
  pthread_cond_signal(&queue->cv); //a thread may be waiting to push; wake up!
  return data;
}

/**
 *  Allocates heap memory for a queue_t and initializes it.
 *  Returns a pointer to this allocated space.
 * NOT thread safe, but shouldn't need to be!
 */
queue_t *queue_create(int maxSize) { //think done
  /* Your code here */
  queue_t * newQueue = malloc(sizeof(queue_t));
  newQueue->maxSize = maxSize;
  newQueue->size = 0;
  newQueue->head = NULL;
  newQueue->tail = NULL;
  if(0 != pthread_cond_init(&newQueue->cv, NULL)) printf("queue_create(): condition variable initialization failed!\n");
  if(0 != pthread_mutex_init(&newQueue->m, NULL)) printf("queue_create(): mutex initialization failed!\n");
  return newQueue;
}

/**
 *  Destroys the queue, freeing any remaining nodes in it.
 * NOT thread safe, but shouldn't need to be!
 */
void queue_destroy(queue_t *queue) { //think done
	//Desc. says user should free their own data they fed into the queue..
	queue_node_t * cur = queue->head;
	queue_node_t * next;
	while(cur) {
		next = cur->next;
		free(cur);
		cur = next;
	}
	free(queue);
}
