#ifndef USER_H
#define USER_H

#include <string>
#include "srd_server.h"
#include "room.h"

using namespace std;

typedef struct User{
	string user_name;
	int user_id;
	int room_id;
} User;

#endif