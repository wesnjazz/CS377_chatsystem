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
#define MAX_CLIENTS 100
#define MAX_ROOM_NUM 20
#define MAX_USER_IN_A_ROOM 20

#define MAX_USER_NAME 15
#define MAX_ROOM_NAME 30
#define MAX_MSG_CHAR 100

#define BUF_MAX_20LINES 20
#define BUF_MAX_100CHARS 100
#define BUF_SMALL_300 300

#define DEFAULT_USR_NAME "ChangeName"

typedef struct User{
  // User's nickname
  char user_name[MAX_USER_NAME];
  // User's socket
  int socket;
  // User's current location
  int room_id;
} User;
typedef struct Room{
  // Room name
  char room_name[MAX_ROOM_NAME];
  // room_id is matched with corresponding index of Room_list[],
  // e.g., Room number 0 has room_id of 0 and it is at Room_list[0]
  int room_id;
  // keep track a number of current clients in a specific Room
  int num_users;
  // Client list in a specific Room
  int socket_list_in_Room[MAX_USER_IN_A_ROOM];
  // Circular buffer which records all messages from clients in a specific Room
  char chat_buffer[BUF_MAX_20LINES][BUF_MAX_100CHARS];
  // HEAD pointer of chat_buffer
  int chat_buffer_HEAD;
  // TAIL pointer of chat_buffer
  int chat_buffer_TAIL;
} Room;
typedef struct Message{
  char message[MAX_MSG_CHAR];
  int socket;
} Message;





// Intended Space - Don't erase empty lines






/*******************
  GLOBAL VARIABLES 
********************/
/* Max text line length */
#define MAXLINE 8192

/* Second argument to listen() */
#define LISTENQ 1024

Room Room_list[MAX_ROOM_NUM];
User User_list[MAX_CLIENTS];

int num_room_list = 0;
int num_user_list = 0;

/* Simplifies calls to bind(), connect(), and accept() */
typedef struct sockaddr SA;

// A lock for the message buffer.
pthread_mutex_t lock;
pthread_mutex_t room;

// We will use this as a simple circular buffer of incoming messages.
char message_buf[BUF_MAX_20LINES][BUF_MAX_100CHARS];
char entire_message_buf[BUF_MAX_20LINES * BUF_MAX_100CHARS];

// This is an index into the message buffer.
int msgi = 0;


// Initialize the message buffer to empty strings.
void init_entire_message_buf() {
  bzero(entire_message_buf, sizeof(entire_message_buf));
}
int init_chat_buffer_in_Room(int room_id);
void init_Rooms_Users_Messages(){ 
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
  for(int i=0; i<MAX_USER_IN_A_ROOM; i++){
    initRoom.socket_list_in_Room[i] = -1;
  }
  for(int i=0; i<BUF_MAX_20LINES; i++){
    bzero(initRoom.chat_buffer[i], BUF_MAX_100CHARS);
  }
  initRoom.chat_buffer_HEAD = 0;
  initRoom.chat_buffer_TAIL = 0;
  for(int i=0; i<MAX_ROOM_NUM; i++){
    Room_list[i] = initRoom;
    init_chat_buffer_in_Room(i);
  }
}
void print_Room(int r_id){
  printf("Room_list[%d] - room_name: %s  -  room_id: %d  -  num_users: %d\n",
          r_id, Room_list[r_id].room_name, Room_list[r_id].room_id, Room_list[r_id].num_users);
}
void print_User(int u_id){
  printf("User_list[%d] - user_name: %s  -   socket: %d  -  room_id: %d\n",
          u_id, User_list[u_id].user_name, User_list[u_id].socket, User_list[u_id].room_id);
}
void print_Room_list(){
  for(int r_id=0; r_id<MAX_ROOM_NUM; r_id++){
    // printf("Room[%d]: %s\n", i, Room_list[i].room_name);
    printf("Room_list[%d] - room_name: %s  -  room_id: %d  -  num_users: %d\n",
        r_id, Room_list[r_id].room_name, Room_list[r_id].room_id, Room_list[r_id].num_users);
  }
}
void print_Room_socket_list(int r_id){
  for(int i=0; i<MAX_USER_IN_A_ROOM; i++){
    if(Room_list[r_id].socket_list_in_Room[i] != -1) {
      printf("%3d", Room_list[r_id].socket_list_in_Room[i]);
    }
  }
  printf("\n");
}






// Intended Space - Don't erase empty lines








/********************************
  CHAT_BUFFER (Circular Buffer)
********************************/
int add_title_into_chat_buffer(int room_id, int head){
  // chat_buffer[] will always have the title message which displays Room name and number at the HEAD
  // e.g., "---------- Room[11]: Bohemians ----------"
  // ***NOTE*** this buffer does not increase HEAD. Caller function should increase HEAD manually.
  char title[BUF_SMALL_300] = "---------- Room[";
  char buf[5]="";
  bzero(buf, sizeof(buf));
  sprintf(buf, "%d", room_id);
  strcat(title, buf);
  strcat(title, (char *)"]: ");
  strcat(title, Room_list[room_id].room_name);
  strcat(title, (char *)" ----------");
  
  strcpy(Room_list[room_id].chat_buffer[head], title);
}
int init_chat_buffer_in_Room(int room_id){
  /** initialize Chat_Buffer in a Room by writing zeros **/
  for(int i=0; i<BUF_MAX_20LINES; i++){
    bzero(Room_list[room_id].chat_buffer[i], BUF_MAX_100CHARS);
  }
  add_title_into_chat_buffer(room_id, 0); 
  Room_list[room_id].chat_buffer_HEAD = 0;
  Room_list[room_id].chat_buffer_TAIL = 0;
}
int get_User_list_index_by_socket(int connfd);
int add_message_into_chat_buffer(int connfd, int room_id, char *message){
  /**
    TAIL++
    if TAIL > max_index
      TAIL = 0
    if TAIL <= HEAD
      HEAD++
      if HEAD > max_index
        HEAD =0
      HEAD <- title msg
    buf[TAIL] = msg
  **/

  int tail = Room_list[room_id].chat_buffer_TAIL;
  int head = Room_list[room_id].chat_buffer_HEAD;
  message[BUF_MAX_100CHARS - MAX_USER_NAME - 3]='\0';

  printf("\t\t\t\t\t\t[head:%d tail:%d]\n", head, tail);
  if(head == tail){
    add_title_into_chat_buffer(room_id, head);
  }
  char prefix[BUF_MAX_100CHARS];
  bzero(prefix, sizeof(prefix));
  int user_idx = get_User_list_index_by_socket(connfd);
  strncpy(prefix, User_list[user_idx].user_name, MAX_USER_NAME);
  strcat(prefix, (char *)": ");

  tail++;
  if (tail >= BUF_MAX_20LINES){
    tail = 0;
  }
  if (tail <= head){
    head++;
    if (head >= BUF_MAX_20LINES){
      head = 0;
    }
    // strcpy(Room_list[room_id].chat_buffer[head], title);
    add_title_into_chat_buffer(room_id, head);
    Room_list[room_id].chat_buffer_HEAD = head;
  }
  strcat(prefix, message);
  printf("\tprefix: %s\n", prefix);
  strncpy(Room_list[room_id].chat_buffer[tail], prefix, BUF_MAX_100CHARS-1);
  Room_list[room_id].chat_buffer_TAIL = tail;
}
void get_chat_buffer(int room_id){
  // printf("\tget_chat_buffer() begin\n");
  char entire_message[BUF_MAX_20LINES * BUF_MAX_100CHARS];
  bzero(entire_message, sizeof(entire_message));

  int tail = Room_list[room_id].chat_buffer_TAIL;
  int head = Room_list[room_id].chat_buffer_HEAD;
  
  if(tail>head){
    for(int i=head; i<tail; i++){
      strcat(entire_message, Room_list[room_id].chat_buffer[i]);
    }
  }
  else{
    for(int i=head; i<BUF_MAX_20LINES; i++){
      strcat(entire_message, Room_list[room_id].chat_buffer[i]);
    }
    for(int i=0; i<=tail; i++){
      strcat(entire_message, Room_list[room_id].chat_buffer[i]);
    }
  }
  // printf("\tentire_message:\n\t%s\n", entire_message);
  // printf("\tget_chat_buffer() finish\n");
  // return entire_message;
}
void print_chat_buffer(int room_id){
  for(int i=0; i<BUF_MAX_20LINES; i++){
    printf("%3d: %s\n", i, Room_list[room_id].chat_buffer[i]);
  }
}
int create_entire_message_from_chat_buffer(int room_id){
  // printf("\tcreate_entire_message_from_chat_buffer() begin\n");
  // printf("\tbefore init: %s\n", entire_message_buf);
  init_entire_message_buf();
  // printf("\t after init: %s\n", entire_message_buf);
  int tail = Room_list[room_id].chat_buffer_TAIL;
  int head = Room_list[room_id].chat_buffer_HEAD;
  // printf("\t head: %d  tail: %d\n", head, tail);
  
  if(head==tail){
    strcat(entire_message_buf, Room_list[room_id].chat_buffer[head]);
    return 1;
  }
  if(tail>head){
    for(int i=head; i<=tail; i++){
      // printf("\t\tputting msg[%d]: %s\n", i, Room_list[room_id].chat_buffer[i]);
      strcat(entire_message_buf, Room_list[room_id].chat_buffer[i]);
      strcat(entire_message_buf, (char *)"\n");
    }
  }
  else{
    for(int i=head; i<BUF_MAX_20LINES; i++){
      // printf("\t\tputting msg[%d]: %s\n", i, Room_list[room_id].chat_buffer[i]);
      strcat(entire_message_buf, Room_list[room_id].chat_buffer[i]);
      strcat(entire_message_buf, (char *)"\n");
    }
    for(int i=0; i<=tail; i++){
      // printf("\t\tputting msg[%d]: %s\n", i, Room_list[room_id].chat_buffer[i]);
      strcat(entire_message_buf, Room_list[room_id].chat_buffer[i]);
      strcat(entire_message_buf, (char *)"\n");
    }
  }
  // printf("\tcreate_entire_message_from_chat_buffer() finish\n");
  return 1;
}



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







/********************************
  USER
*********************************/
int get_number_of_user_list(){
  return num_user_list;
}
void increase_number_of_user_list(){
  num_user_list++;
}
void decrease_number_of_user_list(){
  num_user_list--;
}
int get_User_list_index_by_socket(int connfd){  
  /** find an User index from User_list[] by comparing corresponding socket(client) **/
  for(int i=0; i<MAX_CLIENTS; i++){
    // printf("User_list[%d]: %s %d %d\n", i, User_list[i].user_name, User_list[i].socket, User_list[i].room_id);
    if(User_list[i].socket != -1 && User_list[i].socket == connfd){  // found the corresponding socket(client)
      return i;   // return index number of corresponding socket(client)
    }
  }
  return -1;  // that socket(client) is not existing
}
int find_empty_spot_in_User_list(){
  int i = 0;
  for(i=0; i<MAX_CLIENTS; i++){
    if(User_list[i].socket==-1){
      return i;
    }
  }
  return -1;
}
int create_new_User_at(int idx, int connfd, char *nickname, int room_id){
  if(get_number_of_user_list() > MAX_CLIENTS) { return -1; }
  strcpy(User_list[idx].user_name, nickname);
  User_list[idx].socket = connfd;
  User_list[idx].room_id = room_id;
  print_User(idx);
  increase_number_of_user_list();
  return idx;
}
// int add_User_in_User_list(User user){  // add an User into User_list[]
//   int idx = find_empty_spot_in_User_list();
//   create_new_User_at(idx, user.socket, user.user_name, user.user_id);
//   return idx;
//   // int i = 0;
//   // for(i=0; i<MAX_CLIENTS; i++){
//   //   if(User_list[i].socket!=-1){  // if this User slot is not empty
//   //     continue; // check next
//   //   } else { // this slot is empty
//   //     User_list[i] = user;  // add the user into User_list[]
//   //     return i; // successfully added user into User_list and return index
//   //   }
//   // }
//   // return -1; // couldn't find empty slot, or there was an error
// }
int find_empty_spot_socket_list_in_Room(int room_id);
int add_User_in_Room(int user_idx, int room_id){
  /***
    purpose:  adding a User into existing Room
    assumption: room_id of the existing Room is found,
                we do now know if there is an User with this socket(client), not by nickname(bcoz nickname will change at sometime)
  ***/
  int n = Room_list[room_id].num_users; // getting num of users in the corresponding Room
  if (n >= MAX_USER_IN_A_ROOM) {  // if number of maximum users exceeded,
    return -1;  // there is already MAX users in the room, return error(-1)
  } // there is an empty spot in the Room
  int socket_idx = find_empty_spot_socket_list_in_Room(room_id);
  Room_list[room_id].socket_list_in_Room[socket_idx] = User_list[user_idx].socket; // add the socket(client) into the user list of the Room
  Room_list[room_id].num_users++; // increase the number of users in the Room
  User_list[user_idx].room_id = room_id;
  // int idx = get_User_list_index_by_socket(connfd);  // check if there is an existing User in User_list[] by checking returning value of socket number

  // if (idx != -1) {  // there is an existing User in User_list[] of corresponding socket(client)
  //   bzero(User_list[idx].user_name, MAX_USER_NAME); // initialize
  //   strcmp(User_list[idx].user_name, nickname); // update User's name by nickname
  // } else {    // there is no such socket(client) in User_list[], so create a new User with this socket(client)
  //   User newUser; // create a new User
  //   bzero(newUser.user_name, MAX_USER_NAME);  // initialize name
  //   strcmp(newUser.user_name, nickname);  // update name by nickname
  //   newUser.socket = connfd;  // match this User with the connected socket(client)
  //   newUser.room_id = room_id;  // mark the room_id which this User belongs to currently
  //   add_User_in_User_list(newUser); // add this User of corresponding socket(client) into User_list[]
  // }
  // print_Room_list();
  return 1;
}
int change_nickname(int idx, char *nickname){
  if (strlen(nickname) > MAX_USER_NAME) { return -1; }
  strcpy(User_list[idx].user_name, nickname);
  return 1;
}
int check_user_in_which_room(char *nickname){ //check user in which room 
  //which is /where command.
  
  for(int i=0;i<MAX_CLIENTS;i++){ //loop whole server ,and return room id. or -1 if not existed.
    char *tempName=User_list[i].user_name;
    if(strcmp(nickname,tempName)==0)//check name if same.
    {
      return User_list[i].room_id;//return room id
    }
  }
  return -1;
}
int check_user_id_by_nickname(char *nickname){

}
const char * check_username_by_socket(int connfd){ //this will get name by socket number // have error on this part.

  printf("connfd is %d",connfd);
  for(int i=0;i<MAX_CLIENTS;i++){//loop whole server.
    printf("searching %d" ,i);
    int tempSocket=User_list[i].socket;//socket of users.
    printf("socketnumber is %d",tempSocket);
    if(connfd == tempSocket)//if we find matched socket.
    {
      printf("FIND name %s" ,User_list[i].user_name);
      return User_list[i].user_name;//we return this name or -1 if none-existed.
    }
  }
  return (char *)-1;  // temporary fixed to return char *
}
int check_socket_by_username(char *user_name){
  for(int i=0; i<MAX_CLIENTS; i++){
    // printf("size of User[%d]: %d\n", i, )
    if(User_list[i].user_name[0] != '\0' && strcmp(user_name, User_list[i].user_name) == 0) {
      return User_list[i].socket;
    }
  }
  return -1;
}
int check_is_this_name_existing(char *nickname){
  int find_name = check_socket_by_username(nickname);
  if ( find_name > -1){
    return 1;
  }
  return 0;
}
int remove_User_from_list(int connfd){
  int user_idx = get_User_list_index_by_socket(connfd);
  User_list[user_idx].user_name[0] = '\0';
  User_list[user_idx].socket = -1;
  User_list[user_idx].room_id = -1;
  decrease_number_of_user_list();
}
int find_User_socket_idx_from_Room(int connfd, int old_room_id);
int remove_Room_from_list(int room_id);
int remove_User_from_belonging_Room(int connfd){
  int user_idx = get_User_list_index_by_socket(connfd);
  int room_id = User_list[user_idx].room_id;
  int sock_idx = find_User_socket_idx_from_Room(connfd, room_id);
  Room_list[room_id].socket_list_in_Room[sock_idx] = -1;
  Room_list[room_id].num_users--;
  if(Room_list[room_id].num_users <= 0){
    remove_Room_from_list(room_id);
  }
}







// Intended Space - Don't erase empty lines









/********************************
  ROOM
*********************************/
int get_number_of_room_list(){ //room list
  return num_room_list;
}
int increase_number_of_room_list(){
  num_room_list++;  
}
int decrease_number_of_room_list(){
  num_room_list--;  
}
int get_room_userlist(char *room_name){//printout list of user in this room, can just printout nicknames.
  //since room id isnot needed.
  return 0;
}// not sure why this function still existed.
int find_empty_spot_in_Room_list(){
  int i = 0;
  for(i=0; i<MAX_ROOM_NUM; i++){
    if(Room_list[i].room_id == -1){
      return i;
    }
  }
  return -1;
}
int create_new_Room(char *room_name){
  /***
      purpose: create a new Room
      assumption: there is no existing Room in Room_list[],
                  there is at least one space in Room_list, that is, MAX_ROOM_NUM is not reached yet.
                  index of Room_list[] and room_id is matched always, so first Room's room_id is 0 at Room_list[0], and so on.
  ***/
  pthread_mutex_lock(&room);
  printf("\t[+]creating a room \"%s\".\n", room_name);
  int x = get_number_of_room_list();
  if(x > MAX_ROOM_NUM){
    return -1;
  }
  int empty_room_idx = find_empty_spot_in_Room_list();
 // Room initRoom;
 //  initRoom.room_name[0] = '\0';
 //  initRoom.room_id = -1;
 //  initRoom.num_users = 0;
 //  initRoom.socket_list_in_Room[0] = -1;
 //  for(int i=0; i<MAX_ROOM_NUM; i++){
 //    Room_list[i] = initRoom;
 //  }

  strcpy(Room_list[empty_room_idx].room_name, room_name);
  Room_list[empty_room_idx].room_id = empty_room_idx;
  Room_list[empty_room_idx].num_users = 0;

  // Room newRoom; // create a temporary Room object
  // strcpy(newRoom.room_name, room_name); // update its name with room_name
  // newRoom.room_id = x;  // update its room_id
  // newRoom.num_users = 0;  // this Room is created, it means there is at least one User inside
  // newRoom.socket_list_in_Room[0] = connfd;  // add socket(client) into socket_list_in_Room at 0 because it is just created.
  // int add_User_in_Room(int connfd, char *nickname, int room_id)
  // newRoom.socket_list_in_Room[0] = connfd;  // add socket(client) into socket_list_in_Room at 0 because it is just created.
  // Room_list[x] = newRoom; // put this Room into Room_list[], index number and room_id is always matched
  increase_number_of_room_list();  // increate number_of_roomlist
  add_title_into_chat_buffer(empty_room_idx, 0);
  pthread_mutex_unlock(&room);
  return empty_room_idx;
}
int is_room_name_existing(char *room_name){
  printf("\tchecking is_room_name_existing() with %s\n", room_name);
  for(int i=0; i<MAX_ROOM_NUM; i++){
    if (strcmp(Room_list[i].room_name, room_name) == 0 ){ // there is an existing room with the same name
      printf("\tExisting a room with the name %s\n", room_name);
      return Room_list[i].room_id; // return room_id
    }
  }
  printf("\tNo such name of room existing %s\n", room_name);
  return -1;  // there is no room with the same name
}
// int leave_room(char *nickname,int room_id){ // this function will let user leave a room.
//   int tempSocket=-1;//create a temp socket number for compare.
//   for(int i=0;i<MAX_CLIENTS;i++){ //search who server. loop it.
//     char *tempName=User_list[i].user_name;//find a user name who in the server.
//     if(strcmp(nickname,tempName)==0)// if we find this user.
//     {
//       tempSocket=User_list[i].socket;//we find this socket and saved in tempsocket.
//     }
//   }

//   for(int i=0;i<MAX_ROOM_NUM;i++){//search whole room
//     if(tempSocket==Room_list[room_id].socket_list_in_Room[i])//if we find this use's socket.
//     {
//       Room_list[room_id].socket_list_in_Room[i]=-1;//we will set this socket to -1, which is disconnected.
//       //im not sure if this will do this job or not ,you may need to check.

//       return 1;//return 1 successed
//     }
    
//   }
//   return -1;//return -1 if we can not find this user.

// }
int find_empty_spot_socket_list_in_Room(int room_id){
  if(Room_list[room_id].num_users > MAX_USER_IN_A_ROOM) {
    return -1;
  }
  int i=0;
  for(i=0; i<MAX_USER_IN_A_ROOM; i++){
    if(Room_list[room_id].socket_list_in_Room[i] == -1) {
      return i;
    }
  }
  return -1;
}
int find_User_socket_idx_from_Room(int connfd, int old_room_id){
  int i=0;
  for(i=0;i<MAX_USER_IN_A_ROOM;i++){
    if(Room_list[old_room_id].socket_list_in_Room[i] == connfd) {
      return i;
    }
  }
  return -1;
}
int remove_Room_from_list(int room_id){
  if(Room_list[room_id].num_users > 0){
    return -1;
  } else {
    Room_list[room_id].room_id = -1;
    Room_list[room_id].num_users = 0;
    Room_list[room_id].room_name[0] = '\0';
    for(int i=0; i<MAX_USER_IN_A_ROOM; i++){
      Room_list[room_id].socket_list_in_Room[i] = -1;
    }
    init_chat_buffer_in_Room(room_id);
    decrease_number_of_room_list();
  }  
}
int leave_room(int user_idx, int old_room_id, int socket_idx){ // leave the current Room
  printf("[.]Leaving Room\n");
  // print_Room_list();
  print_Room(old_room_id);
  Room_list[old_room_id].socket_list_in_Room[socket_idx] = -1;
  Room_list[old_room_id].num_users--;
  // User_list[user_idx].room_id = -1;
  if(strcmp(Room_list[old_room_id].room_name, (char *)"Lobby") != 0 && Room_list[old_room_id].num_users <= 0){
    printf("[.]Removing the Room\n");
    remove_Room_from_list(old_room_id);
  }
  // print_Room_list();
}

int JOIN_Nickname_Room(int connfd, char *nickname, char *room_name){// create room if room is not existed, and add the user.
  /*** 
    purpose: process the command of "\JOIN nichname roomname"
  // if there is existing room with room_name
  //    add an user in the existing room() - we do not know if there is such an User in User_list[] yet.
  // else (no such room with the name)
  //    if number of room >= MAX
  //        return error
  //    create_new_Room() - create a new Room
  // return success
  ***/
  // find if a room of same name existing. 
  // RETURN VALUE:  
  //                 2: [+]New Room [] created. Entering into it.
  //                 1: [.]Entering existing Room [].
  //                -1: [-]Same name existing. Creating a new User profile failed.
  //                -2: [-]Same name existing. Changing nickname failed.
  //                -3: [-]Room [] is full now. Try again later.
  //                -4: [-]Max number of Rooms reached! No more Rooms can be created at this moment.

  int room_id = is_room_name_existing(room_name);
  // find the index of User_list[] by socket
  int user_idx = get_User_list_index_by_socket(connfd);

  bool created = false;

  printf("\texisting room_id: %d\n", room_id);
  printf("\texisting user_id: %d\n", user_idx);

  char old_room_name[MAX_ROOM_NAME] = "";
  strcpy(old_room_name, Room_list[User_list[user_idx].room_id].room_name);

  if(user_idx < 0){
    user_idx = find_empty_spot_in_User_list();
    int is_same_name = check_is_this_name_existing(nickname);
    if(is_same_name > 0){
      printf("[-]Same name existing. Creating User profile failed.\n");
      return -1;
    }
    create_new_User_at(user_idx, connfd, nickname, room_id);  // create a new User at index
    add_User_in_Room(user_idx, room_id);
    return 1;
  }
  // 1. Check if trying to using same name
  if(strcmp(User_list[user_idx].user_name, nickname) != 0){
    int is_same_name = check_is_this_name_existing(nickname);
    if(is_same_name > 0){
      printf("[-]Same name existing. Changing nickname failed.\n");
      return -2;
    }
    printf("\tchanging nickname from %s to %s\n", User_list[user_idx].user_name, nickname);
    change_nickname(user_idx, nickname);  // change nickname
  }
  // 2. Check if the Room is not existing
  if(room_id < 0){
    printf("\tNon-Existing Room!\n");
    // there is no such Room with that name
    if (get_number_of_room_list() >= MAX_ROOM_NUM) { 
      printf("\t[-]Max number of Rooms reached! No more Rooms can be created at this moment.\n");
      return -4;
    }
    room_id = create_new_Room(room_name);
    created = true;
    printf("\tcreated room_id: %d\n", room_id);
  // }
  // 3. Check if trying to enter same name Room
  // print_User(user_idx);
  // print_Room(room_id);
  }
  printf("\told_room_name: %s\n", old_room_name);
  printf("\tRoom_list[%d].room_name: %s\n", room_id, Room_list[room_id].room_name);
  if(strcmp(Room_list[room_id].room_name, old_room_name) != 0){
    printf("\tgetting into different Room\n");
    // User is trying to enter different Room
    int empty_socket_idx = find_empty_spot_socket_list_in_Room(room_id);  // find an empty spot in that Room
    if(empty_socket_idx < 0){
      printf("\t[-]Room is full. Failed to getting in.\n");
      return -3;
    }
    // print_User(user_idx);
    // print_Room(room_id);
    int old_room_id = User_list[user_idx].room_id;
    int socket_idx = find_User_socket_idx_from_Room(connfd, old_room_id);
    leave_room(user_idx, old_room_id, socket_idx); // leave the current Room
    add_User_in_Room(user_idx, room_id);
  }
  if (created) {
    printf("[+]New Room created\n");
    return 2;
  } else {
    printf("[.]Entering existing Room\n");
    return 1;
  }
}








// Intended Space - Don't erase empty lines










/********************************
  MESSAGE BUFFER / WRAPPER / Etc.
*********************************/
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
  // printf("\tsend_message() begin\n");
  // printf("\tmessage size: %d\n", strlen(message));
  // printf("\tmessage: %s\n", message);
  return send(connfd, message, strlen(message), 0);
}
// This function adds a message that was received to the message buffer.
// Notice the lock around the message buffer.
void add_message(char *buf) {
  pthread_mutex_lock(&lock);
  strncpy(message_buf[msgi % 20], buf, 80);
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
char *token_array[3];// for string to token
void string_to_token(char buf[]){
  token_array[0] = '\0';
  token_array[1] = '\0';
  token_array[2] = '\0';
  int i = 0;
    char *p = strtok (buf, " ");
    while (p != NULL)
    {
        token_array[i++] = p;
        p = strtok (NULL, " ");
    }
}







// Intended Space - Don't erase empty lines








/****************************************
  COMMANDS
*****************************************/
// A predicate function to test incoming message.
int is_Command_message(char *message) { 
  if(message[0]=='\\'){
    //printf("%s\n","it is Command" );
    return true;
  }
  else{
    return false;
  }
}

int send_ROOM_message(int connfd) {
  char message[1024] = "ROOM";
  printf("Sending: %s \n", message);

  return send_message(connfd, message);
}
int send_JOIN_message(int connfd){
  
}
/* Command: \ROOMS */
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
  
  for(int i=0;i<MAX_ROOM_NUM;i++){
    if (Room_list[i].room_id < 0) { continue; }
    strcat(list_buffer, "\n");
    strcat(list_buffer, prefix);  // add prefix
    int temp_int = Room_list[i].room_id;  // get room_id
    sprintf(temp, "%d", temp_int);  // convert room_id into chars
    strcat(list_buffer, temp); // add room_id chars
    strcat(list_buffer, suffix);  // add suffix
    strcat(list_buffer, Room_list[i].room_name); // add room_name
    
  }
  printf("Sending: %s\n", list_buffer);
  return send_message(connfd, list_buffer);
}
/* Command: \WHO */
int send_userlist_message(int connfd) { 
  char message[BUF_MAX_20LINES * BUF_MAX_100CHARS] = "";
  int tempRoom_ID = -1;
  for(int i=0;i<MAX_CLIENTS;i++){
    int tempSocket = User_list[i].socket;
    if(tempSocket==connfd)
    {
      tempRoom_ID=User_list[i].room_id;
    }
  }
  //we find room_id

  for(int i=0;i<MAX_ROOM_NUM;i++){
    int current_socket_in_room=Room_list[tempRoom_ID].socket_list_in_Room[i];
    char *nickname[MAX_USER_NAME];
    if(current_socket_in_room >-1)
    {
      for(int i=0; i<MAX_CLIENTS;i++){
        if(User_list[i].socket==current_socket_in_room)
        {
          strcat(message, User_list[i].user_name);
          strcat(message,",");
        }
      }

    }
  }
  

  // End the message with a newline and empty. This will ensure that the
  // bytes are sent out on the wire. Otherwise it will wait for further
  // output bytes.
  strcat(message, "\n\0");
  printf("Sending: %s", message);

  return send_message(connfd, message);
}
/* Command : \WHERE */
// int send_where_message(int connfd, char *message){
// //this should be two jobs: 
// // 1) "\WHERE" which tells where I am
// // 2) "\WHERE nickname" which tells where he is. 
// // a) if a user send "\WHERE name" which that name is his name, then it is same as "\WHERE" and don't need to search himself again
// //
// //  if command is not a valid command, then return error
// //  if 2nd argument is same as his name, then simply call 
//  // message = (char *)("%s "," Your location room is ");
  
//   char buffer[BUF_SMALL_300] = "";
//   string_to_token(message);
//   int user_index = -1;

//   if (!token_array[1]){ // if there is no second argument, e.g., \WHERE
//     printf("checking myself\n");
//     user_index = get_User_list_index_by_socket(connfd);
//   } else {  // there is 2nd argument(user name). e.g., \WHERE user_name
//     printf("check_socket_by_username()\n");
//     user_index = get_User_list_index_by_socket(check_socket_by_username(token_array[1]));
//   }

//   bzero(buffer, sizeof(buffer));
//   printf("user_index:%d\n", user_index);
//   strcat(buffer, "User ");
//   if (user_index == -1) {
//     if(token_array[1]) strcat(buffer, token_array[1]);
//     strcat(buffer, " does not exist.");
//     printf("[%s]\n", buffer);
//   } else {
//     strcat(buffer, User_list[user_index].user_name);
//     strcat(buffer, " is at Room[");
//     strcat(buffer, (char *)User_list[user_index].room_id);
//     strcat(buffer, "]: ");
//     strcat(buffer, Room_list[User_list[user_index].room_id].room_name);
//   }
//   return send_message(connfd, buffer);
//   // // char* nickname = (char*)check_username_by_socket(connfd);  // we should know socket number by username rather than to know username by socket becuase the argument is username and nobody knows other's socket number
//   // // printf("the nickname is %s",nickname);
//   // // int r_id = check_user_in_which_room(nickname);
//   // // char *temp_room_name = Room_list[r_id].room_name;
//   // // printf("the room_id is %d\troom name:%s",r_id, temp_room_name);
//   // // strcat(message,temp_room_name);
// }

int send_where_message(int connfd, char *message){
//this should be two jobs: 
// 1) "\WHERE" which tells where I am
// 2) "\WHERE nickname" which tells where he is. 
// a) if a user send "\WHERE name" which that name is his name, then it is same as "\WHERE" and don't need to search himself again
//
//  if command is not a valid command, then return error
//  if 2nd argument is same as his name, then simply call 
 // message = (char *)("%s "," Your location room is ");
  
  char buffer[BUF_SMALL_300] = "";
  string_to_token(message);
  int user_index = -1;

  if (!token_array[1]){ // if there is no second argument, e.g., \WHERE
    printf("checking myself\n");
    user_index = get_User_list_index_by_socket(connfd);
  } else {  // there is 2nd argument(user name). e.g., \WHERE user_name
    printf("check_socket_by_username()\n");
    user_index = get_User_list_index_by_socket(check_socket_by_username(token_array[1]));
  }

  bzero(buffer, sizeof(buffer));

  
  strcat(buffer, "User ");
  
  if (user_index == -1) {
    
    if(token_array[1]) strcat(buffer, token_array[1]);
    strcat(buffer, " does not exist.");
    
  } else {
    
    strcat(buffer, User_list[user_index].user_name);
    
    strcat(buffer, " is at Room[");
    
    
     char temp_room_id[20];
     sprintf(temp_room_id, "%d", User_list[user_index].room_id);
   
    
    strcat(buffer, temp_room_id);
    
    strcat(buffer, "]: ");
    
    strcat(buffer, Room_list[User_list[user_index].room_id].room_name);
  }
  return send_message(connfd, buffer);
  // // char* nickname = (char*)check_username_by_socket(connfd);  // we should know socket number by username rather than to know username by socket becuase the argument is username and nobody knows other's socket number
  // // printf("the nickname is %s",nickname);
  // // int r_id = check_user_in_which_room(nickname);
  // // char *temp_room_name = Room_list[r_id].room_name;
  // // printf("the room_id is %d\troom name:%s",r_id, temp_room_name);
  // // strcat(message,temp_room_name);
}
int send_helplist_message(int connfd) { // this part do not need help list.】
  char message[BUF_MAX_20LINES * BUF_MAX_100CHARS] = "";
  bzero(message, sizeof(message));
  const char *temp_user_list[8];
    temp_user_list[0] = "\\JOIN nickname room, join other room with name you want ";
    temp_user_list[1] = "\\ROOMS,list the room names  of server has right now";
    temp_user_list[2] = "\\LEAVE, leave server, close connection";
    temp_user_list[3] = "\\WHO, show your name in this room";
    temp_user_list[4] = "\\nickname message, whisper message to nickname you want";
    temp_user_list[5] = "\\HELP, show list of command";
    temp_user_list[6] = "\\WHERE, show where you at which room";
    temp_user_list[7] = "\\WHERE nickname, show the room name of this user located";
    temp_user_list[8] = "\\WHISPER nickname, whisper to a user that only both of you can see the message";
  for (int i = 0; i < 8; i++) {
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

int send_whisper_message(int connfd, int whisper_target, char *message){
  int user_idx = get_User_list_index_by_socket(connfd);
  char whisper_msg[BUF_SMALL_300];
  bzero(whisper_msg, sizeof(whisper_msg));
  strcat(whisper_msg, (char *)"Whisper[");
  strcat(whisper_msg, User_list[user_idx].user_name);
  strcat(whisper_msg, (char *)"]:");  
  strcat(whisper_msg, message);
  send_message(whisper_target, whisper_msg);
  return send_message(connfd, whisper_msg);
}
int send_chat_message(int connfd, char *message){
  // printf("\t\t\t\t\tstrlen(message):%d\n", strlen(message));
  message[BUF_MAX_100CHARS-1]='\0';
  // printf("\tgot message from client[%d]: %s\n", connfd, message);
  int user_idx = get_User_list_index_by_socket(connfd);
  int room_id = User_list[user_idx].room_id;
  // printf("\t\t\tmessage length: %d\n", strlen(message));
  if(strlen(message) > 0 && strcmp(message, "\n") != 0) {
    // printf("before add_message()\n");
    add_message_into_chat_buffer(connfd, room_id, message);
    // printf("after add_message()\n");
    print_chat_buffer(room_id);
  }
  create_entire_message_from_chat_buffer(room_id);
  // printf("\tentire_message_buf: %s\n", entire_message_buf);
  return send_message(connfd, entire_message_buf);
  // return send_message(connfd, get_chat_buffer(room_id));
}

int process_message(int connfd, char *message) {//idk if we can use case switch
  
  if (is_Command_message(message)) {
    printf("Received a Command: ");
    // Bug: if the command is not made of three words, e.g., just "\JOIN", then this will crash.
    //      we should deal with those edge cases.
    if(strncmp(message, "\\JOIN",5) == 0){
      printf("%s\n","\\JOIN nickname room" );
      string_to_token(message);//convert to tokens
      if(!token_array[1]||!token_array[2]){// if token_array[1][2]is null, it will send back the message.
        printf("%s \n","make sure you have 3 arguments");
        return send_message(connfd, (char *)"command not recognized,make sure you have 3 arguments");
      }
      //token_array[1] is nickname
      //token_array[2] is room name
      printf("\n[socket%d]Trying to make a Room: %s", connfd, token_array[2]);
      printf("\tNickname: %s\n", token_array[1]);
      // we can safely call JOIN_Nickname_Room() without any considerations here.
      // the function will deal with any cases.
      char msg_buf[MAX_ROOM_NAME + 30];
      bzero(msg_buf, sizeof(msg_buf));
      int join = JOIN_Nickname_Room(connfd, (char *)token_array[1],(char *)token_array[2]);
      // RETURN VALUE:  
      //                 2: [+]New Room [] created. Entering into it.
      //                 1: [.]Entering existing Room [].
      //                -1: [-]Same name existing. Creating a new User profile failed.
      //                -2: [-]Same name existing. Changing nickname failed.
      //                -3: [-]Room [] is full now. Try again later.
      //                -4: [-]Max number of Rooms reached! No more Rooms can be created at this moment.
      if(join==2){
        strcpy(msg_buf, (char *)"[+]New Room [");
        strcat(msg_buf, token_array[2]);
        strcat(msg_buf, (char *)"] created. Entering into it.");
        return send_message(connfd, msg_buf);
      }
      else if(join == 1){
        strcpy(msg_buf, (char *)"[.]Entering existing Room [");
        strcat(msg_buf, token_array[2]);
        strcat(msg_buf, (char *)"]");
        return send_message(connfd, msg_buf);
      }
      else if(join == -1){
        strcpy(msg_buf, (char *)"[-]Same name existing. Creating a new User profile failed.");
        return send_message(connfd, msg_buf);
      }
      else if(join == -2){
        strcpy(msg_buf, (char *)"[-]Same name existing. Changing nickname failed.");
        return send_message(connfd, msg_buf);
      }
      else if(join == -3){
        strcpy(msg_buf, (char *)"[-]Room [] is full now. Try again later.");
        return send_message(connfd, msg_buf);
      }
      else if(join == -4){
        strcpy(msg_buf, (char *)"[-]Max number of Rooms reached! No more Rooms can be created at this moment.");
        return send_message(connfd, msg_buf);
      }
      return send_message(connfd, msg_buf);
      // if(join <= -1){
      //   strcpy(msg_buf, (char *)"[-]Error creating a room ");
      //   strcat(msg_buf, token_array[2]);
      //   return send_message(connfd, msg_buf);
      // }
      // if(join == 2 || strcmp(token_array[2], (char *)"Lobby") == 0) {
      //   strcpy(msg_buf, (char *)"[+]Entering ");
      // } else {
      //   strcpy(msg_buf, (char *)"[+]Created a Room ");
      // }
      // strcat(msg_buf, token_array[2]);
    }
    else if(strcmp(message, "\\ROOMS") == 0){//this part is fine
            printf("%s\n","\\ROOMS" );
            // printf("%s\n", get_room_list());
            print_Room_list();
            return send_roomlist_message(connfd);
    }
    else if(strcmp(message, "\\LEAVE") == 0){//this part is fine
      char msg_buf[20] = "SERVER[0]: GoodBye";
      send_message(connfd, msg_buf);
      // close(connfd);
      return 99;
      // return send_message(connfd, message);
    }
    else if(strncmp(message, "\\WHERE", 5) == 0){
      printf("%s\n","\\WHERE" );
      return send_where_message(connfd, message);

    }
    else if(strcmp(message, "\\WHO") == 0){//this part is fine
          printf("%s\n","\\WHO" );
          return send_userlist_message(connfd);
    }
    else if(strcmp(message, "\\HELP") == 0){//this part is fine
          printf("%s\n","it is \\HELP" );
          return send_helplist_message(connfd);
    }
    // else if(strcmp(message, "\\nickname message") == 0){//this part will be in a same room and whisper by nickname
      //if you can not find the nickname then show it user not existed. some thing like this. much easy.
    // }
    else{
      // Check if the client is trying to WHISPER someone on the server.
      char nick_buf[MAX_USER_NAME+10];  // buffer for storing the nickname
      char mesg_buf[MAX_MSG_CHAR+10];
      bzero(nick_buf, sizeof(nick_buf));  // initialize buffer
      bzero(mesg_buf, sizeof(nick_buf));  // initialize buffer

      int i = 1;  // first index of nickname array - token_array[0] has "\NICKNAME"
      for(i=1; i<strlen(message); i++){  // extract a char at a time
          // if((int)(*(message+i)) == 32) printf("WHITE SPACE!!!\n");
          if (message+i == '\0' || (int)(*(message+i)) == 32) { break; }
          strncat(nick_buf, message+i, 1);
          if(i>=MAX_USER_NAME) { break; } // if the input user nickname is exceeded MAX_USER_NAME chars
      }
      if(i==MAX_USER_NAME) {
        while(1){
          if(*(message+i) != ' ') {
            i++;
          } else {
            break;
          }
        }
      }
      for(i++; i<strlen(message); i++){  // extract a char at a time
          if (message+i == '\0' || *(message+i) == '\n') { break; }
          strncat(mesg_buf, message+i, 1);
          if(i>=MAX_MSG_CHAR) { break; } // if the input user nickname is exceeded MAX_USER_NAME chars
      }

      printf("\n\t\tnick_buf: %s\n", nick_buf);
      printf("\t\tmesg_buf: %s\n", mesg_buf);

      int whisper_target = check_socket_by_username(nick_buf);
      printf("\t\twhisper_target: %d\n", whisper_target);

      if(whisper_target>-1){
        int idx = get_User_list_index_by_socket(whisper_target);
        printf("[+]User found at [%d]: %s\n", idx, User_list[idx].user_name);
        return send_whisper_message(connfd, whisper_target, mesg_buf);
      } else {
        // char tempMessage[MAX_MSG_CHAR + 100] ="[-]Command not recognized\n";//this part is fine
        char tempMessage[MAX_MSG_CHAR + 100];
        bzero(tempMessage, sizeof(tempMessage));
        if (strlen(mesg_buf) > 0) {
          strcpy(tempMessage, (char *)"User ");
          strcat(tempMessage, nick_buf); 
          strcat(tempMessage, (char *)" not found.\n");
        } else {
          strcpy(tempMessage, (char *)"[-]Command ");
          strcat(tempMessage, message); 
          strcat(tempMessage, (char *)" not recognized.\n");
        }
        printf("%s\n ", tempMessage);
        return send_message(connfd, tempMessage);
      }
    }
    return send_ROOM_message(connfd);
  } 
  else {//this part is fine
    printf("Server sending chat messages to clients.\n");
    return send_chat_message(connfd, message);
    // printf("Server responding with echo response.\n");
    // return send_message(connfd, message);
  }
}

void chat_system(int connfd){
  size_t n;
  char message[MAXLINE];

  // Create a default User object
  User defaultUser;
  strncpy(defaultUser.user_name, DEFAULT_USR_NAME, sizeof(DEFAULT_USR_NAME));
  defaultUser.socket = connfd;  // match this User object and the connected socket(client)
  defaultUser.room_id = 0;

  // create_new_Room((char *)"Lobby");
  JOIN_Nickname_Room(connfd, (char *)DEFAULT_USR_NAME, (char *)"Lobby");

  // find the index of User_list[] by socket

  while((n=receive_message(connfd, message))>0) {
    printf("From socket[%d]: Server received a meesage of %d bytes: %s\n", connfd, (int)n, message);
    // int user_idx = get_User_list_index_by_socket(connfd);
    // int room_id = is_room_name_existing(Room_list[User_list[user_idx].room_id].room_name);
    // print_User(user_idx);
    // print_Room(room_id);

    n = process_message(connfd, message);
    if(n == 99) {
      remove_User_from_belonging_Room(connfd);
      remove_User_from_list(connfd);
      close(connfd);
      break;
    }
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
  create_new_Room((char *)"Lobby");

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
  // Grab the connection file descriptor.
  int connfd = *((int *)vargp);
  // Detach the thread to self reap.
  pthread_detach(pthread_self());
  // Free the incoming argument - allocated in the main thread.
  free(vargp);
  // Handle the echo client requests.
  printf("[+]New Thread created with the socket [%d]\n", connfd);
  print_sockets();
  chat_system(connfd);
  printf("[-]Client with socket [%d] disconnected.\n", connfd);
  delete_socket(connfd);
  print_sockets();
  // Don't forget to close the connection!
  close(connfd);
  return NULL;
}
