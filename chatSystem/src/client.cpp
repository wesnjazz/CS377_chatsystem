#include "Client.h"

#define BUF_SIZE 2000
#define USRNAME_SIZE 15
#define SOCKET_EX 1
#define CONNECTION_EX 2
#define BUF_SIZE_EX 3
#define RECEIVE_EX 4
#define LEAVE_EX 5

Client::Client(int portInst){
	port=portInst;
	socket_status= connection_status =receive_status=-1;
}

void Client::create_socket(){
	socket_status = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_status<0){
		throw SOCKET_EX;
	}
	printf("[+]Client Socket is created.\n");
}

void Client::connecting(){

	if(socket_status<0){
		throw SOCKET_EX;
	}

	struct sockaddr_in serverAddr;

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	connection_status =connect(socket_status, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	if(connection_status<0){
		throw CONNECTION_EX;
	}
	printf("[+]Connected so Server.\n");
}

string* Client::readFile(string fileName){

	ifstream file(fileName.c_str());
	int max_line=30;
	string* info=new string[max_line];
  	if (file.is_open())
  	{
  		int i=0;
    	while (getline(file,info[i])
    		&& i<max_line){
    		i++;
    	}
    }

    file.close();
}

void Client::sendMessage(char* inputBuffer){
	if(socket_status<0){
		throw SOCKET_EX;
	}else if(connection_status<0){
		throw CONNECTION_EX;
	}else if(sizeof(inputBuffer)>BUF_SIZE){
		throw BUF_SIZE_EX;
	}

	send(socket_status, inputBuffer, strlen(inputBuffer), 0);

	receive_status =recv(socket_status, outputBuffer, BUF_SIZE, 0);
	if(receive_status<0)
		throw RECEIVE_EX;

	printf("Server:\t\t %s\n", outputBuffer);


	if(strcmp(inputBuffer, "\\LEAVE") == 0
		&&strncmp(outputBuffer, "GoodBye",9) == 0){

		close(socket_status);
		printf("[-]Disconnected from server.\n");
		throw LEAVE_EX;
	}
}


int main(int argc, char *argv[]){

  // Check the program arguments and print usage if necessary.

  	if (argc != 2) {
    	fprintf(stderr, "usage: %s <port>\n", argv[0]);
    	exit(0);
  	}

  	Client c(atoi(argv[1]));

  	try{
  		c.create_socket();
  		c.connecting();
  	}catch(int status){
  		if(status==SOCKET_EX)
  			printf("[-]Error in creating socket.\n");
  		else if(status==CONNECTION_EX)
  			printf("[-]Error in creating connection.\n");
  		if(status==SOCKET_EX || status == CONNECTION_EX)
			exit(1);
  	}

	char user_name[USRNAME_SIZE] = "client";
	char buffer[BUF_SIZE];

	while(true){
		printf("%s: ", user_name);
		fgets(buffer, sizeof(buffer), stdin);	// read whole line including '\n'
		if(buffer[0]!='\n') buffer[strlen(buffer)-1] = '\0';	// delete '\n' from the input string unless if user input only ENTER.

		try{
			c.sendMessage(buffer);
		}catch(int status){
			if(status==SOCKET_EX)
  				printf("[-]Error in creating socket.\n");
  			else if(status==CONNECTION_EX)
  				printf("[-]Error in creating connection.\n");
  			else if(status==BUF_SIZE_EX)
				printf("[-]Error in too large sending message.\n");
			else if(status==RECEIVE_EX)
				printf("[-]Error in receiving data.\n");

			if(status==LEAVE_EX || status==SOCKET_EX || status == CONNECTION_EX)
				exit(1);
		}
	}

  	return 0;

}