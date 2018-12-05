#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include "server.h"
#include "user.h"

using namespace std;

typedef struct Message{
	string message;
	int user_id;
} Message;

#endif