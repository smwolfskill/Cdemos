/**
 * Mad Mad Access Pattern
 * CS 241 - Fall 2016
 */
#include <sys/mman.h> //for mmap
#include <sys/stat.h>	//for open
#include <fcntl.h> //..
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "tree.h"
#include "utils.h"

#define PRINT_ADDRESS 0
#define PRINT_NODES 0
/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses mmap to access the data.

  ./lookup2 <data_file> <word> [<word> ...]
*/

void printNode(BinaryTreeNode const * const toPrint, uint32_t offset, short addresses) {
	printf("PRINTING NODE @ offset %u:\n", offset);
	printf("\tleft_child = %u\n", toPrint->left_child);
	if(addresses) printf("\t\t%p\n", &toPrint->left_child);
	printf("\tright_child = %u\n", toPrint->right_child);
	if(addresses) printf("\t\t%p\n", &toPrint->right_child);
	printf("\tcount = %u\n", toPrint->count);
	if(addresses) printf("\t\t%p\n", &toPrint->count);
	printf("\tprice = %f\n", toPrint->price);
	if(addresses) printf("\t\t%p\n", &toPrint->price);
	printf("\tword = %s\n", toPrint->word);
	if(addresses) printf("\t\t%p\n", &toPrint->word);
	printf("====================\n");
}

BinaryTreeNode * createNode(char * f, uint32_t startIndex) {
	size_t BinaryTreeNode_size = sizeof(BinaryTreeNode);
	BinaryTreeNode * allocatedNode = malloc(BinaryTreeNode_size);
	if(startIndex != 0) {
		allocatedNode->left_child = *((uint32_t*)(&f[startIndex]));
		allocatedNode->right_child = *((uint32_t*)(&f[startIndex+4]));
		allocatedNode->count = *((uint32_t*)(&f[startIndex+8]));
		allocatedNode->price = *((float*)(&f[startIndex+12]));
		size_t wordLen = 0;
		size_t i = startIndex + BinaryTreeNode_size;
		while(1) {
			//printf("f[%d] = %c;", i, f[i]);
			//Don't need to allocate b/c "char[0]" is dynamic-length
			allocatedNode = realloc(allocatedNode, BinaryTreeNode_size + wordLen + 1);
			allocatedNode->word[wordLen] = f[i];
			wordLen++;
			if(f[i] == '\0') break;
			i++;
		}
		if(PRINT_NODES) printNode(allocatedNode, startIndex, PRINT_ADDRESS);
	} //else fprintf(stderr, "ERROR: setNode called w/ startIndex = %u\n", startIndex);
	return allocatedNode;
}

//Recursively traverses search tree until either finds it or reaches leaf node
void findKeyword(char * f, BinaryTreeNode * curNode, const char * keyword) {
	short noPath = 0; //1 if there's no path to get to keyword; DNE
	//if(!curNode) printf("findKeyword: curNode was NULL!\n");
	//if(!keyword) printf("findKeyword: keyword was NULL!\n");
	if(strcmp(keyword, curNode->word) == 0) { //case 1: found it!
		printFound(keyword, curNode->count, curNode->price);
		free(curNode);
	} else if(strcmp(keyword, curNode->word) < 0) { //Go to L child, if exists
		if(curNode->left_child == 0) noPath = 1;
		else {
			BinaryTreeNode * Lchild = createNode(f, curNode->left_child);
			free(curNode); //to prevent running out of mem
			if(Lchild) findKeyword(f, Lchild, keyword);
			else {
				//fprintf(stderr, "Tried create Lchild, but was NULL!\n");
				noPath = 1;
			}
		}
	} else { //Go to R child, if exists
		if(curNode->right_child == 0) noPath = 1;
		else {
			BinaryTreeNode * Rchild = createNode(f, curNode->right_child);
			free(curNode); //to prevent running out of mem
			if(Rchild) findKeyword(f, Rchild, keyword);
			else {
				//fprintf(stderr, "Tried create Rchild, but was NULL!\n");
				noPath = 1;
			}
		}
	}
	if(noPath) {
		printNotFound(keyword);
		free(curNode);
	}
}

int main(int argc, char **argv) {
	//Version 2: MMAP ONLY! No read(). I assume we can use seek...
	if(argc < 3) {
		printArgumentUsage();
		return 1;
	}
	//1. Open data file. Check for errors.
	int fd = open(argv[1], O_RDONLY);
	if(fd == -1) {
		openFail(argv[1]);
		return 2;
	}
	//2. Map it to memory using mmap
	size_t size = lseek(fd, 0, SEEK_END);
	//printf("file '%s' has %lu bytes.\n", argv[1], size);
	char * f = (char*) mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	//3. Check if first specified bytes are in correct format
	char start[BINTREE_ROOT_NODE_OFFSET+1];
	start[BINTREE_ROOT_NODE_OFFSET] = '\0';
	size_t i = 0;
	while(i < BINTREE_ROOT_NODE_OFFSET) {
		start[i] = f[i];
		i++;
	}
	if(strcmp(start, BINTREE_HEADER_STRING)) {
		formatFail(argv[1]);
		return 2;
	}
	//4. Run search for all keyword args
	int keywordIndex = 2;
	BinaryTreeNode * cur;
	while(keywordIndex < argc) {
		//printf("keywordIndex %d: searching for keyword '%s'...\n", keywordIndex, argv[keywordIndex]);
		cur = createNode(f, BINTREE_ROOT_NODE_OFFSET);
		findKeyword(f, cur, argv[keywordIndex]);
		keywordIndex++;
	}
	//5. Cleanup
	close(fd);
	munmap((void*)f, size);
	return 0;
}
