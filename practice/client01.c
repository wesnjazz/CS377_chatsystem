#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 2000

int main(int argc, char *argv[]){
	int sockfd, ret;
	struct sockaddr_in serverAddr;

	char buffer[BUF_SIZE];
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd<0){
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Client Socket is created.\n");

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[1]));
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	ret=connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	if(ret<0){
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Connected so Server.\n");

	while(1){
		printf("Client: ");
		scanf("%s", buffer);
		send(sockfd, buffer, strlen(buffer), 0);

		if(strcmp(buffer, ":exit") == 0){
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

	return 0;
}