#include "echoserver.h"



EchoServer::EchoServer(const std::string &ip, const u_int16_t port, int epolltimeout_ms, int subthreadnum, 
                        int workthreadnum, int conntimercycle_s, int conntimeout_s)
                        :server_(new TcpServer(ip, port, epolltimeout_ms, subthreadnum, conntimercycle_s,conntimeout_s)) \
                        ,threadpool_(new ThreadPool(workthreadnum, "WORK"))
{
    //注册接口函数(不关心的可以不注册)
    server_->registnewconncb(std::bind(&EchoServer::HandleNewConnect,this,std::placeholders::_1));
    server_->registclosecb(std::bind(&EchoServer::HandleClose,this,std::placeholders::_1));
    server_->registerrorcb(std::bind(&EchoServer::HandleError,this,std::placeholders::_1));
    server_->registmessagecb(std::bind(&EchoServer::HandleMessage,this,std::placeholders::_1, std::placeholders::_2));
    server_->registsendfinishcb(std::bind(&EchoServer::HandleSendComplete,this,std::placeholders::_1));
    server_->registepolltimeoutcb(std::bind(&EchoServer::HandleEpollTimeout,this,std::placeholders::_1));
}

EchoServer::~EchoServer()
{
    stop();
}

void EchoServer::HandleNewConnect(spConnection conn)
{
    std::cout << "[" << syscall(SYS_gettid) << "]"<< Timestamp::now().tostring() << "[" << conn->fd() << "]" << "连接成功["<< "ip:" << conn->ip() << " port:" << conn->port() << "]" << std::endl;
}

void EchoServer::HandleClose(spConnection conn) //关闭事件回调
{
    std::cout<< "[" << syscall(SYS_gettid) << "]"<< Timestamp::now().tostring() << "[" << conn->fd() << "]"<< "连接断开["<< "ip:" << conn->ip() << " port:" << conn->port() << "]" << std::endl;
}

void EchoServer::HandleError(spConnection conn) //错误事件回调
{
    std::cout << "[" << syscall(SYS_gettid) << "]"<< Timestamp::now().tostring() << "[" << conn->fd() << "]"<< "连接错误["<< "ip:" << conn->ip() << " port:" << conn->port() << "]" << std::endl;
}

void EchoServer::HandleMessage(spConnection conn, std::string &msg) //消息包处理
{     
    if(threadpool_->size() > 0) //在工作线程中处理
    {
        threadpool_->addtask(std::bind(&EchoServer::onmessage, this, conn, msg));
    }
    else //直接在IO线程中处理
    {
        onmessage(conn,msg);
    }
}



#include <fstream>
std::ofstream fout;

#define PATH "/home/ubuntu/reactor/recv/"

struct st_info{ 
    int persize;
    off_t totalsize;
    char name[128];
}info;

void EchoServer::onmessage(spConnection conn, std::string &msg)
{
    int head;
    memcpy(&head,msg.data(),4);
    msg.erase(0,4);

    if(head == 0) //头部处理
    {
        memcpy(&info,msg.data(),msg.size());
        cout << info.persize << endl;
        cout << info.totalsize << endl;
        cout << info.name << endl;

        fout.open(std::string(PATH)+info.name);

        std::string buf;
        if(!fout.is_open())
        {
            head = -1;
            buf.append((char*)&head, 4);
            buf += "ostream open failed";
            cout << std::string(PATH)+info.name + "ostream open failed" << endl;
        }
        else
        {
            head += 1;
            buf.append((char*)&head, 4);
            buf += "ok";
            cout << "ready" << endl;
        }
        
        conn->send(buf);
    }
    else//文件体处理
    {
        fout.write(msg.c_str(),msg.size());

        info.totalsize -= msg.size();

        //反馈信息
        std::string buf;
        head += 1;
        buf.append((char*)&head, 4);
        buf += "ok";
        conn->send(buf);
        cout << info.totalsize << endl;

        //TODO:中途关闭程序前需要先关闭文件流
    }

    if(info.totalsize == 0) 
    {
        cout << "recv finish" << endl;
        fout.close();
    }
}





void EchoServer::HandleSendComplete(spConnection conn) //数据发送完成回调
{
    // std::cout << "[" << syscall(SYS_gettid) << "]" << "[" << conn->fd() << "]"<< "send completed" << std::endl;
}

void EchoServer::HandleEpollTimeout(EventLoop *loop) //事件循环超时回调
{
    std::cout << "epoll_wait timeout [epfd:" << loop->epfd() << "]" << std::endl;
}

void EchoServer::run(int timeout_ms)
{
    server_->start(timeout_ms);
}

void EchoServer::stop()
{
    server_->stop();
}

