#include <stdio.h>
#include <stdlib.h> //for malloc
#include <string.h>
struct Person {
	char * name;
	int age;
	struct Person * friends[];
};
typedef struct Person person_t;

person_t * create(char * name, int age) {
	person_t * toReturn = (person_t *) malloc(sizeof(person_t));
	toReturn->name = strdup(name);
	toReturn->age = age;
}

void destroy(person_t * person) {
	free(person->name);
	person->name = NULL;
	free(person);
	//Note: can still use person->friends just fine
}

int main() {
	person_t * agentSmith = create("Agent Smith", 128);
	person_t * sonnyMoore = create("Sonny Moore", 256);
	agentSmith->friends[0] = sonnyMoore;
	sonnyMoore->friends[0] = agentSmith;
	destroy(agentSmith);
	printf("sonny exists? %p\n%s\n", &(agentSmith->friends[0]), agentSmith->friends[0]->name);
	printf("agent exists? %p\n%s\n", &agentSmith, agentSmith->name);
	return 0;
}
