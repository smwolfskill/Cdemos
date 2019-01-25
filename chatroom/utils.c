/**
 * Chatroom Lab
 * CS 241 - Fall 2016
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include "utils.h"
static const size_t MESSAGE_SIZE_DIGITS = 4;

char *create_message(char *name, char *message) {
  int name_len = strlen(name);
  int msg_len = strlen(message);
  char *msg = calloc(1, msg_len + name_len + 4);
  sprintf(msg, "%s: %s", name, message);

  return msg;
}

ssize_t get_message_size(int socket) {
  int32_t size;
  ssize_t read_bytes =
      read_all_from_socket(socket, (char *)&size, MESSAGE_SIZE_DIGITS);
  if (read_bytes == 0 || read_bytes == -1)
    return read_bytes;

  return (ssize_t)ntohl(size);
}

ssize_t write_message_size(size_t size, int socket) {
  //Prefix message w/ first 4 bytes being the #bytes of the message.
  //Returns #bytes successfully written, 0 if socket is disconnected, or -1 on failure
	uint32_t size_net = htonl((uint32_t) size);
  //return write_all_to_socket(socket, (char*)size_net, MESSAGE_SIZE_DIGITS);
	size_t rem = MESSAGE_SIZE_DIGITS;
	ssize_t res = -1;
	while(rem > 0) {
		errno = 0;
	 	res = write(socket, &size_net, MESSAGE_SIZE_DIGITS);
	 	if(res == 0) return 0; //socket disconnected
	 	else if(res == -1) {
	 		if(errno != EINTR) return -1; //write failure
	 	} else rem -= res;
	 }
  return MESSAGE_SIZE_DIGITS;
}

ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
  //Returns #bytes read, 0 if socket is disconnected, or -1 on failure.
	size_t rem = count;
	ssize_t res = -1;
	while(rem > 0) {
		errno = 0;
	 	res = read(socket, buffer, count);
	 	if(res == 0) return 0; //socket disconnected
	 	else if(res == -1) {
	 		if(errno != EINTR) return -1; //read failure
	 	} else rem -= res;
	 }
  return count;
}

ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) {
  //Returns #bytes successfully written, 0 if socket is disconnected, or -1 on failure
	size_t rem = count;
	ssize_t res = -1;
	while(rem > 0) {
		errno = 0;
	 	res = write(socket, buffer, count);
	 	if(res == 0) return 0; //socket disconnected
	 	else if(res == -1) {
	 		if(errno != EINTR) return -1; //write failure
	 	} else rem -= res;
	 }
  return count;
}
