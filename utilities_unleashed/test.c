#include <stdio.h>

int main(int argc, char ** argv) {
	printf("test.c called w/ argc = %d\n", argc);
	sleep(2);
	printf("test.c:");
	if(argc > 1) printf(" %s", argv[1]);
	else printf(" (null)");
	if(argc > 2) printf(" %s", argv[2]);
	printf("\n");
	return 0;
}
