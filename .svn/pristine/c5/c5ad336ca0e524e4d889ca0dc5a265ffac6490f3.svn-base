#include <stdio.h>
#include <stdlib.h> //for free()
#include <string.h> //for strcpy(), strcat()

int main(void) {
	/*char * output = (char *) malloc(50);
	strcpy(output, "greeeeeeetings\n\0");*/
	char * output = "greeetings!\n\0";
	char * temp = "trying to cause a segfault\n\0";
	strcat(output, temp); //segfault because output's size only allows it to contain its initial string
	//free(output);
	//free(temp); //segfault because free only valid for mem allocated via _alloc
	printf("result is: %s\n", output);
	return 0;
}
