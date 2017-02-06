#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
int main() {
	char * buf = NULL;
	size_t capacity = 0;
	ssize_t result = 1;
	FILE * fd = fopen("readWholeFileDemo.txt", "r");
	while(result > 0) {
		result = getline(&buf, &capacity, fd);
		if(result > 0) {
			if(buf[result-1]=='\n') buf[result-1] = '\0'; //replace terminating ‘\n’ with ‘\0’ which is much more useful
			puts(buf);
		}
	}
	fclose(fd);
	free(buf);
	return 0;
}
