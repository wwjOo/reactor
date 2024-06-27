#pragma once
#include <netinet/tcp.h> // 内含TCP_NODELAY定义
#include <functional>

#include "tcpserver.h"
#include "eventloop.h"

class EventLoop;

class Acceptor
{
private:
    EventLoop *loop_;   //事件循环，不属于本类，由外部传入
    Socket servsock_;   //监听socket，属于本类，需要进行析构释放
    Channel servch_;    //监听channel，属于本类，需要进行析构释放
    
    std::function<void(std::unique_ptr<Socket>)> tcpnewconncb_;  //回调TcpServer的NewConnection函数

public:
    Acceptor(EventLoop *loop, const std::string &ip, const u_int16_t port); 
    ~Acceptor();

    void newconnection();   //新的连接
    void registTcpnewconncb(std::function<void(std::unique_ptr<Socket>)> fn);   //注册回调
};
