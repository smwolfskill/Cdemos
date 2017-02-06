#include <pthread.h>

//Allows for thread-safe reading and writing.
// One must pass in the non-thread safe reading and writing functions.
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
