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

string* Client::readFile(char* fileName){

	ifstream file(fileName);
	string* info=new string[MESSAGE_LINES];

  	if (file.is_open())
  	{
  		int i=0;
    	while (getline(file,info[i])
    		&& i<MESSAGE_LINES){
    		i++;
    	}
    	file.close();
    	lines=i;
    }
    else{
    	printf("[-]Not such file exits.\n");
    }
    return info; 
}

void Client::scripting(char* fileName){
	string* info=readFile(fileName);
	printf("%s\n\n",fileName );
	// printf("%s\n",(*(info+5)).c_str());
  	for(int i=0; i<lines; i++){
  		// printf("%s\n",(*(info+i)).c_str());
  	  // char* copy;
      // strcpy(copy, (*(info+i)).c_str());
  		// char* inst=(*(info+i)).c_str();
  		char buffer[BUF_SIZE];
  		strcpy(buffer,(*(info+i)).c_str());
  		if(buffer[0]!='\n') buffer[strlen(buffer)] = '\0';

  		printf("\n%s: %s\n", getName(),buffer);
      	sendMessage(buffer);
      	usleep((unsigned int)100000);
      	// sleep(2);
      	// sendMessage("\\ROOMS");

  	}
}


void Client::sendMessage(const char* inputBuffer){
	if(socket_status<0){
		throw SOCKET_EX;
	}else if(connection_status<0){
		throw CONNECTION_EX;
	}else if(sizeof(inputBuffer)>BUF_SIZE){
		throw BUF_SIZE_EX;
	}

	char outputBuffer[2000];
	send(socket_status, inputBuffer, strlen(inputBuffer), 0);

	init_outputBuffer();
	receive_status =recv(socket_status, outputBuffer, BUF_SIZE, 0);
	if(receive_status<0)
		throw RECEIVE_EX;


	if(strncmp(outputBuffer, "SERVER[0]:",10) == 0){

		printf("%s\n", outputBuffer);
		close(socket_status);
		socket_status=-1;
		printf("[-]Disconnected from server.\n");
		throw LEAVE_EX;
	}
	else if(strncmp(outputBuffer, "SERVER[1]:",10) == 0){
		printf("%s\n", outputBuffer);
		char infoCopy[2000];
		strcpy(infoCopy,inputBuffer);

		char* token = strtok (infoCopy," ");
		token = strtok(NULL, " ");
		name=token;
	}
	else if(strncmp(outputBuffer, "SERVER[2]:",10) == 0){
		char* convert=&(outputBuffer[0]);
		string toConvert(convert);
		string whisper = toConvert.substr(11, toConvert.length()-1);
		printf("%s\n", whisper); 
	}
	else{
		printf("%s\n", outputBuffer);
	}

}

void Client::init_outputBuffer(){
	bzero(outputBuffer, 2000);
}
