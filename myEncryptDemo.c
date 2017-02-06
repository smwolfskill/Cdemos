#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //for read()
#include <fcntl.h> //for open()
/*Author: Scott Wolfskill
 *Last edited: 08/30/16  
 *
 *Simple encryption demo using fact that (x ^ a) ^ a = xa' = x (in this case)
 * creates encryption.txt with an encrypted "Hi!Scott"  */
void decrypt() {
	//First, read encrypted string from the file:
	char * buf = malloc(9);
	int fd = open("encryption.txt", O_RDONLY);
	read(fd, buf, 9);
	close(fd);
	printf("encrypted:\n%s\n", buf);
	//Decrypt it:
	int hex[] = {0x3f, 0x10, 0xa5, 0xde, 0x28, 0x6c, 0xac, 0xd2, 0xff};
	int i = 0;
	while(i < 8) {
		buf[i] = buf[i] ^ hex[i++];
	}
	printf("\ndecrypted:\n%s\n", buf);
	free(buf);
}

int main() {
	FILE * fd = fopen("encryption.txt", "w+");
	char * msg = "Hi!Scott";
	char * encrypted = malloc(9);
	int hex[] = {0x3f, 0x10, 0xa5, 0xde, 0x28, 0x6c, 0xac, 0xd2, 0xff};
	int i = 0;
	while(i < 8) {
		encrypted[i] = msg[i] ^ hex[i++];
	}
	encrypted[8] = '\0';
	fprintf(fd, encrypted);
	fclose(fd);
	free(encrypted);
	
	decrypt();
	return 0;
}
