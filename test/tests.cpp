#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>

#include "../src/client.h"

using namespace std;

#define BUF_SIZE 2000
#define USRNAME_SIZE 15
#define SOCKET_EX 1
#define CONNECTION_EX 2
#define BUF_SIZE_EX 3
#define RECEIVE_EX 4
#define LEAVE_EX 5


TEST(Test, Create_Socket)
{
  Client client1(3000);

  try{
      client1.create_socket();
  }catch(int status){
      if(status==SOCKET_EX)
        printf("Server Connection with port 3000 has not up yet.\n");
  }

  bool code=(client1.getSocket()>=0);

  ASSERT_EQ(1, code) << "Failure in create_socket test\n"
  <<"create_socket returned status "<<code<<", but should have returned 1";
}

TEST(Test, Create_Connection)
{
  Client client1(3000);

  try{
      client1.create_socket();
      client1.connecting();

  }catch(int status){
      if(status==CONNECTION_EX)
        printf("Server Connection with port 3000 has not up yet (server is not up yet).\n");  
  }

  bool code=(client1.getConnection()>=0);

  ASSERT_EQ(1, code) << "Failure in create_connection test\n"
  <<"function connecting returned status "<<code<<", but should have returned 1";
  close(client1.getSocket());
}

TEST(Test, Send_Simple_Message)
{
  Client client1(3000);
  string* c1_info = client1.readFile("client1.txt");
  Client client2(3000);
  string* c2_info = client2.readFile("client2.txt");
  char outputBuffer[2000];
  for(int i=0; (c1_info[i]).length()>0; i++){
      strcpy(outputBuffer, (c1_info[i]).c_str());
      printf("%s\n---%d",outputBuffer,sizeof(c1_info[i]));
  }

  for(int i=0; (c2_info[i]).length()>0; i++){
      strcpy(outputBuffer, (c2_info[i]).c_str());
      printf("%s\n", outputBuffer);
  }


  try{
      client1.create_socket();
      client1.connecting();

      client2.create_socket();
      client2.connecting();

  }catch(int status){
      printf("Server Connection with port 3000 has not up yet.\n");  
  }



  while(true){
      
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
      }
  }

  bool code=0;

  ASSERT_EQ(1, code) << "Failure in create_connection test\n"
  <<"function connecting returned status "<<code<<", but should have returned 1";
  close(client1.getSocket());
}


// test create_file return code 1 for success
// TEST(ClientTest, test1)
// {
//   myFileSystem f((char *)"disk0");
//   int code = f.create_file((char *)"test.c", 1);
//   ASSERT_EQ(1, code);
//   f.close_disk();
// }

// // test delete_file return code -1 for failure
// TEST(ClientTest, test2) {
//   myFileSystem f((char *)"disk0");
//   int code = f.delete_file((char *)"test.c");
//   ASSERT_EQ(-1, code);
//   f.close_disk();
// }

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
