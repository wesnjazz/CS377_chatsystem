#ifndef SRD_SERVER_H
#define SRD_SERVER_H

#include "room.h"
#include "user.h"
#include "message.h"

/* Max text line length */
#define MAXLINE 8192

// A wrapper around recv to simplify calls.
int receive_message(int connfd, char *message) {
  return recv(connfd, message, MAXLINE, 0);
}

// A wrapper around send to simplify calls.
int send_message(int connfd, char *message) {
  return send(connfd, message, strlen(message), 0);
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
int send_roomlist_message(int connfd) {
  char message[20 * 50] = "";
  const char *temp_room_list[2];
    temp_room_list[0] = "blah";
    temp_room_list[1] = "hmm";
  for (int i = 0; i < 2; i++) {
    if (strcmp(temp_room_list[i], "") == 0) break;//room list
    strcat(message, temp_room_list[i]);//room list
    strcat(message, ",");
  }

  // End the message with a newline and empty. This will ensure that the
  // bytes are sent out on the wire. Otherwise it will wait for further
  // output bytes.
  strcat(message, "\n\0");
  printf("Sending: %s", message);

  return send_message(connfd, message);
}
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
    temp_user_list[0] = "-JOIN nickname room";
    temp_user_list[1] = "-ROOMS";
    temp_user_list[2] = "-LEAVE";
    temp_user_list[3] = "-WHO";
    temp_user_list[4] = "-nickname message";
    temp_user_list[5] = "-HELP";
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


int process_message(int connfd, char *message) {//idk if we can use case switch
  if (is_Command_message(message)) {
    printf("iT IS COMMAND.\n");
    if(strcmp(message, "-JOIN nickname room") == 0){
            printf("%s\n","it is -JOIN nickname room" );
    }
    else if(strcmp(message, "-ROOMS") == 0){
            printf("%s\n","it is -ROOMS" );
            return send_roomlist_message(connfd);
    }
    else if(strcmp(message, "-LEAVE") == 0){
      char message[1024] = "GoodBye";
      return send_message(connfd, message);

    }
    else if(strcmp(message, "-WHO") == 0){
          printf("%s\n","it is -WHO" );
          return send_userlist_message(connfd);
    }
    else if(strcmp(message, "-HELP") == 0){
          printf("%s\n","it is -HELP" );
          return send_helplist_message(connfd);
    }
    else if(strcmp(message, "-nickname message") == 0){

    }
    else{
      char tempMessage[1024] ="command not recognized";
      strcat(message,tempMessage);
      printf("%s\n", message );
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

  printf("********** SIMPLE_MESSAGE() **********\n");
  while((n=receive_message(connfd, message))>0) {
    // message[n] = '\0';  // null terminate message (for string operations)
    printf("Server received a meesage of %d bytes: %s\n", (int)n, message);
    n = process_message(connfd, message);
  }

  printf("********** SIMPLE_MESSAGE() ********** finish\n");
}

#endif