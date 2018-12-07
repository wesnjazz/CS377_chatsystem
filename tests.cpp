#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>

#include "../src/fs.h"

using namespace std;

class FSTest : public ::testing::Test {
 protected:
  FSTest() {
    system("rm -f disk0");
    system("rm -f disk1");
    system("./create_fs disk0");
    system("./create_fs disk1");
  }
};

//Tests for Students
TEST_F(FSTest, CreateFile_BasicCreate)
{
  myFileSystem f((char *)"disk0");
  int code;
  code = f.create_file((char *)"test1.c", 3);
  ASSERT_EQ(1, code) << "Failure in create_file test CreateFile_BasicCreate\n"
  <<"create_file on test1.c returned "<<code<<", but should have returned 1";
  code = f.create_file((char *)"test2.c", 1);
  ASSERT_EQ(1, code) << "Failure in create_file test CreateFile_BasicCreate\n"
  <<"create_file on test2.c returned "<<code<<", but should have returned 1";
  code = f.create_file((char *)"test3.c", 6);
  ASSERT_EQ(1, code) << "Failure in create_file test CreateFile_BasicCreate\n"
  <<"create_file on test3.c returned "<<code<<", but should have returned 1";
  code = f.create_file((char *)"test4.c", 0); //This can happen
  ASSERT_EQ(1, code) << "Failure in create_file test CreateFile_BasicCreate\n"
  <<"create_file on test4.c returned "<<code<<", but should have returned 1";
  f.close_disk();
  
  fstream fl;
  char freelist[128];
  char ind[48];
  fl.open((char *)"disk0",fstream::in | fstream::out | fstream::binary);
  
  fl.seekg(0);
  fl.read(freelist,128);
  int usedblocks=0;
  for(int i=0;i<128;i++){usedblocks+=(freelist[i]==1?1:0);}
  ASSERT_EQ(11, usedblocks) << "Failure in create_file test CreateFile_BasicCreate\n"
  <<usedblocks<<" blocks are being used, 11 blocks should be\n"
  <<"Perhaps you didn't write the modified free block list to disk, or did so incorrectly";
  
  bool used; //this won't work once delete gets in ivolved
  int nused=0;
  for(int i=0;i<16;i++)
  {
    fl.seekg(128+48*i);
    fl.read(ind,48);
    used=false;
    for(int j=0;j<48;j++){used=(used?used:(ind[j]!=0));}
    nused+=(used?1:0);
  }
  ASSERT_EQ(4, nused) << "Failure in create_file test CreateFile_BasicCreate\n"
  <<nused<<" inodes are being used, 4 inodes should be\n"
  <<"Perhaps you you didn't write the modified inodes to disk, or did so incorrectly";
  
  fl.close();
}

TEST_F(FSTest, CreateFile_FileTooLarge)
{
  myFileSystem f((char *)"disk0");
  int code = f.create_file((char *)"test.c", 9);
  ASSERT_EQ(-1, code) << "Failure in create_file test CreateFile_FileTooLarge\n"
  <<"create_file on test.c returned "<<code<<", but should have returned -1\n"
  <<"Perhaps you aren't checking if the requested file is larger than 8 blocks";
  f.close_disk();
}

TEST_F(FSTest, CreateFile_FillDisk)
{
  myFileSystem f((char *)"disk0");
  int code;
  char name[8];
  strcpy(name,"file");
  for (int i = 0; i < 15; i++)
  {
    name[4]=65+i;
    code = f.create_file(name, 8);
    ASSERT_EQ(1, code) << "Failure in create_file test CreateFile_FillDisk\n"
    <<"create_file on "<<name<<" (for loop i = "<<i<<") returned "<<code<<", but should have returned 1\n"
    <<"If you are passing CreateFile_BasicCreate, this shouldn't happen";
  }
  code = f.create_file((char *)"fileX", 7);
  ASSERT_EQ(1, code) << "Failure in create_file test CreateFile_FillDisk\n"
  <<"create_file on fileX returned "<<code<<", but should have returned 1\n"
  <<"Perhaps you are marking too many blocks as used in the free block list";
  f.close_disk();
}

TEST_F(FSTest, CreateFile_DiskTooFull)
{
  myFileSystem f((char *)"disk0");
  int code;
  char name[8];
  strcpy(name,"file");
  for (int i = 0; i < 15; i++)
  {
    name[4]=65+i;
    code = f.create_file(name, 8);
    ASSERT_EQ(1, code) << "Failure in create_file test CreateFile_DiskTooFull\n"
    <<"create_file on "<<name<<" (for loop i = "<<i<<") returned "<<code<<", but should have returned 1\n"
    <<"If you are passing CreateFile_BasicCreate, this shouldn't happen";
  }
  code = f.create_file((char *)"fileX", 8);
  ASSERT_EQ(-1, code) << "Failure in create_file test CreateFile_DiskTooFull\n"
  <<"create_file on test.c returned "<<code<<", but should have returned -1\n"
  <<"Perhaps you aren't marking enough blocks as used in the free block list\n"
  <<"Perhaps you aren't checking if you have enough blocks for the file's size";
  f.close_disk();
}

TEST_F(FSTest, CreateFile_TooManyFiles)
{
  myFileSystem f((char *)"disk0");
  int code;
  char name[8];
  strcpy(name,"file");
  for (int i = 0; i < 16; i++)
  {
    name[4]=65+i;
    code = f.create_file(name, 1);
    ASSERT_EQ(1, code) << "Failure in create_file test CreateFile_TooManyFiles\n"
    <<"create_file on "<<name<<" (for loop i = "<<i<<") returned "<<code<<", but should have returned 1\n"
    <<"If you are passing CreateFile_BasicCreate, this shouldn't happen";
  }
  code = f.create_file((char *)"fileX", 1);
  ASSERT_EQ(-1, code) << "Failure in create_file test CreateFile_TooManyFiles\n"
  <<"create_file on fileX returned "<<code<<", but should have returned -1\n"
  <<"Perhaps you aren't properly marking inodes as used\n"
  <<"Perhaps you are trying to use an inode that is already used";
  f.close_disk();
}

TEST_F(FSTest, CreateFile_DuplicateName)
{
  myFileSystem f((char *)"disk0");
  int code;
  code = f.create_file((char *)"test.c", 1);
  ASSERT_EQ(1, code) << "Failure in create_file test CreateFile_DuplicateName\n"
  <<"create_file on test.c returned "<<code<<", but should have returned 1\n"
  <<"If you are passing CreateFile_BasicCreate, this shouldn't happen";
  code = f.create_file((char *)"test.c", 1);
  ASSERT_EQ(-1, code) << "Failure in create_file test CreateFile_DuplicateName\n"
  <<"create_file on test.c returned "<<code<<", but should have returned -1\n"
  <<"Perhaps you aren't checking for uniqueness of names";
  f.close_disk();
}

TEST_F(FSTest, CreateFile_LongName)
{
  myFileSystem f((char *)"disk0");
  int code;
  code = f.create_file((char *)"test.txt", 1);
  ASSERT_EQ(1, code) << "Failure in create_file test CreateFile_LongName\n"
  <<"create_file on test.txt returned "<<code<<", but should have returned 1\n"
  <<"If you are passing CreateFile_BasicCreate, this shouldn't happen";
  code = f.create_file((char *)"test.txt", 1);
  ASSERT_EQ(-1, code) << "Failure in create_file test CreateFile_LongName\n"
  <<"create_file on test.txt returned "<<code<<", but should have returned -1\n"
  <<"Perhaps you have an issue comparing names of length 8\n"
  <<"When you read in an inode who's name has length 8, it lacks a terminating null character\n"
  <<"When the function's input name has length 8, it still actually has name[8]='\0'\n"
  <<"The inode's name[8] is probaly the first byte of the inode's size, not '\0'\n"
  <<"Although these two names are equal on the first 8 bytes, strcmp sees them as different on the ninth\n"
  <<"You DON'T want to change the inode's name[8] (since that is part of size)\n"
  <<"The easiest fix is probably to just use strncmp()";
  f.close_disk();
}

TEST_F(FSTest, DeleteFile_NonexistantFile)
{
  myFileSystem f((char *)"disk0");
  int code = f.delete_file((char *)"test.c");
  ASSERT_EQ(-1, code) << "Failure in delete_file test DeleteFile_NonexistantFile\n"
  <<"delete_file on test.c returned "<<code<<", but should have returned -1\n"
  <<"Perhaps you are not returning -1 when there is no file of that name to delete.";
  f.close_disk();
}

TEST_F(FSTest, CreateDeleteHarmony_BasicDelete)
{
  myFileSystem f((char *)"disk0");
  int code;
  code = f.create_file((char *)"test.c", 1);
  ASSERT_EQ(1, code) << "Failure in delete_file test CreateDeleteHarmony_BasicDelete\n"
  <<"create_file on test.c returned "<<code<<", but should have returned 1\n"
  <<"If you are passing CreateFile_BasicCreate, this shouldn't happen";
  code = f.delete_file((char *)"test.c");
  ASSERT_EQ(1, code) << "Failure in delete_file test CreateDeleteHarmony_BasicDelete\n"
  <<"delete_file on test.c returned "<<code<<", but should have returned 1";
  
  char name[8];
  strcpy(name,"file");
  for (int i = 0; i < 15; i++)
  {
    name[4]=65+i;
    code = f.create_file(name, 8);
    ASSERT_EQ(1, code) << "Failure in delete_file test CreateDeleteHarmony_BasicDelete\n"
    <<"create_file on "<<name<<" (for loop i = "<<i<<") returned "<<code<<", but should have returned 1\n"
    <<"Perhaps when deleting you aren't setting the inode's used to 0 and writing to disk";
  }
  code = f.create_file((char *)"test.c", 7);
  ASSERT_EQ(1, code) << "Failure in delete_file test CreateDeleteHarmony_BasicDelete\n"
  <<"create_file on test.c returned "<<code<<", but should have returned 1\n"
  <<"Perhaps when deleting you aren't setting the inode's used to 0 and writing to disk\n"
  <<"Perhaps when deleting you changing the free block list and writing to the disk\n";
  
  f.close_disk();
}

TEST_F(FSTest, CreateDeleteHarmony_HiddenDuplicateFile)
{
  myFileSystem f((char *)"disk0");
  int code;
  code = f.create_file((char *)"test1.c", 5);
  ASSERT_EQ(1, code) << "Failure in create_file test CreateDeleteHarmony_HiddenDuplicateFile\n"
  <<"create_file on test1.c returned "<<code<<", but should have returned 1\n"
  <<"If you are passing CreateFile_BasicCreate, this shouldn't happen";
  code = f.create_file((char *)"test2.c", 5);
  ASSERT_EQ(1, code) << "Failure in create_file test CreateDeleteHarmony_HiddenDuplicateFile\n"
  <<"create_file on test2.c returned "<<code<<", but should have returned 1\n"
  <<"If you are passing CreateFile_BasicCreate, this shouldn't happen";
  code = f.create_file((char *)"test3.c", 5);
  ASSERT_EQ(1, code) << "Failure in create_file test CreateDeleteHarmony_HiddenDuplicateFile\n"
  <<"create_file on test3.c returned "<<code<<", but should have returned 1\n"
  <<"If you are passing CreateFile_BasicCreate, this shouldn't happen";
  code = f.delete_file((char *)"test1.c");
  ASSERT_EQ(1, code) << "Failure in create_file test CreateDeleteHarmony_HiddenDuplicateFile\n"
  <<"delete_file on test1.c returned "<<code<<", but should have returned 1\n"
  <<"If you are passing CreateDeleteHarmony_BasicDelete, this shouldn't happen";
  code = f.create_file((char *)"test3.c", 1);
  ASSERT_EQ(-1, code) << "Failure in create_file test CreateDeleteHarmony_HiddenDuplicateFile\n"
  <<"create_file on test3.c returned "<<code<<", but should have returned -1\n"
  <<"Perhaps you are not checking all the inodes for existing files of the same name";
  f.close_disk();
}

TEST_F(FSTest, CreateDeleteHarmony_LongName)
{
  myFileSystem f((char *)"disk0");
  int code;
  code = f.create_file((char *)"test.txt", 1);
  ASSERT_EQ(1, code) << "Failure in delete_file test CreateDeleteHarmony_LongName\n"
  <<"create_file on test.txt returned "<<code<<", but should have returned 1\n"
  <<"If you are passing CreateFile_BasicCreate, this shouldn't happen";
  code = f.delete_file((char *)"test.txt");
  ASSERT_EQ(1, code) << "Failure in delete_file test CreateDeleteHarmony_LongName\n"
  <<"delete_file on test.txt returned "<<code<<", but should have returned 1\n"
  <<"Perhaps you have an issue comparing names of length 8\n"
  <<"When you read in an inode who's name has length 8, it lacks a terminating null character\n"
  <<"When the function's input name has length 8, it still actually has name[8]='\0'\n"
  <<"The inode's name[8] is probaly the first byte of the inode's size, not '\0'\n"
  <<"Although these two names are equal on the first 8 bytes, strcmp sees them as different on the ninth\n"
  <<"You DON'T want to change the inode's name[8] (since that is part of size)\n"
  <<"The easiest fix is probably to just use strncmp()";
  f.close_disk();
}

TEST_F(FSTest, LS_BasicLS)
{
  myFileSystem f((char *)"disk0");
  int code = f.ls();
  ASSERT_EQ(1, code) << "Failure in ls test LS_BasicLS\n"
  <<"ls returned "<<code<<", but should have returned 1\n"
  <<"This test is just a reminder to implement ls";
  f.close_disk();
}

TEST_F(FSTest, Write_BasicWrite)
{
  myFileSystem f((char *)"disk0");
  int code;
  code = f.create_file((char *)"test.c", 5);
  ASSERT_EQ(1, code) << "Failure in write test Write_BasicWrite\n"
  <<"create_file on test.c returned "<<code<<", but should have returned 1\n"
  <<"If you are passing CreateFile_BasicCreate, this shouldn't happen";
  
  char buf[1024];
  for(int i=0;i<1024;i++){buf[i]=1;}
  for(int i=0;i<3;i++)
  {
    code = f.write((char *)"test.c", 2*i, buf);
    ASSERT_EQ(1, code) << "Failure in write test Write_BasicWrite\n"
    <<"write to test.c block "<<(2*i)<<" returned "<<code<<", but should have returned 1";
  }
  f.close_disk();
  
  fstream fl;
  fl.open((char *)"disk0",fstream::in | fstream::out | fstream::binary);
  
  bool written; //this won't work once delete gets in ivolved
  int nwritten=0;
  for(int i=1;i<128;i++)
  {
    fl.seekg(1024*i);
    fl.read(buf,1024);
    written=true;
    for(int j=0;j<1024;j++){written=(written?(buf[j]==1):written);}
    nwritten+=(written?1:0);
  }
  ASSERT_EQ(3, nwritten) << "Failure in write test Write_BasicWrite\n"
  <<nwritten<<" blocks were properly written to, 3 blocks should have been\n"
  <<"Perhaps you aren't writing the buffer to disk, or did so incorrectly";
}

TEST_F(FSTest, Write_NonexistantFile)
{
  myFileSystem f((char *)"disk0");
  char buf[1024];
  for(int i=0;i<1024;i++){buf[i]=1;}
  int code = f.write((char *)"test.c", 0, buf);
  ASSERT_EQ(-1, code) << "Failure in write test Write_NonexistantFile\n"
  <<"write to test.c block 0 returned "<<code<<", but should have returned -1\n"
  <<"Perhaps you are not returning -1 when there is no file of that name to write to";
  f.close_disk();
}

TEST_F(FSTest, Write_InvalidBlock)
{
  myFileSystem f((char *)"disk0");
  int code;
  code = f.create_file((char *)"test.c", 5);
  ASSERT_EQ(1, code) << "Failure in write test Write_InvalidBlock\n"
  <<"create_file on test.c returned "<<code<<", but should have returned 1\n"
  <<"If you are passing CreateFile_BasicCreate, this shouldn't happen";
  
  char buf[1024];
  for(int i=0;i<1024;i++){buf[i]=1;}
  code = f.write((char *)"test.c", 5, buf);
  ASSERT_EQ(-1, code) << "Failure in write test Write_InvalidBlock\n"
  <<"write to test.c block 5 returned "<<code<<", but should have returned -1\n"
  <<"Perhaps you are not returning -1 when the requested block doesn't exist in the file";
  f.close_disk();
}

TEST_F(FSTest, Write_LongName)
{
  myFileSystem f((char *)"disk0");
  int code;
  code = f.create_file((char *)"test.txt", 1);
  ASSERT_EQ(1, code) << "Failure in write test Write_LongName\n"
  <<"create_file on test.txt returned "<<code<<", but should have returned 1\n"
  <<"If you are passing Create_BasicCreate, this shouldn't happen";
  
  char buf[1024];
  for(int i=0;i<1024;i++){buf[i]=1;}
  code = f.write((char *)"test.txt", 0, buf);
  ASSERT_EQ(1, code) << "Failure in write test Write_LongName\n"
  <<"write to test.txt block 0 returned "<<code<<", but should have returned 1\n"
  <<"Perhaps you have an issue comparing names of length 8\n"
  <<"When you read in an inode who's name has length 8, it lacks a terminating null character\n"
  <<"When the function's input name has length 8, it still actually has name[8]='\0'\n"
  <<"The inode's name[8] is probaly the first byte of the inode's size, not '\0'\n"
  <<"Although these two names are equal on the first 8 bytes, strcmp sees them as different on the ninth\n"
  <<"You DON'T want to change the inode's name[8] (since that is part of size)\n"
  <<"The easiest fix is probably to just use strncmp()";
  f.close_disk();
}

TEST_F(FSTest, Read_NonexistantFile)
{
  myFileSystem f((char *)"disk0");
  char buf[1024];
  for(int i=0;i<1024;i++){buf[i]=1;}
  int code = f.write((char *)"test.c", 0, buf);
  ASSERT_EQ(-1, code) << "Failure in read test Read_NonexistantFile\n"
  <<"read from test.c block 0 returned "<<code<<", but should have returned -1\n"
  <<"Perhaps you are not returning -1 when there is no file of that name to read to";
  f.close_disk();
}

TEST_F(FSTest, Read_InvalidBlock)
{
  myFileSystem f((char *)"disk0");
  int code;
  code = f.create_file((char *)"test.c", 5);
  ASSERT_EQ(1, code) << "Failure in read test Read_InvalidBlock\n"
  <<"create_file on test.c returned "<<code<<", but should have returned 1\n"
  <<"If you are passing CreateFile_BasicCreate, this shouldn't happen";
  
  char buf[1024];
  code = f.read((char *)"test.c", 5, buf);
  ASSERT_EQ(-1, code) << "Failure in read test Read_InvalidBlock\n"
  <<"read from test.c block 5 returned "<<code<<", but should have returned -1\n"
  <<"Perhaps you are not returning -1 when the requested block doesn't exist in the file";
  f.close_disk();
}

TEST_F(FSTest, ReadWriteHarmony_BasicRead)
{
  myFileSystem f((char *)"disk0");
  int code;
  code = f.create_file((char *)"test.c", 5);
  ASSERT_EQ(1, code) << "Failure in read test ReadWriteHarmony_BasicRead\n"
  <<"create_file on test.c returned "<<code<<", but should have returned 1\n"
  <<"If you are passing CreateFile_BasicCreate, this shouldn't happen";
  
  char buf[1024];
  for(int i=0;i<5;i++)
  {
    for(int j=0;j<1024;j++){buf[j]=i+1;}
    code = f.write((char *)"test.c", i, buf);
    ASSERT_EQ(1, code) << "Failure in read test ReadWriteHarmony_BasicRead\n"
    <<"write to test.c block "<<i<<" returned "<<code<<", but should have returned 1\n"
    <<"If you are passing Write_BasicWrite, this shouldn't happen";
  }
  
  bool written;
  int nwritten=0;
  for(int i=0;i<5;i++)
  {
    code = f.read((char *)"test.c", i, buf);
    ASSERT_EQ(1, code) << "Failure in read test ReadWriteHarmony_BasicRead\n"
    <<"read from test.c block "<<i<<" returned "<<code<<", but should have returned 1";
    
    written=true;
    for(int j=0;j<1024;j++){written=(written?(buf[j]==i+1):written);}
    nwritten+=(written?1:0);
  }
  ASSERT_EQ(5, nwritten) << "Failure in read test ReadWriteHarmony_BasicRead\n"
  <<nwritten<<" blocks were properly read from, all 5 blocks should have been\n"
  <<"Perhaps you aren't readind the disk to the buffer, or did so incorrectly";
  f.close_disk();
}

TEST_F(FSTest, ReadWriteHarmony_LongName)
{
  myFileSystem f((char *)"disk0");
  int code;
  code = f.create_file((char *)"test.txt", 1);
  ASSERT_EQ(1, code) << "Failure in read test ReadWriteHarmony_LongName\n"
  <<"create_file on test.txt returned "<<code<<", but should have returned 1\n"
  <<"If you are passing Create_BasicCreate, this shouldn't happen";
  
  char buf[1024];
  for(int i=0;i<1024;i++){buf[i]=1;}
  code = f.write((char *)"test.txt", 0, buf);
  ASSERT_EQ(1, code) << "Failure in read test ReadWriteHarmony_LongName\n"
  <<"write to test.txt block 0 returned "<<code<<", but should have returned 1\n"
  <<"Perhaps you have an issue comparing names of length 8\n"
  <<"When you read in an inode who's name has length 8, it lacks a terminating null character\n"
  <<"When the function's input name has length 8, it still actually has name[8]='\0'\n"
  <<"The inode's name[8] is probaly the first byte of the inode's size, not '\0'\n"
  <<"Although these two names are equal on the first 8 bytes, strcmp sees them as different on the ninth\n"
  <<"You DON'T want to change the inode's name[8] (since that is part of size)\n"
  <<"The easiest fix is probably to just use strncmp()";
  
  for(int i=0;i<1024;i++){buf[i]=0;}
  code = f.read((char *)"test.txt", 0, buf);
  ASSERT_EQ(1, code) << "Failure in read test ReadWriteHarmony_LongName\n"
  <<"read from test.txt block 0 returned "<<code<<", but should have returned 1\n"
  <<"Perhaps you have an issue comparing names of length 8\n"
  <<"When you read in an inode who's name has length 8, it lacks a terminating null character\n"
  <<"When the function's input name has length 8, it still actually has name[8]='\0'\n"
  <<"The inode's name[8] is probaly the first byte of the inode's size, not '\0'\n"
  <<"Although these two names are equal on the first 8 bytes, strcmp sees them as different on the ninth\n"
  <<"You DON'T want to change the inode's name[8] (since that is part of size)\n"
  <<"The easiest fix is probably to just use strncmp()";
  f.close_disk();
}

// test create_file return code 1 for success
TEST_F(FSTest, create_file_test)
{
  myFileSystem f((char *)"disk0");
  int code = f.create_file((char *)"test.c", 1);
  ASSERT_EQ(1, code);
  f.close_disk();
}

// test delete_file return code -1 for failure
TEST_F(FSTest, delete_file_test) {
  myFileSystem f((char *)"disk0");
  int code = f.delete_file((char *)"test.c");
  ASSERT_EQ(-1, code);
  f.close_disk();
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
