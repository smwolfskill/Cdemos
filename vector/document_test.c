/**
 * Machine Problem: Vector
 * CS 241 - Fall 2016
 */

#include "document.h"
#include <stdio.h>

void listAll(Document * doc) {
	size_t i = 1;
	size_t size = Document_size(doc);
	printf("    Listing all lines of a Document of size %d...\n", (int) size);
	while(i <= size) {
		printf("\tLine %d:%s\n", (int) i, Document_get_line(doc, i));
		i++;
	}
	printf("    (done)\n");
}
// test your document here
int main() { 
	//1: Read from file "document_test0.txt" and write it back to "...1.txt"
    Document * doc = Document_create_from_file("document_test0.txt");
    listAll(doc);
	Document_write_to_file(doc, "document_test1.txt");
	Document_destroy(doc);
	return 0; 
}
