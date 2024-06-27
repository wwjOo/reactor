#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

class inetaddress
{
private:
    sockaddr_in addr_;
public:
    inetaddress(){}
    inetaddress(const std::string &ip, const unsigned short port);  //用于监听的服务端地址
    inetaddress(const sockaddr_in recvaddr);                        //用于连接的客户端地址
    ~inetaddress();

    const char *ip() const;                     //获取字符串形式ip地址
    unsigned short port() const;                //获取端口号
    const sockaddr *addr() const;               //获取sockaddr形式的地址信息
    void setaddr(const sockaddr_in &clientaddr);//设置addr_成员
};

