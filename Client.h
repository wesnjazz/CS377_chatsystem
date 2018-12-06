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

using namespace std;

class Client {
	public:
		Client(int portNum); // constructor to initialize locks and conditional variables
		void communicate();
	private:
		int port;
		int create_socket();
		int connecting(int sockfd);
};

#endif
