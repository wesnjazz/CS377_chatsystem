#include "Client.h"

#define BUF_SIZE 2000
#define USRNAME_SIZE 15
#define SOCKET_EX 1
#define CONNECTION_EX 2
#define BUF_SIZE_EX 3
#define RECEIVE_EX 4
#define LEAVE_EX 5
#define MESSAGE_LINES 30

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
	printf("[+]Connected to Server.\n");
}

string* Client::readFile(string fileName){

	ifstream file(fileName.c_str());
	string* info=new string[MESSAGE_LINES];

  	if (file.is_open())
  	{
  		int i=0;
    	while (getline(file,info[i])
    		&& i<MESSAGE_LINES){
    		i++;
    	}
    	file.close();
    }
    else{
    	printf("[-]Not such file exits.\n");
    }
    return info; 
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
		socket_status=-1;
		printf("[-]Disconnected from server.\n");
		throw LEAVE_EX;
	}
}

