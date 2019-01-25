/**
* Finding Filesystems
* CS 241 - Fall 2016
*/
#include "format.h"
#include "fs.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

void printDirectBlocks(file_system* fs, inode * ino, uint64_t data_block_size) {
	printf("PRINTING ALL DIRECT BLOCKS:\n");
	int i = 0;
	char * curData;
	while(i < NUM_DIRECT_INODES) {
		curData = fs->data_root[ino->direct_nodes[i]].data;
		printf("Printing block %d: '", i);
		fflush(stdout);
		write(1, curData, data_block_size);
		write(1, "'\n", 2);
		i++;
	}
	write(1, "=====================\n", 22);
}

void fs_ls(file_system *fs, char *path) {
	// Arrrrrgh Matey
	inode * ino = get_inode(fs, path);
	if(!ino) {
		print_no_file_or_directory();
		return;
	}
	if(is_file(ino)) { //file.
		//Need to extract filename from path (slightly annoying)
		char* filename;
		size_t endIndex = strlen(path) - 1;
		int i = (int) endIndex;
		while(i >= 0) {
			if(path[i] == '/') { //found start of filename at i+1
				i++; 
				break;
			} 
			//Ex: path = "/path/to/file". Iterate backwards until 
			//we find '/' by "o/f". So start at i+1 = 'f'
			i--;
		}
		//printf("ls: start index of filename is %d; endIndex = %lu\n", i, endIndex);
		filename = malloc(endIndex - i + 2);
		filename[endIndex-i+1] = '\0';
		int j = 0;
		while(i <= (int) endIndex) {
			filename[j] = path[i];
			//printf("\t(i,j) = (%d,%d): '%s'\n", i, j, filename);
			i++;
			j++;
		}
		//printf("ls: got filename '%s'\n", filename);
		print_file(filename);
		free(filename);
		return;
	} //Else, directory:
	dirent * myDirent;
	inode * dir = ino;
	inode * cur;
	char * curData;
	uint64_t bytesLeft = ino->size;
	uint64_t bytesDone = 0;
	uint64_t data_block_size = (uint64_t) sizeof(data_block);
	//printf("data_block_size = %lu\n", data_block_size);
	int i = 0;
	int success = 1;
	//printDirectBlocks(fs, dir, data_block_size);
	while(bytesLeft > 0 && i <= NUM_DIRECT_INODES) {
		myDirent = calloc(1, sizeof(dirent));
		if(bytesDone % data_block_size == 0) { //go to new data_block
			curData = fs->data_root[dir->direct_nodes[i]].data;
			/*printf("(BytesLeft = %lu); Got new block %d starting with '", bytesLeft, i);
			fflush(stdout);
			write(1, curData, 10);
			write(1, "'...\n", 5);*/
			i++;
		}
		if(!(success = make_dirent_from_string(curData+(bytesDone%data_block_size), myDirent) )) break;
		cur = &fs->inode_root[myDirent->inode_num];
		if(is_file(cur)) print_file(myDirent->name);
		else print_directory(myDirent->name);
		bytesLeft -= 256;
		bytesDone += 256;
		free(myDirent);
		if(bytesLeft > 0 && i == (NUM_DIRECT_INODES) && bytesDone % data_block_size == 0) {
			//Did all of single direct but still bytesLeft; do indirect
			//printf("\tSingle indirect...\n");
			dir = &fs->inode_root[dir->single_indirect];
			i = 0;
		}
	}
	if(!success) free(myDirent);
}

void fs_cat(file_system *fs, char *path) {
	// Shiver me inodes
	//The data_blocks run for sizeof(data_block) bytes. Your job is to write a function that loops through all of the data blocks in the node (possibly including indirect blocks) and prints out all of the bytes to standard out. 
	inode * ino = get_inode(fs, path);
	if(!ino) {
		print_no_file_or_directory();
		return;
	}
	//TODO: Need to use permissions?
	//inode->permissions: 9 LSBs are read-write-execute for owner-group-others
	//how to tell who current user is?
	//1. Print out all bytes from the direct block.
	uint64_t bytesLeft = ino->size; //# bytes we still need to print
	uint64_t data_block_size = (uint64_t) sizeof(data_block);
	uint64_t curBytes;
	int i = 0;
	data_block * cur;
	while(bytesLeft > 0 && i < NUM_DIRECT_INODES) {
		cur = &fs->data_root[ino->direct_nodes[i]];
		curBytes = (bytesLeft > data_block_size ? data_block_size : bytesLeft);
		write(1, cur->data, curBytes);
		bytesLeft -= curBytes;
		i++;
	}
	//2. If that's not all, print remaining bytes that're in indirect block
	if(bytesLeft) {
		inode * indirect = &fs->inode_root[ino->single_indirect];
		i = 0;
		while(bytesLeft) {
			cur = &fs->data_root[indirect->direct_nodes[i]];
			curBytes = (bytesLeft > data_block_size ? data_block_size : bytesLeft);
			write(1, cur->data, curBytes);
			bytesLeft -= curBytes;
			i++;
		}
		//write(1, "\nUsed indirect!\n", 16);
	}
	//write(1, "cat!\n", 5);
	//fflush(stdout);
}
