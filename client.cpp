#include "Client.h"

#define BUF_SIZE 2000
#define USRNAME_SIZE 15

Client::Client(int portInst){
	port=portInst;
}

int Client::create_socket(){
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd<0){
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Client Socket is created.\n");
	return sock_fd;
}

int Client::connecting(int sockfd){
	struct sockaddr_in serverAddr;

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	int ret =connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	if(ret<0){
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Connected so Server.\n");
}

void Client::communicate(){

	int sockfd=create_socket();;
	connecting(sockfd);

	char buffer[BUF_SIZE];
	char user_name[USRNAME_SIZE] = "client";

	while(true){
		printf("%s: ", user_name);
		fgets(buffer, sizeof(buffer), stdin);	// read whole line including '\n'
		if(buffer[0]!='\n') buffer[strlen(buffer)-1] = '\0';	// delete '\n' from the input string unless if user input only ENTER.
		send(sockfd, buffer, strlen(buffer), 0);

		if(strcmp(buffer, "\\QUIT") == 0){
			close(sockfd);
			printf("[-]Disconnected from server.\n");
			exit(1);
		}

		if(recv(sockfd, buffer, BUF_SIZE, 0)<0){
			printf("[-]Error in receiving data.\n");
		}else{
			printf("Server: %s\n", buffer);
		}
	}
}

int main(int argc, char *argv[]){

  // Check the program arguments and print usage if necessary.

  if (argc != 2) {

    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  Client c(atoi(argv[1]));
  c.communicate();

  return 0;

}