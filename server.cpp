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
#define MAX_MSG_CHAR 100

#define MAX_ROOM_NUM 20
#define MAX_USER_IN_A_ROOM 20
#define MAX_CLIENTS 100

#define DEFAULT_USR_NAME "ChangeName"

typedef struct User{
  char user_name[MAX_USER_NAME];
  int socket;
  int room_id;
  // int user_id;//user id is not needed anymore. since we decide to make a unique nickname.
} User;
typedef struct Room{
  char room_name[MAX_ROOM_NAME];
  int room_id;
  int num_users;
  int socket_list_in_Room[MAX_USER_IN_A_ROOM];
  // User *user_list[MAX_USER_IN_A_ROOM];// we do not need a double pointers for user list
  // a array of user object is totally enough, because we do not need to let server have all the user infomation.
} Room;
typedef struct Message{
  char message[MAX_MSG_CHAR];
  int socket;
  // int user_id;//we will use nickname instead.
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
// Room **room_list;
// User **user_list;
// Message ***msg_list;
Room Room_list[MAX_ROOM_NUM];
User User_list[MAX_CLIENTS];

int num_room_list = 0;
int *unique_user_id_set = (int *)malloc(sizeof(int) * MAX_USER_IN_A_ROOM * MAX_ROOM_NUM);//maybe no more need.
bool *unique_user_id_set_mark = (bool *)malloc(sizeof(bool) * MAX_USER_IN_A_ROOM * MAX_ROOM_NUM);//maybe no more need.
// int volatile unique_user_id_set[MAX_USER_IN_A_ROOM * MAX_ROOM_NUM];
// bool volatile unique_user_id_set_mark[MAX_USER_IN_A_ROOM * MAX_ROOM_NAME];

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
  SOCKETS
********************************/
int volatile socket_list[MAX_CLIENTS];  // global variable for storing sockets
void initialize_sockets(){  // initialize all the socket by -1 to indicate it is empty
  for(int i=0; i<MAX_CLIENTS; i++){
    socket_list[i] = -1;
  }
}
void print_sockets(){ // print current connected sockets
  printf("--- socket list ---\n");
  for(int i=0; i<MAX_CLIENTS; i++){
    if(socket_list[i] != -1)
      printf("socket[%d]:%d\n", i, socket_list[i]);
  }
  printf("-------------------\n");
}
void add_sockets(int connfd){ // add new socket into current socket list. Sockets are filled from left to right
  for(int i=0; i<MAX_CLIENTS; i++){
    if( socket_list[i] != -1) { continue; }
    else {
      socket_list[i] = connfd;
      break;
    }
  }
}
void delete_socket(int connfd){ // find a matching socket and delete it.
  for(int i=0; i<MAX_CLIENTS; i++){
    if( socket_list[i] == connfd) {
      socket_list[i] = -1;
      break;
    }
  }
}






// Intended Space - Don't erase empty lines





Room room_list_test[MAX_USER_IN_A_ROOM];

/********************************
  ROOM
*********************************/
int get_number_of_room_list(){ //room list
  return num_room_list;
}
int increare_number_of_roomlist(){
  num_room_list++;  
}
int get_User_list_index_by_socket(int connfd){
  for(int i=0; i<MAX_CLIENTS; i++){
    if(User_list[i].socket == connfd){
      return i;   // return index number of corresponding socket(client)
    }
  }
  return -1;  // that socket(client) is not existing
}
int add_new_User_in_User_list(User user){
  int i = 0;
  for(i=0; i<MAX_CLIENTS; i++){
    if(User_list[i].socket!=-1){
      continue;
    } else{
      User_list[i] = user;
      return i; // successfully added user into User_list and return index
    }
  }
  return -1;
}
int add_User_in_existing_Room(int connfd, char *nickname, int room_id){

//   typedef struct Room{
//   char room_name[MAX_ROOM_NAME];
//   int room_id;
//   int num_users;
//   int socket_list[MAX_USER_IN_A_ROOM];
// } Room;

  // Room newRoom = get_User_by_socket(connfd);
  // Room newRoom = get_Room_by_roomname();
  int n = Room_list[room_id].num_users;
  if (n >= MAX_USER_IN_A_ROOM) {
    return -1;  // there is already MAX users in the room
  }
  Room_list[room_id].socket_list_in_Room[n] = connfd;
  Room_list[room_id].num_users++;
  int idx = get_User_list_index_by_socket(connfd);
  if (idx != -1) {
    bzero(User_list[idx].user_name, MAX_USER_NAME);
    strcmp(User_list[idx].user_name, nickname);
  } else {
//     typedef struct User{
//   char user_name[MAX_USER_NAME];
//   int socket;
//   int room_id;
//   // int user_id;//user id is not needed anymore. since we decide to make a unique nickname.
// } User;

    User newUser;
    bzero(newUser.user_name, MAX_USER_NAME);
    strcmp(newUser.user_name, nickname);
    newUser.socket = connfd;
    newUser.room_id = room_id;
    add_new_User_in_User_list(newUser);
  }
  // if user_list_idx != -1
    // then it is existing User, so update its user_name
    // User_list[user_list_idx].user_name = nickname;
  // else
  //    create new User

  return 1;
  // // newRoom.room_name = (char *)"Du Bois Library";
  // strcpy(newRoom.room_name, (char *)"Du Bois Library");
  // newRoom.room_id = room_id;
  // newRoom.num_users = 1;
  // newRoom.socket_list[0] = user.socket;
  // room_list_test[0] = 
}
int add_user(char *room_name, User *user){// add a user, if the room existed , and doesnot have same name
  int x = get_number_of_room_list();

  
  // for (int i=0; i <=x; i++){
    // if((*(room_list[i])).room_name==room_name){

      //join the room
      // User *user_list = (*(room_list[i])).user_list;

    // }
  // }
  return 0;
}
int get_room_userlist(char *room_name){//printout list of user in this room, can just printout nicknames.
  //since room id isnot needed.
  return 0;
}
// int create_room_backup(char *room_name, char *user_name){// create room if room is not existed, and add the user.
//   //if room is existed and not full , we add the user.
//   pthread_mutex_lock(&room);
//   printf("\t[+]creating a room \"%s\".\n", room_name);
//   int x = get_number_of_room_list();
  
//   for (int i=0; i <=x; i++){
//     if((*(room_list[i])).room_name==room_name){
//       //join the room
//       // User *user_list = (*(room_list[i])).user_list;

//     }
//     if(i==x)
//     {
//       if (x>=MAX_ROOM_NUM) return -1;
//       strcpy((*(room_list[x])).room_name, room_name);
//       (*(room_list[x])).room_id = x;
//       //update user_list of the room
//       num_room_list++;
//       printf("\tsize of room_list: %d\n", get_number_of_room_list());
//     }
//   }
 
  
//   pthread_mutex_unlock(&room);
//   return num_room_list;
// }
int create_room(int connfd, char *nickname, char *room_name){// create room if room is not existed, and add the user.
  //if room is existed and not full , we add the user.
  pthread_mutex_lock(&room);
  printf("\t[+]creating a room \"%s\".\n", room_name);
  int x = get_number_of_room_list();

//   typedef struct Room{
//   char room_name[MAX_ROOM_NAME];
//   int room_id;
//   int num_users;
//   int socket_list[MAX_USER_IN_A_ROOM];
// } Room;

  Room newRoom;
  strcpy(newRoom.room_name, room_name);
  newRoom.room_id = x;
  newRoom.num_users = 1;
  newRoom.socket_list_in_Room[0] = connfd;
  Room_list[x] = newRoom;
  increare_number_of_roomlist();
  pthread_mutex_unlock(&room);
  return num_room_list;
}
int is_room_name_existing(char *room_name){
  for(int i=0; i<get_number_of_room_list(); i++){
    if (strcmp(Room_list[i].room_name, room_name) == 0 ){ // there is an existing room with the same name
      printf("Existing a room with the name %s\n", room_name);
      return Room_list[i].room_id; // return room_id
    }
  }
  printf("No such name of room existing %s\n", room_name);
  return -1;  // there is no room with the same name
}
int JOIN_Nickname_Room(int connfd, char *nickname, char *room_name){// create room if room is not existed, and add the user.
  //if room is existed and not full , we add the user.
  // pthread_mutex_lock(&room);
  // check 
  // if there is existing room with room_name
  //    add_user_existing_room()
  // else
  //    if number of room >= MAX
  //        return error
  //    create_room()
  // return success

  if(int r_id = is_room_name_existing(room_name) > -1){  // if there is existing room with same name
    add_User_in_existing_Room(connfd, nickname, r_id);
  } else {  // nope? then we need to create a new room
    if (get_number_of_room_list() >= MAX_ROOM_NUM) { return -1; }   // if number of room is full
    create_room(connfd, nickname, room_name);
  }
  // pthread_mutex_unlock(&room);
}

void init_Rooms_Users_Messages(){ 
  // room_list = NULL;
  // user_list = NULL;
  // msg_list = NULL;
// typedef struct User{
//   char user_name[MAX_USER_NAME];
//   int socket;
//   int room_id;
//   // int user_id;//user id is not needed anymore. since we decide to make a unique nickname.
// } User;
// typedef struct Room{
//   char room_name[MAX_ROOM_NAME];
//   int room_id;
//   int num_users;
//   int socket_list_in_Room[MAX_USER_IN_A_ROOM];
//   // User *user_list[MAX_USER_IN_A_ROOM];// we do not need a double pointers for user list
//   // a array of user object is totally enough, because we do not need to let server have all the user infomation.
// } Room;
// typedef struct Message{
//   char message[MAX_MSG_CHAR];
//   int socket;
//   // int user_id;//we will use nickname instead.
// } Message;

  User initUser;
  initUser.user_name[0] = '\0';
  initUser.socket = -1;
  initUser.room_id = -1;
  for(int i=0; i<MAX_CLIENTS; i++){
    User_list[i] = initUser;
  }

  Room initRoom;
  initRoom.room_name[0] = '\0';
  initRoom.room_id = -1;
  initRoom.num_users = 0;
  initRoom.socket_list_in_Room[0] = -1;
  for(int i=0; i<MAX_ROOM_NUM; i++){
    Room_list[i] = initRoom;
  }



  // Initialize room_list
  // room_list = (Room **)malloc(sizeof(Room*) * MAX_ROOM_NUM);
  // for(int i=0;i<MAX_ROOM_NUM;i++){
    // room_list[i] = (Room *)malloc(sizeof(Room));
    // Room_list[i] = 0;
  // }
  // int r = JOIN_Nickname_Room((char *)"Lobby",(char *)"Administer"); // create the first room with the name "Lobby"

  // Initialize unique_user_id
  // for(int i=0; i<MAX_USER_IN_A_ROOM*MAX_ROOM_NUM; i++){ //we may not need this part.
  //   unique_user_id_set_mark[i] = false;
  //   unique_user_id_set[i] = i;
  // }
  // for(int i=0; i<MAX_USER_IN_A_ROOM*MAX_ROOM_NUM; i++){
  //   printf("\t%d",unique_user_id_set[i]);
  // }
}
bool check_user_in_room(int room_id, int user_id){ // we can simply this function, since we do not need pointer.
  // for(int i=0; i<(*(room_list[room_id])).num_users; i++){
  //   if ( (*(*(room_list[room_id])).user_list[i]).user_id == user_id) {
  //     return true;
  //   }
  // }
  return false;
}
int get_unique_user_id(){// we may not need this function anymore.
  // printf("%p\n", unique_user_id_set);
  for(int i=0; i<MAX_USER_IN_A_ROOM*MAX_ROOM_NUM; i++){
    if(unique_user_id_set_mark[i] == false) {
      unique_user_id_set_mark[i] == true;
      // printf("ha %p\n", unique_user_id_set);
      // printf("ha %d %d %d\n", unique_user_id_set[0], unique_user_id_set[1], unique_user_id_set[2]);
      // printf("ha %d %d %d\n", unique_user_id_set_mark[0], unique_user_id_set_mark[1], unique_user_id_set_mark[2]);
      return unique_user_id_set[i];
    }
  }
  return -1;  // if all user_id is used
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
int send_JOIN_message(int connfd){

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

// int send_roomlist_message_backup(int connfd){
//   char *prefix = (char *)"Room [";
//   char *suffix = (char *)"]: ";
//   char list_buffer[(MAX_ROOM_NUM + 2) * (sizeof(prefix) + MAX_ROOM_NAME + 1)];
//   bzero(list_buffer, sizeof(list_buffer));  // initialize list_buffer
  
//   char *preprefix = (char *)"\nNumber of Rooms: ";
//   char temp[10]; // buffer to save int values
//   sprintf(temp, "%d", get_number_of_room_list());
//   //temp[strlen(temp)] = '\n';
//   //temp[9] = '\0';

//   strcat(list_buffer, preprefix);

//   strcat(list_buffer, temp);

//   for(int i=0;i<num_room_list;i++){
//     strcat(list_buffer, "\n");
//     strcat(list_buffer, prefix);  // add prefix
//     int temp_int = (*(room_list[i])).room_id;  // get room_id
//     sprintf(temp, "%d", temp_int);  // convert room_id into chars
//     strcat(list_buffer, temp); // add room_id chars
//     strcat(list_buffer, suffix);  // add suffix
//     strcat(list_buffer, (*(room_list[i])).room_name); // add room_name
    
//   }
//   printf("Sneding: %s\n", list_buffer);
//   return send_message(connfd, list_buffer);
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
    int temp_int = Room_list[i].room_id;  // get room_id
    sprintf(temp, "%d", temp_int);  // convert room_id into chars
    strcat(list_buffer, temp); // add room_id chars
    strcat(list_buffer, suffix);  // add suffix
    strcat(list_buffer, Room_list[i].room_name); // add room_name
    
  }
  printf("Sneding: %s\n", list_buffer);
  return send_message(connfd, list_buffer);
}


/* Command: \WHO */
int send_userlist_message(int connfd) { // we call help function which is user-list.(get a list of user nickname.)
  //for this function ,we can just fomatting the list and send the message to client who send the command.

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

int send_helplist_message(int connfd) { // this part do not need help list.

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

char *array[3];// for string to token
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
    printf("Received a Command: ");
    // if(strcmp(message, "\\JOIN nickname room") == 0){ 
    if(strncmp(message, "\\JOIN",5) == 0){ 
      printf("%s\n","it is \\JOIN nickname room" );
      string_to_token(message);//convert to tokens
      //array[1] is nickname
      //array[2] is room name
      printf("\n the room name is %s", array[2]);
      printf("nickname:%s\n", array[1]);
      //check the user existed or not 
      //if not , just create a new user
      //else pass the user into create room function.
      
    
   
      if(JOIN_Nickname_Room(connfd, (char *)array[1],(char *)array[2]) == -1){
        return send_message(connfd, (char *)"Error creating a room");
      }
      
      
      return send_message(connfd, (char *)"Created a room");
    }
    else if(strcmp(message, "\\ROOMS") == 0){//this part is fine
            printf("%s\n","it is \\ROOMS" );
            // printf("%s\n", get_room_list());
            return send_roomlist_message(connfd);
    }
    else if(strcmp(message, "\\LEAVE") == 0){//this part is fine
      char message[1024] = "GoodBye";
      return send_message(connfd, message);

    }
    else if(strcmp(message, "\\WHO") == 0){//this part is fine
          printf("%s\n","it is \\WHO" );
          return send_userlist_message(connfd);
    }
    else if(strcmp(message, "\\HELP") == 0){//this part is fine
          printf("%s\n","it is \\HELP" );
          return send_helplist_message(connfd);
    }
    else if(strcmp(message, "\\nickname message") == 0){//this part will be in a same room and whisper by nickname
      //if you can not find the nickname then show it user not existed. some thing like this. much easy.

    }
    else{
      char tempMessage[1024] =" command not recognized";//this part is fine
      strcat(message,tempMessage);
      printf("%s\n ", message );
      return send_message(connfd,message);
    }
    return send_ROOM_message(connfd);
  } 

    else {//this part is fine
      printf("Server responding with echo response.\n");
      return send_message(connfd, message);
    }
}

void simple_message(int connfd){
  size_t n;
  char message[MAXLINE];
  // User *defaultUser = (User *)malloc(sizeof(User));
  // (*defaultUser).user_id = get_unique_user_id();
  // printf("This User's id: %d\n", (*defaultUser).user_id);

  User defaultUser;
  strncpy(defaultUser.user_name, DEFAULT_USR_NAME, sizeof(DEFAULT_USR_NAME));
  defaultUser.socket = connfd;
  defaultUser.room_id = 0;

  JOIN_Nickname_Room(connfd, (char *)DEFAULT_USR_NAME, (char *)"Lobby");

  // create_room(connfd, (char *)DEFAULT_USR_NAME, (char *)"Lobby");
  // add_User_in_existing_Room(connfd, (char *)DEFAULT_USR_NAME, 0);  

  while((n=receive_message(connfd, message))>0) {
    // message[n] = '\0';  // null terminate message (for string operations)
    printf("From socket[%d]: Server received a meesage of %d bytes: %s\n", connfd, (int)n, message);
    n = process_message(connfd, message);
    bzero(message, sizeof(message));  // reintialize the message[] buffer
  }
  // free(defaultUser);
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

int main(int argc, char *argv[]){// we need help function before we call the thread.
  //we need let user change name first. the default name can a random string by your choose,
  // but once user connected, we create a user for this client and ask a name from this client.
  // check if it is repeat. this will all do in a help function.
  
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
  init_Rooms_Users_Messages();

  initialize_sockets();
  print_sockets();
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

    printf("[+]Server connected to %s (%s), port %u, socket[%d]\n", hp->h_name, haddrp,
           client_port, *connfdp);

    // Create a new thread to handle the connection.
    add_sockets(*connfdp);
    // printf("\t\t sockets[] array at:%p \t sockets[0]:%d \t *(sockets[0]):%d \n", sockets, sockets[0], (sockets[0]));
    // printf("\t\t\t\t\t\t connfdp: %p \t *connfdp: %d\n", connfdp, *connfdp);
    pthread_t tid;
    pthread_create(&tid, NULL, thread, connfdp);
  }

  return 0;
}

/* thread routine */
void *thread(void *vargp) {
  // printf("%p\n", unique_user_id_set);
  // printf("%d %d %d\n", unique_user_id_set[0], unique_user_id_set[1], unique_user_id_set[2]);
  // printf("%d %d %d\n", unique_user_id_set_mark[0], unique_user_id_set_mark[1], unique_user_id_set_mark[2]);
  // Grab the connection file descriptor.
  int connfd = *((int *)vargp);
  // Detach the thread to self reap.
  pthread_detach(pthread_self());
  // Free the incoming argument - allocated in the main thread.
  free(vargp);
  // Handle the echo client requests.
  printf("[+]New Thread created with the socket [%d]\n", connfd);
  print_sockets();
  simple_message(connfd);
  printf("[-]Client with socket [%d] disconnected.\n", connfd);
  delete_socket(connfd);
  print_sockets();
  // Don't forget to close the connection!
  close(connfd);
  return NULL;
}
