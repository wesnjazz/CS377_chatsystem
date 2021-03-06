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
#include <time.h>
#include <iostream>
#include "server.h"

/********************************
  Initialization PART
********************************/
void init_entire_message_buf() {
  /* Initialize the message buffer to empty strings. */
  pthread_mutex_lock(&lock);
  bzero(entire_message_buf, sizeof(entire_message_buf));
  pthread_mutex_unlock(&lock);
}
void init_Rooms_Users(){ 
  /* initialize Rooms and Users */
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
/* helper functions of printing for debugging purpose */
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
  pthread_mutex_lock(&room);
  for(int i=0; i<BUF_MAX_20LINES; i++){
    bzero(Room_list[room_id].chat_buffer[i], BUF_MAX_100CHARS);
  }
  add_title_into_chat_buffer(room_id, 0); 
  Room_list[room_id].chat_buffer_HEAD = 0;
  Room_list[room_id].chat_buffer_TAIL = 0;
  pthread_mutex_unlock(&room);
  return 1;
}
int add_message_into_chat_buffer(int connfd, int room_id, char *message){
  /** add incoming message into the char_buffer of the specific Room **/
  /** purpose: recording chat history **/
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

  // Circular Buffer calculation
  tail++;
  if (tail >= BUF_MAX_20LINES){
    tail = 0;
  }
  if (tail <= head){
    head++;
    if (head >= BUF_MAX_20LINES){
      head = 0;
    }
    add_title_into_chat_buffer(room_id, head);
    pthread_mutex_lock(&room);
    Room_list[room_id].chat_buffer_HEAD = head;
    pthread_mutex_unlock(&room);
  }
  strcat(prefix, message);
  printf("\tprefix: %s\n", prefix);
  strncpy(Room_list[room_id].chat_buffer[tail], prefix, BUF_MAX_100CHARS-1);
  pthread_mutex_lock(&room);
  Room_list[room_id].chat_buffer_TAIL = tail;
  pthread_mutex_unlock(&room);
  return 1;
}
void get_chat_buffer(int room_id){
  /** getting the char_buffer from the specific Room **/
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
}
void print_chat_buffer(int room_id){
  for(int i=0; i<BUF_MAX_20LINES; i++){
    printf("%3d: %s\n", i, Room_list[room_id].chat_buffer[i]);
  }
}
int create_entire_message_from_chat_buffer(int room_id){
  /** build up the entire message for boradcasting chat history to all users in the Room **/
  init_entire_message_buf();
  int tail = Room_list[room_id].chat_buffer_TAIL;
  int head = Room_list[room_id].chat_buffer_HEAD;
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
  pthread_mutex_lock(&lock);
  for(int i=0; i<MAX_CLIENTS; i++){
    if( socket_list[i] != -1) { continue; }
    else {
      socket_list[i] = connfd;
      break;
    }
  }
  pthread_mutex_unlock(&lock);
}
void delete_socket(int connfd){ // find a matching socket and delete it.
  pthread_mutex_lock(&lock);
  for(int i=0; i<MAX_CLIENTS; i++){
    if( socket_list[i] == connfd) {
      socket_list[i] = -1;
      break;
    }
  }
  pthread_mutex_unlock(&lock);
}
int erase_all_info_of_socket_from_server(int connfd){
  for(int i=0; i<MAX_ROOM_NUM; i++){
    for(int j=0; j<MAX_USER_IN_A_ROOM; j++){
      if(Room_list[i].socket_list_in_Room[j] == connfd) {
        Room_list[i].socket_list_in_Room[j] = -1;
        // Room_list[i].num_users--;
      }
    }
  }
  for(int i=0; i<MAX_CLIENTS; i++){
    if(socket_list[i] == connfd) {
      socket_list[i] = -1;
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
  /** increase number of User **/
  pthread_mutex_lock(&user);
  num_user_list++;
  pthread_mutex_unlock(&user);
}
void decrease_number_of_user_list(){
  /** decrease number of User **/
  pthread_mutex_lock(&user);
  num_user_list--;
  pthread_mutex_lock(&user);
}
int get_User_list_index_by_socket(int connfd){  
  /** find an User index from User_list[] by comparing corresponding socket(client) **/
  /** RETURN: index of the given socket(client) **/
  for(int i=0; i<MAX_CLIENTS; i++){
    if(User_list[i].socket != -1 && User_list[i].socket == connfd){  // found the corresponding socket(client)
      return i;   // return index number of corresponding socket(client)
    }
  }
  return -1;  // that socket(client) is not existing
}
int find_empty_spot_in_User_list(){
  /** find if there is an empty spot for a User to be created **/
  /** RETURN: index of the empty spot from User_list **/
  int i = 0;
  for(i=0; i<MAX_CLIENTS; i++){
    if(User_list[i].socket==-1){
      return i;
    }
  }
  return -1;
}
int create_new_User_at(int idx, int connfd, char *nickname, int room_id){
  /** create a new User profile into User_list */
  /** RETURN: index of the location where the User profile is created **/
  if(get_number_of_user_list() > MAX_CLIENTS) { return -1; }
  strcpy(User_list[idx].user_name, nickname);
  pthread_mutex_lock(&user);
  User_list[idx].socket = connfd;
  User_list[idx].room_id = room_id;
  pthread_mutex_unlock(&user);
  print_User(idx);
  increase_number_of_user_list();
  return idx;
}
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
  pthread_mutex_lock(&room);
  Room_list[room_id].socket_list_in_Room[socket_idx] = User_list[user_idx].socket; // add the socket(client) into the user list of the Room
  Room_list[room_id].num_users++; // increase the number of users in the Room
  User_list[user_idx].room_id = room_id;
  pthread_mutex_unlock(&room);
  return 1;
}
int change_nickname(int idx, char *nickname){
  /** Change the User's nickname **/
  /** RETURN: 1 - Succedded to change name, -1 - Max number of User's reached **/
  if (strlen(nickname) > MAX_USER_NAME) { return -1; }
  pthread_mutex_lock(&user);
  strcpy(User_list[idx].user_name, nickname);
  pthread_mutex_unlock(&user);
  return 1;
}
int check_user_in_which_room(char *nickname){
  /** Check where the User is located amogn Rooms **/
  /** RETURN: index of the Room which that name of User is currently in **/
  for(int i=0;i<MAX_CLIENTS;i++){ //loop whole server ,and return room id. or -1 if not existed.
    char *tempName=User_list[i].user_name;
    if(strcmp(nickname,tempName)==0)//check name if same.
    {
      return User_list[i].room_id;//return room id
    }
  }
  return -1;
}
int check_socket_by_username(char *user_name){
  /** Get a socket descriptor by user name **/
  /** RETURN: index of the given socket(client) **/
  for(int i=0; i<MAX_CLIENTS; i++){
    if(User_list[i].user_name[0] != '\0' && strcmp(user_name, User_list[i].user_name) == 0) {
      return User_list[i].socket;
    }
  }
  return -1;
}
int check_is_this_name_existing(char *nickname){
  /** Check if the given name is existing already **/
  /** RETURN: 1 - existing, 0 - not existing **/
  int find_name = check_socket_by_username(nickname);
  if ( find_name > -1){
    return 1;
  }
  return 0;
}
int remove_User_from_list(int connfd){
  /** Remove a client(socket) from the server by erasing socket **/
  int user_idx = get_User_list_index_by_socket(connfd);
  pthread_mutex_lock(&user);
  User_list[user_idx].user_name[0] = '\0';
  User_list[user_idx].socket = -1;
  User_list[user_idx].room_id = -1;
  pthread_mutex_unlock(&user);
  decrease_number_of_user_list();
  return 1;
}
int remove_User_from_belonging_Room(int connfd){
  /** Remove the user from its belonging Room **/
  int user_idx = get_User_list_index_by_socket(connfd);
  int room_id = User_list[user_idx].room_id;
  int sock_idx = find_User_socket_idx_from_Room(connfd, room_id);
  pthread_mutex_lock(&room);
  Room_list[room_id].socket_list_in_Room[sock_idx] = -1;
  Room_list[room_id].num_users--;
  pthread_mutex_lock(&room);
  if(Room_list[room_id].num_users <= 0){
    remove_Room_from_list(room_id);
  }
  return 1;
}



// Intended Space - Don't erase empty lines



/********************************
  ROOM
*********************************/
int get_number_of_room_list(){ //room list
  return num_room_list;
}
int increase_number_of_room_list(){
  /** Increase the number of Room **/
  num_room_list++;  
}
int decrease_number_of_room_list(){
  /** Decrease the number of Room **/
  num_room_list--;  
}
int find_empty_spot_in_Room_list(){
  /** Find an empty spot in the Room list to create to a new Room **/
  /** RETURN: index of the empty spot in the Room **/
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
  if(x > MAX_ROOM_NUM){ return -1; }
  int empty_room_idx = find_empty_spot_in_Room_list();
  strcpy(Room_list[empty_room_idx].room_name, room_name);
  Room_list[empty_room_idx].room_id = empty_room_idx;
  Room_list[empty_room_idx].num_users = 0;

  increase_number_of_room_list();  // increate number_of_roomlist
  add_title_into_chat_buffer(empty_room_idx, 0);
  pthread_mutex_unlock(&room);
  return empty_room_idx;
}
int is_room_name_existing(char *room_name){
  /** Check if the given name of Room existing **/
  /** RETURN: index of the Room **/
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
int find_empty_spot_socket_list_in_Room(int room_id){
  /** Check an empty spot in the specific Room so that let a client get into that Room **/
  /** RETURN: index of that socket **/
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
    pthread_mutex_lock(&room);
    Room_list[room_id].room_id = -1;
    Room_list[room_id].num_users = 0;
    Room_list[room_id].room_name[0] = '\0';
    for(int i=0; i<MAX_USER_IN_A_ROOM; i++){
      Room_list[room_id].socket_list_in_Room[i] = -1;
    }
    pthread_mutex_unlock(&room);
    init_chat_buffer_in_Room(room_id);
    decrease_number_of_room_list();
  }  
}
int leave_room(int user_idx, int old_room_id, int socket_idx){ // leave the current Room
  printf("[.]Leaving Room\n");
  print_Room(old_room_id);
  pthread_mutex_lock(&room);
  Room_list[old_room_id].socket_list_in_Room[socket_idx] = -1;
  Room_list[old_room_id].num_users--;
  pthread_mutex_unlock(&room);
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
  int user_idx = get_User_list_index_by_socket(connfd);   // find the index of User_list[] by socket
  bool created = false;

  char old_room_name[MAX_ROOM_NAME] = "";
  strcpy(old_room_name, Room_list[User_list[user_idx].room_id].room_name);

  // Check if ths User profile has been created
  if(user_idx < 0){
    user_idx = find_empty_spot_in_User_list();
    int is_same_name = check_is_this_name_existing(nickname);
    if(is_same_name > 0){
      printf("[-]Same name existing. Creating User profile failed.\n");
      return -1;
    }
    create_new_User_at(user_idx, connfd, nickname, room_id);  // create a new User at index
    add_User_in_Room(user_idx, room_id); // get into the Lobby as the first location
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
    int ch = change_nickname(user_idx, nickname);  // change nickname
    // if (ch) send_message(connfd, (char *)"SERVER[1]: Your nickname has been changed.");
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
  }
  // 3. Check if trying to enter same name Room
  if(strcmp(Room_list[room_id].room_name, old_room_name) != 0){
    printf("\tgetting into different Room\n");
    // User is trying to enter different Room
    int empty_socket_idx = find_empty_spot_socket_list_in_Room(room_id);  // find an empty spot in that Room
    if(empty_socket_idx < 0){
      printf("\t[-]Room is full. Failed to getting in.\n");
      return -3;
    }
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
  return send(connfd, message, strlen(message), 0);
}
// This function adds a message that was received to the message buffer.
// Destructively modify string to be upper case
char *token_array[3];// for string to token
void string_to_token(char buf[]){
  pthread_mutex_lock(&lock);
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
  pthread_mutex_unlock(&lock);
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
}
int send_helplist_message(int connfd) { // this part do not need help list.】
  char message[BUF_MAX_20LINES * BUF_MAX_100CHARS];
  bzero(message, sizeof(message));
  const char *temp_user_list[12];
    temp_user_list[0] = "\n\\JOIN nickname room\t: Join other room with the name you want ";
    temp_user_list[1] = "\\ROOMS\t\t\t: List all the Room names";
    temp_user_list[2] = "\\LEAVE\t\t\t: Leave server, close connection";
    temp_user_list[3] = "\\WHO\t\t\t: List all users in this room";
    temp_user_list[4] = "\\HELP\t\t\t: Show list of command";
    temp_user_list[5] = "\\nickname message\t: Whisper (send private message) to nickname(same or another Room) you want";
    temp_user_list[6] = "\n\n *** Creative Features ***\n";
    temp_user_list[7] = "\\KICK nickname\t\t: You can kick someone off any user in the server. >.< (Be respectful!)";
    temp_user_list[8] = "\\WHERE\t\t\t: Show where you are at which room";
    temp_user_list[9] = "\\WHERE nickname\t\t: Show the room name of target user is located";
    temp_user_list[10] = "\\CHANGENAME\t\t: Change the nickname conveniently without using JOIN command";
    temp_user_list[11] = "\\TIME\t\t\t: Show the current time";
    // temp_user_list[8] = "\\WHISPER nickname\t\t: Whisper to a user that only both of you can see the message";
  for (int i = 0; i < 12; i++) {
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
    // print_chat_buffer(room_id);
  }
  create_entire_message_from_chat_buffer(room_id);
  // printf("\tentire_message_buf: %s\n", entire_message_buf);
  return send_message(connfd, entire_message_buf);
  // return send_message(connfd, get_chat_buffer(room_id));
}

int process_message(int connfd, char *message) {//idk if we can use case switch
  if (is_Command_message(message)) {
    printf("Received a Command: ");
    if(strncmp(message, "\\JOIN",5) == 0){
      printf("%s\n","\\JOIN nickname room" );
      string_to_token(message);//convert to tokens
      if(!token_array[1]||!token_array[2]){// if token_array[1][2]is null, it will send back the message.
        printf("%s \n","make sure you have 3 arguments");
        return send_message(connfd, (char *)"command not recognized,make sure you have 3 arguments");
      }
      //token_array[1] is nickname
      //token_array[2] is room name
      printf("\n[socket%d]Trying to make a Room: %s\n", connfd, token_array[2]);
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
    }
    else if(strcmp(message, "\\ROOMS") == 0){
            printf("%s\n","\\ROOMS" );
            print_Room_list();
            return send_roomlist_message(connfd);
    }
    else if(strcmp(message, "\\LEAVE") == 0){
      char msg_buf[20] = "SERVER[0]: GoodBye";
// int leave_room(int user_idx, int old_room_id, int socket_idx){ // leave the current Room
      int user_idx = get_User_list_index_by_socket(connfd);
      int room_id = User_list[user_idx].room_id;
      int socket_idx = find_User_socket_idx_from_Room(connfd, room_id);
      leave_room(user_idx, room_id, socket_idx);
      return send_message(connfd, msg_buf);
      // close(connfd);
      // return 99;
    }
    else if(strncmp(message, "\\WHERE", 5) == 0){
      printf("%s\n","\\WHERE" );
      return send_where_message(connfd, message);
    }
    else if(strcmp(message, "\\WHO") == 0){
          printf("%s\n","\\WHO" );
          return send_userlist_message(connfd);
    }
    else if(strcmp(message, "\\HELP") == 0){
          printf("%s\n","it is \\HELP" );
          return send_helplist_message(connfd);
    }
    else if(strncmp(message,"\\CHANGENAME", 10)==0){
          printf("%s\n","it is \\ChangeName" );
          string_to_token(message);
          if(!token_array[1]){
            printf("\n 1 arguments");
            return send_message(connfd, (char *)"command not recognized,make sure you have 2 arguments");
          }
          else{
          printf("\n 2 arguments");
          int idx = get_User_list_index_by_socket(connfd);
          int room_id=User_list[idx].room_id;
          char * room_name;
          room_name =(char*) Room_list[room_id].room_name;
          printf(" user idx is %d\n", idx);
          printf(" user room id is %d\n", room_id);
          printf(" user name is %s\n", token_array[1]);
          printf(" user room name is %s\n", room_name);
          JOIN_Nickname_Room(connfd,token_array[1],room_name);
          }
        }
    else if(strcmp(message, "\\TIME") == 0){
          time_t rawtime;
          struct tm * timeinfo;

          time ( &rawtime );
          timeinfo = localtime ( &rawtime );
          printf ( "Current local time and date: %s", asctime (timeinfo) );
          char msg_buf[20] = "" ;
          strcat(msg_buf, (char *)"Current local time and date:");
          strcat(msg_buf,asctime (timeinfo));
          send_message(connfd, msg_buf);
    }
    else if(strncmp(message, "\\KICK",4) == 0){
          string_to_token(message);
          if(!token_array[1]){
            printf("\n 1 arguments");
            return send_message(connfd, (char *)"command not recognized,make sure you have 2 arguments");
          
          }
          else{
          int tempSocket = check_socket_by_username(token_array[1]);
          printf(" user socket is %d\n", tempSocket);
          char * message = "\\LEAVE";
          process_message(tempSocket, message);
        }
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
        // char tempMessage[MAX_MSG_CHAR + 100] ="[-]Command not recognized\n";
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
  else {
    printf("Server sending chat messages to clients.\n");
    return send_chat_message(connfd, message);
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

  JOIN_Nickname_Room(connfd, (char *)DEFAULT_USR_NAME, (char *)"Lobby");

  while((n=receive_message(connfd, message))>0) {
    printf("From socket[%d]: Server received a meesage of %d bytes: %s\n", connfd, (int)n, message);

    int user_idx = get_User_list_index_by_socket(connfd);
    n = process_message(connfd, message);
    // print_User(user_idx);
    // print_Room(Room_list[user_idx].room_id);
    // print_Room_list();
    // print_sockets();

    if(n == 99) {
      remove_User_from_belonging_Room(connfd);
      remove_User_from_list(connfd);
      // close(connfd);
      // delete_socket(connfd);
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
  pthread_mutex_init(&user, NULL);

  // The port number for this server.
  int port = atoi(argv[1]);

  // The listening file descriptor.
  int listenfd = open_listenfd(port);

  // Initialize ROOMS, USERS, MESSAGES
  init_Rooms_Users();

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
  // erase_all_info_of_socket_from_server(connfd);
  delete_socket(connfd);
  print_sockets();
  // Don't forget to close the connection!
  close(connfd);
  return NULL;
}
