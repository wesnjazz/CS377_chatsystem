/*******************
  GLOBAL VARIABLES 
********************/
#define MAX_CLIENTS 100
#define MAX_ROOM_NUM 20
#define MAX_USER_IN_A_ROOM 20

#define MAX_USER_NAME 15
#define MAX_ROOM_NAME 30
#define MAX_MSG_CHAR 100

#define BUF_MAX_20LINES 20
#define BUF_MAX_100CHARS 100
#define BUF_SMALL_300 300

#define DEFAULT_USR_NAME "ChangeName"

/* Max text line length */
#define MAXLINE 8192

/* Second argument to listen() */
#define LISTENQ 1024

/**********************
  Structs
***********************/
typedef struct User{
  // User's nickname
  char user_name[MAX_USER_NAME];
  // User's socket
  int socket;
  // User's current location
  int room_id;
} User;
typedef struct Room{
  // Room name
  char room_name[MAX_ROOM_NAME];
  // room_id is matched with corresponding index of Room_list[],
  // e.g., Room number 0 has room_id of 0 and it is at Room_list[0]
  int room_id;
  // keep track a number of current clients in a specific Room
  int num_users;
  // Client list in a specific Room
  int socket_list_in_Room[MAX_USER_IN_A_ROOM];
  // Circular buffer which records all messages from clients in a specific Room
  char chat_buffer[BUF_MAX_20LINES][BUF_MAX_100CHARS];
  // HEAD pointer of chat_buffer
  int chat_buffer_HEAD;
  // TAIL pointer of chat_buffer
  int chat_buffer_TAIL;
} Room;

Room Room_list[MAX_ROOM_NUM];
User User_list[MAX_CLIENTS];

int num_room_list = 0;
int num_user_list = 0;

/* Simplifies calls to bind(), connect(), and accept() */
typedef struct sockaddr SA;

// A lock for the message buffer.
pthread_mutex_t lock;
pthread_mutex_t room;
pthread_mutex_t user;

// We will use this as a simple circular buffer of incoming messages.
char message_buf[BUF_MAX_20LINES][BUF_MAX_100CHARS];
char entire_message_buf[BUF_MAX_20LINES * BUF_MAX_100CHARS];

// This is an index into the message buffer.
int msgi = 0;

int init_chat_buffer_in_Room(int room_id);
int get_User_list_index_by_socket(int connfd);
int find_empty_spot_socket_list_in_Room(int room_id);
int find_User_socket_idx_from_Room(int connfd, int old_room_id);
int remove_Room_from_list(int room_id);
int send_message(int connfd, char *message);

