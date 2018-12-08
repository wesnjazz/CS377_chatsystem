#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>

#include "../src/client.h"

using namespace std;

class Test : public ::testing::Test {
 protected:
  Test() {
    system("./server 3000");
  }
};

TEST(Test, CreateSocket)
{
  Client client1(3000);
  client1.create_socket();
  client1.connecting();
  bool status=(client1.getSocket()>0);


  ASSERT_EQ(1, status) << "Failure in create_file test CreateFile_BasicCreate\n"
  <<"create_file on test1.c returned "<<status<<", but should have returned 1";

  status=0;
  status=(client1.getConnection()>0);
  ASSERT_EQ(1, status) << "Failure in create_file test CreateFile_BasicCreate\n"
  <<"create_file on test1.c returned "<<status<<", but should have returned 1";

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
