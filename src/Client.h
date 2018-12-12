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
		void sendMessage(const char* inputBuffer);
		string* readFile(char* fileName);
		void scripting(char* fileName);
		
		int getSocket(){return socket_status;			}
		// int getReceive(){return receive_status;			}
		int getConnection(){return connection_status;	}
		char* getName(){return name;					}


	private:
		int port;
		int socket_status;
		int connection_status;
		int receive_status;
		int lines=0;
		char* name="client";
		// char outputBuffer[2000];
};

#endif
