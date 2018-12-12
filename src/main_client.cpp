#include "Client.h"

#define BUF_SIZE 2000
#define USRNAME_SIZE 15
#define SOCKET_EX 1
#define CONNECTION_EX 2
#define BUF_SIZE_EX 3
#define RECEIVE_EX 4
#define LEAVE_EX 5

int main(int argc, char *argv[]){

  // Check the program arguments and print usage if necessary.

  	if (argc != 2 && argc!=3) {
    	fprintf(stderr, "usage: %s <port>\n", argv[0]);
      fprintf(stderr, "or usage: %s <port> <fileName>\n", argv[0]);
    	exit(0);
  	}

  	Client c(atoi(argv[1]));

  	try{
  		c.create_socket();
  		c.connecting();
  	}catch(int status){
  		if(status==SOCKET_EX)
  			printf("[-]Error in creating socket.\n");
  		else if(status==CONNECTION_EX)
  			printf("[-]Error in creating connection.\n");
  		if(status==SOCKET_EX || status == CONNECTION_EX)
			exit(1);
  	}

    char buffer[BUF_SIZE];
    if(argc==2){
      while(true){
        printf("%s: ", c.getName());
		    fgets(buffer, sizeof(buffer), stdin);	// read whole line including '\n'
		    if(buffer[0]!='\n') buffer[strlen(buffer)-1] = '\0';	// delete '\n' from the input string unless if user input only ENTER.

		    try{
			     c.sendMessage(buffer);
		    }catch(int status){
			     if(status==SOCKET_EX)
  				    printf("[-]Error in creating socket.\n");
  			   else if(status==CONNECTION_EX)
  				    printf("[-]Error in creating connection.\n");
  			   else if(status==BUF_SIZE_EX)
				      printf("[-]Error in too large sending message.\n");
			     else if(status==RECEIVE_EX)
				      printf("[-]Error in receiving data.\n");

			     if(status==LEAVE_EX || status==SOCKET_EX || status == CONNECTION_EX)
				      exit(1);
		    }
	    }
    }
    else{
      try{
           c.scripting(argv[2]);
        }catch(int status){
           if(status==SOCKET_EX)
              printf("[-]Error in creating socket.\n");
           else if(status==CONNECTION_EX)
              printf("[-]Error in creating connection.\n");
           else if(status==BUF_SIZE_EX)
              printf("[-]Error in too large sending message.\n");
           else if(status==RECEIVE_EX)
              printf("[-]Error in receiving data.\n");

           if(status==LEAVE_EX || status==SOCKET_EX || status == CONNECTION_EX)
              exit(1);
        }

    }
  	return 0;
}