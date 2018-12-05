#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "srd_server.h"

Room **room_list;
User **user_list;
Message ***msg_list;

/* Simplifies calls to bind(), connect(), and accept() */
typedef struct sockaddr SA;

// A lock for the message buffer.
pthread_mutex_t lock;

/* Max text line length */
#define MAXLINE 8192

/* Second argument to listen() */
#define LISTENQ 1024

// We will use this as a simple circular buffer of incoming messages.
char message_buf[20][50];

// Initialize the message buffer to empty strings.
void init_message_buf() {
  int i;
  for (i = 0; i < 20; i++) {
    strcpy(message_buf[i], "");
  }
}

// A wrapper around recv to simplify calls.
int receive_message(int connfd, char *message) {
  return recv(connfd, message, MAXLINE, 0);
}

// A wrapper around send to simplify calls.
int send_message(int connfd, char *message) {
  return send(connfd, message, strlen(message), 0);
}

// A predicate function to test incoming message.
int is_ROOM_message(char *message) { return strncmp(message, "-", 1) == 0; }

int send_ROOM_message(int connfd) {
	char message[1024] = "ROOM";
  printf("Sending: %s", message);

  return send_message(connfd, message);
}

int process_message(int connfd, char *message) {
  if (is_ROOM_message(message)) {
    printf("Server responding with ROOM response.\n");
    return send_ROOM_message(connfd);
  } 
  // else {
  //   printf("Server responding with echo response.\n");
  //   return send_echo_message(connfd, message);
  // }
}

void simple_message(int connfd){
	size_t n;
	char message[MAXLINE];

	printf("SIMPLE_MESSAGE\n");
	while((n=receive_message(connfd, message))>0) {
	   message[n] = '\0';  // null terminate message (for string operations)
		printf("Server received a meesage of %dbytes: %s\n", (int)n, message);
		n = process_message(connfd, message);
	}
	printf("SIMPLE_MESSAGE  finish\n");
}

// Helper function to establish an open listening socket on given port.
int open_listenfd(int port) {
  int listenfd;    // the listening file descriptor.
  int optval = 1;  //
  struct sockaddr_in serveraddr;

  /* Create a socket descriptor */
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;

  /* Eliminates "Address already in use" error from bind */
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
                 sizeof(int)) < 0)
    return -1;

  /* Listenfd will be an endpoint for all requests to port
     on any IP address for this host */
  bzero((char *)&serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);
  if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0) return -1;

  /* Make it a listening socket ready to accept connection requests */
  if (listen(listenfd, LISTENQ) < 0) return -1;
  return listenfd;
}

// thread function prototype as we have a forward reference in main.
void *thread(void *vargp);

int main(int argc, char *argv[]){
  // Check the program arguments and print usage if necessary.
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  // initialize message buffer.
  init_message_buf();

  // Initialize the message buffer lock.
  pthread_mutex_init(&lock, NULL);

  // The port number for this server.
  int port = atoi(argv[1]);

  // The listening file descriptor.
  int listenfd = open_listenfd(port);

  while(1){
    // The connection file descriptor.
    int *connfdp = (int *)malloc(sizeof(int));

    // The client's IP address information.
    struct sockaddr_in clientaddr;

    // Wait for incoming connections.
    socklen_t clientlen = sizeof(struct sockaddr_in);
    *connfdp = accept(listenfd, (SA *)&clientaddr, &clientlen);

    /* determine the domain name and IP address of the client */
    struct hostent *hp =
        gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                      sizeof(clientaddr.sin_addr.s_addr), AF_INET);

    // The server IP address information.
    char *haddrp = inet_ntoa(clientaddr.sin_addr);

    // The client's port number.
    unsigned short client_port = ntohs(clientaddr.sin_port);

    printf("server connected to %s (%s), port %u\n", hp->h_name, haddrp,
           client_port);

    // Create a new thread to handle the connection.
    pthread_t tid;
    pthread_create(&tid, NULL, thread, connfdp);
  }

	return 0;
}

/* thread routine */
void *thread(void *vargp) {
  // Grab the connection file descriptor.
  int connfd = *((int *)vargp);
  // Detach the thread to self reap.
  pthread_detach(pthread_self());
  // Free the incoming argument - allocated in the main thread.
  free(vargp);
  // Handle the echo client requests.
  simple_message(connfd);
  printf("client disconnected.\n");
  // Don't forget to close the connection!
  close(connfd);
  return NULL;
}
