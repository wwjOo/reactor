#pragma once
#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
using namespace std;

class File
{
private:
    std::string path_;  //文件路径
    ifstream fin_;      //文件流，用于读取文件
    struct stat info_;  //文件信息 
public:
    File(std::string path);
    ~File();

    std::string path() const;   //获取文件路径+文件名
    __off_t size() const;       //获取文件大小
    __ino_t inode() const;      //获取inode
    std::istream & read(char *s, std::streamsize n); //读取文件内容
};

ostream& operator<<(ostream &out, const File &f); 