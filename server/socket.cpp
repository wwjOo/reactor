#include "socket.h"


int setnonblock(int fd) //将fd设置为非阻塞IO
{
    return fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0)|SOCK_NONBLOCK);
}

int CreatNonblockSocket() //创建非阻塞socket
{
    int fd = socket(PF_INET, SOCK_STREAM|SOCK_NONBLOCK, IPPROTO_TCP);
    if(fd == -1)
    {
        perror("Socket failed");
        // printf("%s:%d:%s Socket failed:%d",__FILE__,__LINE__,__FUNCTION__, errno);
        exit(-1);
    }
    return fd;
}

Socket::Socket(const int fd):fd_(fd)
{
    // std::cout << "[TxBufSize:" << getsystxbufsize() 
    //           << " RxBufSize:" << getsysrxbufsize() << "]" << std::endl; 
}

Socket::~Socket()
{
    ::close(fd_);
}

int Socket::fd() const
{
    return fd_;
}

std::string Socket::ip() //获取ip
{
    return ip_;
}

uint16_t Socket::port() //获取port
{
    return port_;
}

void Socket::setip(const std::string ip) //设置ip
{
    ip_ = ip;
}
void Socket::setport(uint16_t port) //设置port
{
    port_ = port;
}

void Socket::setopt(int optname, bool ison) //设置socket选项，一般设置SO_REUSEADDR,SO_REUSEPORT,TCP_NODELAY,SO_KEEPALIVE四个选项，true为打开，false为关闭
{
    int optval = ison ? 1 : 0;
    setsockopt(fd_,SOL_SOCKET,optname, &optval,static_cast<socklen_t>(sizeof(optval)));
}

int Socket::getsystxbufsize()  //获取系统为socket分配的txbuf大小
{
    int bufsize = 0;
    socklen_t len = sizeof(bufsize);
    getsockopt(fd_, SOL_SOCKET, SO_SNDBUF, &bufsize, &len);
    return bufsize;
}

int Socket::getsysrxbufsize()  //获取系统为socket分配的txbuf大小
{
    int bufsize = 0;
    socklen_t len = sizeof(bufsize);
    getsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &bufsize, &len);
    return bufsize;
}

void Socket::bind(const inetaddress &servaddr) //服务端调用该函数
{
    if(::bind(fd_, servaddr.addr(), sizeof(sockaddr)) == -1)
    {
        perror("bind failed");
        close(fd_);
        exit(-1);
    }

    ip_ = servaddr.ip();
    port_ = servaddr.port();
}

void Socket::listen(int n) //服务端调用该函数,生成半连接队列和accept队列
{
    if(::listen(fd_, n) == -1)
    {
        perror("listen failed");
        close(fd_);
        exit(-1);
    }
}

int Socket::accept(inetaddress &clientaddr, bool isnonblock) //服务端调用该函数，从accept队列中取出一个连接
{
    int clientfd;
    struct sockaddr_in recvaddr; 
    socklen_t len = sizeof(recvaddr);

    if(isnonblock)
        clientfd = accept4(fd_, (sockaddr *)&recvaddr, &len, SOCK_NONBLOCK);
    else
        clientfd = ::accept(fd_, (sockaddr *)&recvaddr, &len);
        
    if(clientfd == -1)
    {
        perror("accept failed");
        close(fd_);
        exit(-1);
    }
    
    clientaddr.setaddr(recvaddr);
    ip_ = clientaddr.ip();
    port_ = clientaddr.port();

    return clientfd;
}