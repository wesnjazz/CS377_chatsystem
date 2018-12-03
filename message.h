#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include "srd_server.h"
#include "user.h"

using namespace std;

typedef struct Message{
	string user_name;
	int user_id;
	int room_id;
} Message;

#endif