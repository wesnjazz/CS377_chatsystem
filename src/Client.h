#ifndef _CLIENT_H
#define _CLIENT_H

//// DO NOT MODIFY ANYTHING IN THIS FILE //////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <fstream>

using namespace std;

class Client {
	public:
		Client(int portNum); // constructor to initialize locks and conditional variables
		void create_socket();
		void connecting();
		void sendMessage(char* inputBuffer);
		string* readFile(string fileName);
		
		int getSocket(){return socket_status;			}
		int getReceive(){return receive_status;			}
		int getConnection(){return connection_status;	}

	private:
		int port;
		int socket_status;
		int connection_status;
		int receive_status;
		char outputBuffer[2000];
};

#endif
