#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

#include "inetaddress.h"


int setnonblock(int fd);
int CreatNonblockSocket();

class Socket
{
private:
    const int fd_;                      //socket持有的fd，在构造时传进来
    std::string ip_;                    //ip
    uint16_t port_;                     //port
public:
    Socket(const int fd);               //传入准备好的fd
    ~Socket();

    int fd() const;                         //获取fd
    std::string ip();                       //获取ip
    uint16_t port();                        //获取port
    void setip(const std::string ip);       //设置ip
    void setport(uint16_t port);            //设置port

    void setopt(int optname, bool ison);    //设置socket选项，一般设置SO_REUSEADDR,SO_REUSEPORT,TCP_NODELAY,SO_KEEPALIVE四个选项，true为打开，false为关闭
    int getsystxbufsize();  //获取系统为socket分配的txbuf大小
    int getsysrxbufsize();  //获取系统为socket分配的txbuf大小

    void bind(const inetaddress &servaddr);                      //服务端调用该函数
    void listen(int n = 128);                                    //服务端调用该函数
    int accept(inetaddress &clientaddr, bool isnonblock = true); //服务端调用该函数
};




