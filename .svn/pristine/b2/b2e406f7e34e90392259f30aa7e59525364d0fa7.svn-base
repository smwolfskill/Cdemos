/**
 * Mad Mad Access Pattern
 * CS 241 - Fall 2016
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h> //for realloc
#include "tree.h"
#include "utils.h"

#define PRINT_ADDRESS 0
#define PRINT_NODES 0
/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses fseek() and fread() to access the data.

  ./lookup1 <data_file> <word> [<word> ...]
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

BinaryTreeNode * createNode(FILE * f, uint32_t startIndex) {
	size_t BinaryTreeNode_size = sizeof(BinaryTreeNode);
	BinaryTreeNode * allocatedNode = malloc(BinaryTreeNode_size);
	if(startIndex != 0) {
		fseek(f, startIndex, SEEK_SET);
		fread(&allocatedNode->left_child, 4, 1, f); //fread and fgetc call fseek
		fread(&allocatedNode->right_child, 4, 1, f);
		fread(&allocatedNode->count, 4, 1, f);
		fread(&allocatedNode->price, 4, 1, f);
		size_t wordLen = 0;
		int c;
		while(1) {
			c = fgetc(f);
			if(c == EOF) break;
			//printf("%c = %d;", c, c);
			//Don't need to allocate b/c "char[0]" is dynamic-length
			allocatedNode = realloc(allocatedNode, BinaryTreeNode_size + wordLen + 1);
			allocatedNode->word[wordLen] = (char) c;
			wordLen++;
			if(c == '\0') break;
		}
		if(PRINT_NODES) printNode(allocatedNode, startIndex, PRINT_ADDRESS);
	} //else fprintf(stderr, "ERROR: setNode called w/ startIndex = %u\n", startIndex);
	return allocatedNode;
}

//Recursively traverses search tree until either finds it or reaches leaf node
void findKeyword(FILE * f, BinaryTreeNode * curNode, const char * keyword) {
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
	//Version 1: fseek/fread (NO mmap)
	//printf("strcmp(aaa, baa) = %d; strcmp(aaa, aba) = %d; strcmp(aaa, abb) = %d; strcmp(zeg, aaa) = %d\n", strcmp("aaa", "baa"), strcmp("aaa", "aba"), strcmp("aaa", "bbb"), strcmp("ccc", "aaa"));
	if(argc < 3) {
		printArgumentUsage();
		return 1;
	}
	short readSuccess = 1;
	FILE * f = fopen(argv[1], "r");
	readSuccess = (f != NULL);
	if(readSuccess) {
		char cur[BINTREE_ROOT_NODE_OFFSET+1];
		fseek(f, 0, SEEK_SET);
		if(fread(cur, 1, BINTREE_ROOT_NODE_OFFSET, f) < BINTREE_ROOT_NODE_OFFSET) readSuccess = 0;
		else {
			cur[BINTREE_ROOT_NODE_OFFSET] = '\0';
			if(strcmp(cur, BINTREE_HEADER_STRING)) {
				//fprintf(stderr, "(1st 4 bytes were %s !)\n", cur);
				readSuccess = 0;
			} else {
				int keywordIndex = 2;
				BinaryTreeNode * cur;
				while(keywordIndex < argc) {
					//printf("keywordIndex %d: searching for keyword '%s'...\n", keywordIndex, argv[keywordIndex]);
					cur = createNode(f, BINTREE_ROOT_NODE_OFFSET);
					findKeyword(f, cur, argv[keywordIndex]);
					keywordIndex++;
				}
			}
		}
	}
	else {
		openFail(argv[1]);
		return 2;
	}
	if(!readSuccess) {
		formatFail(argv[1]);
		return 2;
	}
	fclose(f);
	return 0;
}
