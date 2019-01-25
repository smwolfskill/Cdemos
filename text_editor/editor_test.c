/**
 * Machine Problem: Text Editor
 * CS 241 - Fall 2016
 */

#include "document.h"
#include "editor.h"
#include "format.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/wait.h> //for waitpid
#include <unistd.h> //for exec
#include <stdlib.h> //for exit

/**
 * You can programatically test your text editor.
*/
void printTest(int num) {
	fprintf(stderr, "==============================\nTEST %d:\n", num);
}

void printDocument(Document * doc) {
	fprintf(stderr, "Printing contents of document...\n");
	handle_display_command(doc, "p");
}

int main() {
  // Setting up a docment based on the file named 'filename'.
  char *filename = "test.txt";
  Document *document = Document_create_from_file(filename);
	//Test 1: overwrite line 3 (or create it) with "hello" and list all contents
  printTest(1);
  handle_write_command(document, "w 1 hello1");
  handle_write_command(document, "w 5 hello5");
  printDocument(document);
  //Test 2: append "append1" to line 2 ("")
  printTest(2);
  handle_append_command(document, "a 2 append1");
  handle_display_command(document, "p");
  //Test 3: append "ApPend2" to line 1 ("hello1")
  printTest(3);
  handle_append_command(document, "a 1 ApPend2");
  printDocument(document);
  //Test 4: write "" to line 1 ("hello1ApPend2")
  printTest(4);
  handle_write_command(document, "w 1");
  printDocument(document);
  //Test 5: Delete line 2 ("append1")
  printTest(5);
  handle_delete_command(document, "d 2");
  printDocument(document);
  //Test 6: Search for "hello" (should be two instances)
  printTest(6);
  handle_search_command(document, "/hello");
  printDocument(document);
  //Test 7: 
  printTest(7);
  handle_write_command(document, "w 3 wmulti$wmulti2$wmulti3");
  printDocument(document);
  //Test 8: Test multi appends
  printTest(8);
  handle_append_command(document, "a 5 AMULTI$AMULTI2$AMULTI3");
  printDocument(document);
  //Test 9: Save by writing to file, and read it back
  printTest(9);
  handle_save_command(document, "testSave.txt");
  pid_t child = fork();
  if(!child) { //I'm child
  	fprintf(stderr, "Printing contents of file testSave.txt...\n");
  	execlp("cat", "cat", "testSave.txt", (const char*) NULL);
  	fprintf(stderr, "exec failed!\n");
  	exit(1);
  } else if(child == -1) fprintf(stderr, "fork() failed!\n");
  int status = 0;
  waitpid(child, &status, 0);
  if(WIFEXITED(status)==0 || WEXITSTATUS(status) != 0) exit(1);
  //CLEANUP:
  Document_destroy(document);
  //free(document); //apparently unnecessary; Doc.._destroy() does it
  return 0;
}
