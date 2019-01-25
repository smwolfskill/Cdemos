/**
 * Chatroom Lab
 * CS 241 - Fall 2016
 */

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "chat_window.h"
#include "utils.h"

static volatile int serverSocket;
static pthread_t threads[2];
static struct addrinfo * result;

void *write_to_server(void *arg);
void *read_from_server(void *arg);
void close_program(int signal);

/**
 * Clean up for client
 * Called by close_program upon SIGINT
 */
void close_client() {
  // Cancel the running threads
  fprintf(stderr, "CLOSE SIGNAL\n");
  pthread_cancel(threads[0]);
  pthread_cancel(threads[1]);
  // TODO: Any other cleanup code goes here!
	freeaddrinfo(result);
	close(serverSocket);
	//destroy_windows();
}

/**
 * Sets up a connection to a chatroom server and begins
 * reading and writing to the server.
 *
 * host     - Server to connect to.
 * port     - Port to connect to server on.
 * username - Name this user has chosen to connect to the chatroom with.
 */
void run_client(const char *host, const char *port, const char *username) {
//TCP Client: 3 steps: getaddrinfo, socket, connect
  //1. Set up the connection to (host) on (port)
  /*QUESTION 4, 5, 6*/
	errno = 0;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints)); //make sure no garbage
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	int g = getaddrinfo(host, port, &hints, &result);
	if(g != 0) { destroy_windows(); fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(g)); exit(1); }
  /*QUESTION 1, 2, 3*/
	serverSocket = socket(result->ai_family, result->ai_socktype, IPPROTO_TCP); //create endpoint for IPv4 TCP conn.
	if(serverSocket == -1) { perror(NULL); exit(1); }
  /*QUESTION 7*/
	//printf("run_client(): attempting connection...\n");
	int ok = connect(serverSocket, result->ai_addr, result->ai_addrlen);
	if(ok == -1) { destroy_windows(); perror(NULL); exit(1); }
  //2. Start the 2 threads to read/write to server:
	//printf("run_client(): connection success. Starting threads...\n");
	if(pthread_create(&threads[0], NULL, read_from_server, NULL) != 0) { destroy_windows(); perror(NULL); exit(1); }
	if(pthread_create(&threads[1], NULL, write_to_server, (void*)username) != 0) { destroy_windows(); perror(NULL); exit(1); }
	//printf("run_client(): threads created\n");
	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);
}

typedef struct _thread_cancel_args {
  char **buffer;
  char **msg;
} thread_cancel_args;

/**
 * Cleanup routine in case the thread gets cancelled
 * Ensure buffers are freed if they point to valid memory
 */
void thread_cancellation_handler(void *arg) {
  printf("Cancellation handler\n");
  thread_cancel_args *a = (thread_cancel_args *)arg;
  char **msg = a->msg;
  char **buffer = a->buffer;
  if (*buffer) {
    free(*buffer);
    *buffer = NULL;
  }
  if (msg && *msg) {
    free(*msg);
    *msg = NULL;
  }
}

/**
 * Reads bytes from user and writes them to server
 *
 * arg - void* casting of char* that is the username of client
 */
void *write_to_server(void *arg) {
  char *name = (char *)arg;
  char *buffer = NULL;
  char *msg = NULL;
  ssize_t retval = 1;

  thread_cancel_args cancel_args;
  cancel_args.buffer = &buffer;
  cancel_args.msg = &msg;
  // Setup thread cancellation handlers
  // Read up on pthread_cancel, thread cancellation states, pthread_cleanup_push
  // for more!
  pthread_cleanup_push(thread_cancellation_handler, &cancel_args);

	//printf("write_to_server: started...\n");

  while (retval > 0) {
	//printf("write_to_server: another iteration...\n");
    read_message_from_screen(&buffer);
    if (buffer == NULL) {
    	//fprintf(stderr, "read_message_from_screen: caused NULL buffer!\n");
    	break;
    }

    msg = create_message(name, buffer);
    size_t len = strlen(msg) + 1;

    retval = write_message_size(len, serverSocket);
    if (retval > 0) {
      retval = write_all_to_socket(serverSocket, msg, len);
      //printf("(wrote %zd bytes)\n", retval);
    } //else fprintf(stderr, "write_to_server: write_message_size failed %zd!\n", retval);

    free(msg);
    msg = NULL;
  }

	//printf("write_to_server: done!\n");

  pthread_cleanup_pop(0);
  return 0;
}

/**
 * Reads bytes from the server and prints them to the user.
 *
 * arg - void* requriment for pthread_create function
 */
void *read_from_server(void *arg) {
  // Silence the unused parameter warning
  (void)arg;
  ssize_t retval = 1;
  char *buffer = NULL;
  thread_cancel_args cancellation_args;
  cancellation_args.buffer = &buffer;
  cancellation_args.msg = NULL;
  pthread_cleanup_push(thread_cancellation_handler, &cancellation_args);

	//printf("read_from_server: started...\n");

  while (retval > 0) {
	//printf("read_from_server: another iteration...\n");
    retval = get_message_size(serverSocket);
    if (retval > 0) {
      buffer = calloc(1, retval);
      retval = read_all_from_socket(serverSocket, buffer, retval);
    } //else fprintf(stderr, "read_from_server: get_message_size failed %zd!\n", retval);
    if (retval > 0)
      write_message_to_screen("%s\n", buffer);
    //else fprintf(stderr, "read_from_server: read_all_from_socket failed %zd!\n", retval);

    free(buffer);
    buffer = NULL;
  }

	//printf("read_from_server: done!\n");

  pthread_cleanup_pop(0);
  return 0;
}

/**
 * Signal handler used to close this client program.
 */
void close_program(int signal) {
  if (signal == SIGINT) {
    close_chat();
    close_client();
  }
}

int main(int argc, char **argv) {

  if (argc < 4 || argc > 5) {
    fprintf(stderr, "Usage: %s <address> <port> <username> [output_file]\n",
            argv[0]);
    exit(1);
  }

  char *output_filename;
  if (argc == 5) {
    output_filename = argv[4];
  } else {
    output_filename = NULL;
  }

  // Setup signal handler
  signal(SIGINT, close_program);
  create_windows(output_filename);
	//printf("calling run_client()...\n");
  run_client(argv[1], argv[2], argv[3]);

  destroy_windows();
	//printf("MAIN: done\n");
  return 0;
}
