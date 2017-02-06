#include <stdio.h>

/* 08/30/16
 * Simple demo showing what can happen if you use uninitialized vars.
 * Since test.i goes out of scope and immediately hack() is called, old's address
 * is the same as i's was. Thus we can use the last value of i hackerishly >:)  */

void test() { 
	int i = 69;
	int f = 274; 
}

void hack() {
	int oldi, oldf;
	printf("Hahahaha! %d, %d\n", oldi, oldf);
} //prints 69, 274

int main(int argc, char * argv[]) {
	test();
	hack();
	return 0;
}
