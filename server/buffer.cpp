#include "buffer.h"



Buffer::Buffer(/* args */)
{

}

Buffer::~Buffer()
{

}

void Buffer::append(const char *buf, int size)
{
    buffer_.append(buf, size);
}

#include <iostream>
void Buffer::appendwithmode(const char *buf, int size)  //附加报文头部信息
{
    if(mode_ == 1)
    {
        buffer_.append((char *)&size, 4); //头部信息：报文长度
    }
    
    buffer_.append(buf, size); //报文内容
}

size_t Buffer::size()
{
    return buffer_.size();
}

const char *Buffer::data()
{
    return buffer_.data();
}

void Buffer::clear()
{
    buffer_.clear();
}

void Buffer::erase(int pos, int nn)
{
    buffer_.erase(pos,nn);
}

bool Buffer::pickmsg(std::string &msg) //拆出数据包，成功返回true,失败返回false
{
    if(mode_ == 1)
    {
        int len;
        memcpy(&len, buffer_.data(), sizeof(len));
        if(buffer_.size() < sizeof(len) + len) return false;

        msg = std::string(buffer_.begin() + sizeof(len), buffer_.begin() + sizeof(len) + len);
        buffer_.erase(0, sizeof(len) + len);
    }
    else
    {
        if(buffer_.size() == 0) return false;

        msg = buffer_;
        buffer_.clear();
    }

    return true;
}