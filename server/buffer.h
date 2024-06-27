#pragma once

#include <string>
#include <cstring>

class Buffer
{
private:
    std::string buffer_;
    int mode_ = 1;              //mode: 0不添加任何附加信息， 1添加长度信息到头部
public:
    Buffer(/* args */);
    ~Buffer();

    void append(const char *buf, int size);
    void appendwithmode(const char *buf, int size);
    size_t size();
    const char *data();
    void clear();
    void erase(int pos, int nn);
    bool pickmsg(std::string &msg); //拆出数据包，成功返回true,失败返回false
};

