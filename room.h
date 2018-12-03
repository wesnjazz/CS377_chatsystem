#ifndef ROOM_H
#define ROOM_H

#include <string>
#include "srd_server.h"
#include "user.h"

using namespace std;

typedef struct Room{
	string room_name;
	int room_id;
	User **user_list;
} Room;

#endif