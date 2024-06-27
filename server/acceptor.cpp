#include "acceptor.h"


Acceptor::Acceptor(EventLoop *loop, const std::string &ip, const u_int16_t port) 
                :loop_(loop), servsock_(CreatNonblockSocket()), servch_(servsock_.fd(), loop_)
{
    inetaddress servaddr(ip, port);
    servsock_.setopt(SO_REUSEADDR,true);
    servsock_.setopt(SO_REUSEPORT,true);
    servsock_.setopt(TCP_NODELAY,true);     //关闭Negle算法，允许发送小包数据
    servsock_.setopt(TCP_QUICKACK,true);    //关闭延迟ACK
    servsock_.setopt(SO_KEEPALIVE,true);    //开启TCP保活机制
    servsock_.bind(servaddr);
    servsock_.listen();
    std::cout << "开始监听: [fd:" << servsock_.fd() << " ip:" << servaddr.ip() << " port:" << servaddr.port() << "]" << std::endl;

    servch_.registReadcb(std::bind(&Acceptor::newconnection, this)); //注册连接事件
    servch_.enableread();
}

Acceptor::~Acceptor()
{
    
}

void Acceptor::newconnection() //新的连接
{
    inetaddress clientaddr;
    std::unique_ptr<Socket> clientsock(new Socket(servsock_.accept(clientaddr)));
    clientsock->setip(clientaddr.ip());
    clientsock->setport(clientaddr.port());

    tcpnewconncb_(std::move(clientsock));
}

void Acceptor::registTcpnewconncb(std::function<void(std::unique_ptr<Socket>)> fn) //注册回调
{
    tcpnewconncb_ = fn;
}