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

#include <iostream>
using namespace std;






// Intended Space - Don't erase empty lines






/**********************
  Structs
***********************/
#define MAX_USER_NAME 30
#define MAX_ROOM_NAME 30
#define MAX_MSG_CHAR 50

#define MAX_ROOM_NUM 20

typedef struct User{
  char user_name[MAX_USER_NAME];
  int user_id;
  int room_id;
} User;
typedef struct Room{
  char room_name[MAX_ROOM_NAME];
  int room_id;
  User **user_list;
} Room;
typedef struct Message{
  char message[MAX_MSG_CHAR];
  int user_id;
} Message;





// Intended Space - Don't erase empty lines






/*******************
  GLOBAL VARIABLES 
********************/
/* Max text line length */
#define MAXLINE 8192

/* Second argument to listen() */
#define LISTENQ 1024


/* global structs */
Room **room_list;
User **user_list;
Message ***msg_list;

int num_room_list = 0;

/* Simplifies calls to bind(), connect(), and accept() */
typedef struct sockaddr SA;

// A lock for the message buffer.
pthread_mutex_t lock;
pthread_mutex_t room;

// We will use this as a simple circular buffer of incoming messages.
char message_buf[20][50];

// This is an index into the message buffer.
int msgi = 0;





// Intended Space - Don't erase empty lines






/********************************
  ROOM
*********************************/
int get_number_of_room_list(){
  return num_room_list;
}
int add_user(char *room_name, char *user_name){
  return 0;
}
int room_list(char *room_name){
  return 0;
}
int create_room(char *room_name, char *user_name){
  pthread_mutex_lock(&room);
  printf("\t[+]creating a room \"%s\".\n", room_name);
  int x = get_number_of_room_list();
  
  for (int i=0; i <=x; i++){
    if((*(room_list[i])).room_name==room_name){
      //join the room
      // User *user_list = (*(room_list[i])).user_list;

    }
    if(i==x)
    {
      if (x>=MAX_ROOM_NUM) return -1;
      strcpy((*(room_list[x])).room_name, room_name);
      (*(room_list[x])).room_id = x;
      //update user_list of the room
      num_room_list++;
      printf("\tsize of room_list: %d\n", get_number_of_room_list());
    }
  }
 
  
  pthread_mutex_unlock(&room);
  return num_room_list;
}

void init_rooms_users_messages(){
  room_list = NULL;
  user_list = NULL;
  msg_list = NULL;

  room_list = (Room **)malloc(sizeof(Room*) * MAX_ROOM_NUM);
  for(int i=0;i<MAX_ROOM_NUM;i++){
    room_list[i] = (Room *)malloc(sizeof(Room));
  }
  int r = create_room((char *)"Lobby","Administer");
}





// Intended Space - Don't erase empty lines






/****************************************
  INCOMING MESSAGES FROM CLIENTS
*****************************************/
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

// This function adds a message that was received to the message buffer.
// Notice the lock around the message buffer.
void add_message(char *buf) {
  pthread_mutex_lock(&lock);
  strncpy(message_buf[msgi % 20], buf, 50);
  int len = strlen(message_buf[msgi % 20]);
  message_buf[msgi % 20][len] = '\0';
  msgi++;
  pthread_mutex_unlock(&lock);
}

// Destructively modify string to be upper case
void upper_case(char *s) {
  while (*s) {
    *s = toupper(*s);
    s++;
  }
}

// A predicate function to test incoming message.
int is_Command_message(char *message) { 
  if(message[0]=='\\'){
    //printf("%s\n","it is Command" );
    return true;
  }
  else{
    return false;
  }

  // return strncmp(message, "-", 1) == 0;
}

int send_ROOM_message(int connfd) {
  char message[1024] = "ROOM";
  printf("Sending: %s \n", message);

  return send_message(connfd, message);
}

/* Command: \ROOMS */
// int send_roomlist_message(int connfd) {
//   char message[20 * 50] = "";
//   const char *temp_room_list[2];
//     temp_room_list[0] = "blah";
//     temp_room_list[1] = "hmm";
//   for (int i = 0; i < 2; i++) {
//     if (strcmp(temp_room_list[i], "") == 0) break;//room list
//     strcat(message, temp_room_list[i]);//room list
//     strcat(message, ",");
//   }

//   // End the message with a newline and empty. This will ensure that the
//   // bytes are sent out on the wire. Otherwise it will wait for further
//   // output bytes.
//   strcat(message, "\n\0");
//   printf("Sending: %s", message);

//   return send_message(connfd, message);
// }

int send_roomlist_message(int connfd){
  char *prefix = (char *)"Room [";
  char *suffix = (char *)"]: ";
  char list_buffer[(MAX_ROOM_NUM + 2) * (sizeof(prefix) + MAX_ROOM_NAME + 1)];
  bzero(list_buffer, sizeof(list_buffer));  // initialize list_buffer
  
  char *preprefix = (char *)"\nNumber of Rooms: ";
  char temp[10]; // buffer to save int values
  sprintf(temp, "%d", get_number_of_room_list());
  //temp[strlen(temp)] = '\n';
  //temp[9] = '\0';

  strcat(list_buffer, preprefix);

  strcat(list_buffer, temp);
  
  for(int i=0;i<num_room_list;i++){
    strcat(list_buffer, "\n");
    strcat(list_buffer, prefix);  // add prefix
    int temp_int = (*(room_list[i])).room_id;  // get room_id
    sprintf(temp, "%d", temp_int);  // convert room_id into chars
    strcat(list_buffer, temp); // add room_id chars
    strcat(list_buffer, suffix);  // add suffix
    strcat(list_buffer, (*(room_list[i])).room_name); // add room_name
    
  }
  printf("Sneding: %s\n", list_buffer);
  return send_message(connfd, list_buffer);
}


/* Command: \WHO */
int send_userlist_message(int connfd) {
  char message[20 * 50] = "";
  const char *temp_user_list[3];
    temp_user_list[0] = "shipeng";
    temp_user_list[1] = "ruifeng";
    temp_user_list[2] = "daniel";
  for (int i = 0; i < 3; i++) {
    if (strcmp(temp_user_list[i], "") == 0) break;//room list
    strcat(message, temp_user_list[i]);//room list
    strcat(message, ",");
  }

  // End the message with a newline and empty. This will ensure that the
  // bytes are sent out on the wire. Otherwise it will wait for further
  // output bytes.
  strcat(message, "\n\0");
  printf("Sending: %s", message);

  return send_message(connfd, message);
}
int send_helplist_message(int connfd) {
  char message[20 * 100] = "";
  const char *temp_user_list[6];
    temp_user_list[0] = "\\JOIN nickname room";
    temp_user_list[1] = "\\ROOMS";
    temp_user_list[2] = "\\LEAVE";
    temp_user_list[3] = "\\WHO";
    temp_user_list[4] = "\\nickname message";
    temp_user_list[5] = "\\HELP";
  for (int i = 0; i < 6; i++) {
    if (strcmp(temp_user_list[i], "") == 0) break;//room list
    strcat(message, temp_user_list[i]);//room list
    strcat(message, "\n");
  }

  // End the message with a newline and empty. This will ensure that the
  // bytes are sent out on the wire. Otherwise it will wait for further
  // output bytes.
  strcat(message, "\n\0");
  printf("Sending: %s", message);

  return send_message(connfd, message);
}

char *array[3];
void string_to_token(char buf[]){
  int i = 0;
    char *p = strtok (buf, " ");
    

    while (p != NULL)
    {
        array[i++] = p;
        p = strtok (NULL, " ");
    }

    // for (i = 0; i < 3; ++i) 
    //     printf("%s\n", array[i]);
}
int process_message(int connfd, char *message) {//idk if we can use case switch
  if (is_Command_message(message)) {
    printf("iT IS COMMAND.\n");
    // if(strcmp(message, "\\JOIN nickname room") == 0){ 
    if(strncmp(message, "\\JOIN",5) == 0){ 
      printf("%s\n","it is \\JOIN nickname room" );
      string_to_token(message);
      printf("\n the room name is %s", array[2]);
      
    // Keep printing tokens while one of the 
    // delimiters present in str[]. 
   
      if(create_room((char *)array[2],(char *)array[3]) == -1){
        return send_message(connfd, (char *)"Error creating a room");
      }
      
      // Checklist:
      //  if the command starts with JOIN done
      //  if the number of room does not exceed MAX_ROOM_NUM done
      //    change user's name done 
      //  else done 
      //    return error message done
      //  if the room name exists, 
      //    join the room
      //  else
      //    create a new room with the name
      //  update user_list of the room
      //  return success message
      return send_message(connfd, (char *)"Created a room");
    }
    else if(strcmp(message, "\\ROOMS") == 0){
            printf("%s\n","it is \\ROOMS" );
            // printf("%s\n", get_room_list());
            return send_roomlist_message(connfd);
    }
    else if(strcmp(message, "\\LEAVE") == 0){
      char message[1024] = "GoodBye";
      return send_message(connfd, message);

    }
    else if(strcmp(message, "\\WHO") == 0){
          printf("%s\n","it is \\WHO" );
          return send_userlist_message(connfd);
    }
    else if(strcmp(message, "\\HELP") == 0){
          printf("%s\n","it is \\HELP" );
          return send_helplist_message(connfd);
    }
    else if(strcmp(message, "\\nickname message") == 0){

    }
    else{
      char tempMessage[1024] =" command not recognized";
      strcat(message,tempMessage);
      printf("%s\n ", message );
      return send_message(connfd,message);
    }
    return send_ROOM_message(connfd);
  } 

    else {
      printf("Server responding with echo response.\n");
      return send_message(connfd, message);
    }
}

void simple_message(int connfd){
  size_t n;
  char message[MAXLINE];

  while((n=receive_message(connfd, message))>0) {
    // message[n] = '\0';  // null terminate message (for string operations)
    printf("Server received a meesage of %d bytes: %s\n", (int)n, message);
    n = process_message(connfd, message);
    bzero(message, sizeof(message));  // reintialize the message[] buffer
  }

}






// Intended Space - Don't erase empty lines







/***************
  SERVER PART
****************/
// Helper function to establish an open listening socket on given port.
int open_listenfd(int port) {
  int listenfd;    // the listening file descriptor.
  int optval = 1;  //
  struct sockaddr_in serveraddr;

  /* Create a socket descriptor */
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;
  printf("[+]Server Socket is created.\n");

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
  printf("[+]Server Socket is binded to the server address.\n");

  /* Make it a listening socket ready to accept connection requests */
  if (listen(listenfd, LISTENQ) < 0) return -1;
  printf("[+]Server Socket is ready to listening connection.......\n");
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
  pthread_mutex_init(&room, NULL);

  // The port number for this server.
  int port = atoi(argv[1]);

  // The listening file descriptor.
  int listenfd = open_listenfd(port);

  // Initialize ROOMS, USERS, MESSAGES
  init_rooms_users_messages();

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
