#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> //内含sockaddr_in定义
#include <netdb.h>

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <thread>
#include <poll.h>
#include <vector>
#include <sys/epoll.h>

#include "file.h"
#include "buffer.h"
using namespace std;


class myclient
{
public:
    int fd_;               //-1表示未连接或已断开 >=0表示有效
    string ip_;            //服务端ip
    unsigned short port_;  //服务端port
    Buffer txbuf_;
    Buffer rxbuf_;
    int epfd_;

    myclient():fd_(-1)
    {

    };
    ~myclient()
    {
        close();
        ::close(epfd_);
    }

    void epollcreat()
    {
        epfd_ = epoll_create(1);
        epoll_event ev;
        ev.data.fd = fd_;
        ev.events = EPOLLIN;
        epoll_ctl(epfd_, EPOLL_CTL_ADD, fd_, &ev);
    }

    void epollsendon()
    {
        epoll_event ev;
        ev.data.fd = fd_;
        ev.events = EPOLLIN|EPOLLOUT;
        epoll_ctl(epfd_,EPOLL_CTL_MOD, fd_, &ev);
    }

    void epollsendoff()
    {
        epoll_event ev;
        ev.data.fd = fd_;
        ev.events = EPOLLIN;
        epoll_ctl(epfd_,EPOLL_CTL_MOD, fd_, &ev);
    }

    bool connect(const string &ip, unsigned short port)
    {
        if(fd_ != -1) 
        {
            cout << "[info] socket has been created" << endl; 
            return true;
        }

        this->ip_ = ip;
        this->port_ = port;

        //创建socket
        if((fd_ = socket(PF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0)) == -1) 
            return false; // errno will be set

        //设置服务器端ip+port
        struct sockaddr_in caddr;
        memset(&caddr,0,sizeof(caddr));
        caddr.sin_family = PF_INET;
        caddr.sin_port = htons(port);
        struct hostent *h = gethostbyname(ip_.c_str()); //通过该函数可传域名，主机名，字符串IP，如果用inet_addr，则只能接受字符串IP
        if(h == nullptr) // errno will be set
        {
            return false;
        }
        memcpy(&caddr.sin_addr, h->h_addr, h->h_length);

        //建立与服务端的连接
        //通过wireshark抓包时发现：当连接数大于服务端设置连队列大小时，后续的客户端请求将被拒绝，客户端将停留在SYN_SENT状态并触发重传
        cout << "开始连接:[port:" << port_ << " ip:" << ip_ << " fd:" << fd_ << "]" <<endl;
        //第二次握手成功后返回fd
        while(::connect(fd_, (struct sockaddr *)&caddr, sizeof(caddr)) == -1)
        {
            if((errno != EINPROGRESS) && (errno != EALREADY))
            {
                cout << "failed:" << errno << endl;
                return false; // errno will be set
            }
            else
            {

            }
        }

        epollcreat();

        return true;
    }

    int getsystxbufsize()  //获取系统为socket分配的txbuf大小
    {
        int bufsize = 0;
        socklen_t len = sizeof(bufsize);
        getsockopt(fd_, SOL_SOCKET, SO_SNDBUF, &bufsize, &len);
        return bufsize;
    }

    int getsysrxbufsize()  //获取系统为socket分配的txbuf大小
    {
        int bufsize = 0;
        socklen_t len = sizeof(bufsize);
        getsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &bufsize, &len);
        return bufsize;
    }
    
    void close()
    {
        if(fd_ == -1) return;

        // ::close(fd_);
        shutdown(fd_,SHUT_RD);
        fd_ = -1;
    }
};


#define PERSIZE 2048
myclient client;
File file("111.zip");

struct st_info{
    int persize;
    off_t totalsize; 
    char name[128];
};


int bagnum = 0;
void onsend(const File &f)
{
    if(bagnum == 0)
    {
        st_info info = {PERSIZE, f.size(), {0}};
        strcpy(info.name,f.path().data()); 

        std::string buf;
        buf.append((char*)&bagnum,sizeof(int));
        buf.append((char*)&info,sizeof(info));
        client.txbuf_.appendwithmode(buf.data(), buf.size());
    }
    else
    {
        static int restlen = file.size();

        char buf[PERSIZE];
        int curlen = restlen < (PERSIZE-4) ? restlen : (PERSIZE-4);
        if(curlen == 0) 
        {
            cout << "finish" << endl;
            return;
        }
        memcpy(buf,&bagnum,sizeof(int));
        
        file.read(buf+4, curlen);
        restlen -= curlen;
        
        client.txbuf_.appendwithmode(buf,curlen+4);
    }

    client.epollsendon();
}

//处理接收报文
void onmessage(std::string &msg)
{
    int head;
    memcpy(&head,msg.data(),4);
    msg.erase(0,4);

    if(head == bagnum+1 && msg == "ok") //接收消息体
    {
        bagnum = head;
        onsend(file);
        // cout << "bagnum ok:" << bagnum << endl;
    }
    else if(head == -1)
    {
        cout << "接收方未准备好:" << msg << endl;
    }
    else
    {
        cout << "[recv error]expect:" << bagnum+1 << " recved:" << head << endl;
    }
}

int main(int argc, char const *argv[])
{
    if(argc != 3)
    { 
        cout << "Usage: serveip + port" << endl;
        return -1;
    }

    //连接
    if(client.connect(argv[1],atoi(argv[2])) == false)
    {
        perror("connect"); 
        return -1;
    }
    cout << "连接成功 txbuf:" << client.getsystxbufsize() << " rxbuf:" << client.getsysrxbufsize() << endl; 

    //=======================================
    //发送头部信息
    onsend(file);

    //事件循环，传输文件
    epoll_event ev;
    while(true)
    {
        int num = epoll_wait(client.epfd_,&ev,1,1000);
        if(num < 0) {perror("epoll_wait"); exit(-1);}
        if(num == 0) continue;

        if(ev.events & EPOLLIN) //可读
        {
            while(true)
            {
                char buf[PERSIZE];
                bzero(buf,sizeof(buf));

                int readn = recv(client.fd_, buf, sizeof(buf), 0);
                if(readn > 0) client.rxbuf_.append(buf, readn);
                else if(readn == 0) break; //断开连接
                else if(readn == -1)
                {
                    if(errno == EAGAIN) //读完
                    {
                        while(client.rxbuf_.size() > 0)
                        {
                            std::string msg;
                            if(client.rxbuf_.pickmsg(msg) == false) break;
                            
                            /*=======*/
                            //TODO:解析数据msg
                            onmessage(msg);
                            /*=======*/
                        }
                        break;
                    }
                    else break; //错误
                }
            }
        }
        else if(ev.events & EPOLLOUT) //可写
        {
            //一直写，直到写完或缓冲区满
            while(client.txbuf_.size())
            {
                int ret = write(client.fd_, client.txbuf_.data(), client.txbuf_.size());
                if(ret < 0) 
                {
                    if(errno == EAGAIN) break; //缓冲区满，不可写
                    else {perror("write"); exit(-1);}
                }
                
                //为下次做准备
                client.txbuf_.erase(0,ret);

                //写完后关闭写事件
                if(client.txbuf_.size() == 0)
                {
                    client.epollsendoff();
                }
            }
        }
    }

    cout << "finish" << endl; 

    // for(int i=1; i<=1; i++)
    // {
    //     string buf = to_string(i) + ": 四次挥手以两次收发FIN-ACK";
    //     char txbuf[1024];

    //     int len = buf.size();
    //     memcpy(txbuf, (char *)&len, sizeof(len));
    //     memcpy(txbuf+sizeof(len), buf.data(), len);
    //     if(send(client.fd_, txbuf, len + sizeof(len), 0) <= 0)
    //     {
    //         cerr << "send err" << endl; return -1;
    //     }
    // }

    return 0;
} 


