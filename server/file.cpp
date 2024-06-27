#include "file.h"

ostream& operator<<(ostream &out, const File &f)
{
    out << "[file info]size:" << f.size() << " inode:" << f.inode();
    return out;
}

File::File(std::string path):path_(path)
{
    fin_.open(path);
    if(fin_.is_open() == false)
    {
        perror(std::string("open:" + path).data());
        exit(-1); 
    }

    stat(path.c_str(), &info_);
    cout << *this << endl; 
}

File::~File()
{
    fin_.close();
}

std::string File::path() const   //获取文件路径+文件名
{
    return path_;
}

__off_t File::size() const //获取文件大小
{
    return info_.st_size;
}

__ino_t File::inode() const //获取inode
{
    return info_.st_ino;
}

std::istream &File::read(char *s, std::streamsize n) //读取文件内容
{
    return fin_.read(s,n);
}