#include "tcpserver.h"



//参数：ip，port，epoll超时时间，工作线程数量，连接检测定时器周期
TcpServer::TcpServer(const std::string &ip, const u_int16_t port, int epolltimeout_ms, 
                    int epollthreadnum, int conntimercycle_s, int conntimeout_s)
                    :threadnum_(epollthreadnum) \
                    ,mainloop_(new EventLoop(true, conntimercycle_s, conntimeout_s)) \
                    ,acceptor_(new Acceptor(mainloop_.get(), ip, port)) \
                    ,threadpool_(new ThreadPool(threadnum_,"IO"))
{
    mainloop_->registtimeoutcb(std::bind(&TcpServer::epolltimeoutcb, this, std::placeholders::_1));
    mainloop_->registremoveconncb(std::bind(&TcpServer::closecb,this,std::placeholders::_1));

    acceptor_->registTcpnewconncb(std::bind(&TcpServer::newconnection, this, std::placeholders::_1));  

    for(int i=0; i<threadnum_; i++)
    {
        subloops_.emplace_back(new EventLoop(false, conntimercycle_s, conntimeout_s));
        subloops_[i]->registtimeoutcb(std::bind(&TcpServer::epolltimeoutcb, this, std::placeholders::_1));
        subloops_[i]->registremoveconncb(std::bind(&TcpServer::closecb,this,std::placeholders::_1));
        threadpool_->addtask(std::bind(&EventLoop::run,subloops_[i].get(),epolltimeout_ms)); //bind需要传入普通指针
    }
}

TcpServer::~TcpServer()
{
    
}

void TcpServer::start(int timeout_ms)   //启动事件循环
{
    mainloop_->run(timeout_ms);
}

void TcpServer::stop() //停止所有事件循环
{
    mainloop_->stop();

    for(int i=0; i<subloops_.size(); i++)
    {
        subloops_[i]->stop();
    }
}

//这样做的目的是为了让connection对象属于TcpServer类
void TcpServer::newconnection(std::unique_ptr<Socket> clientsock)   //新的连接
{   
    int belongidx = clientsock->fd()%threadnum_;

    spConnection conn(new Connection(subloops_[belongidx].get(), std::move(clientsock)));  
    conn->registclosecb(std::bind(&TcpServer::closecb,this,std::placeholders::_1));
    conn->registerrorcb(std::bind(&TcpServer::errorcb,this,std::placeholders::_1));
    conn->registmessagecb(std::bind(&TcpServer::messagecb,this,std::placeholders::_1, std::placeholders::_2));
    conn->registsendfinishcb(std::bind(&TcpServer::sendfinishcb,this,std::placeholders::_1));

    {
        std::unique_lock<std::mutex> l(connmtx_);
        conns_[conn->fd()] = conn; //加入连接集合
    }    

    if(subloops_[belongidx]->getTimeoutThr() != -1)
        subloops_[belongidx]->addconn(conn);

    connectcb_(conn);
}

void TcpServer::closecb(spConnection conn) //关闭事件回调
{
    {
        std::unique_lock<std::mutex> l(connmtx_);
        if(conns_.find(conn->fd()) == conns_.end()) return;
    }

    if(closecb_) closecb_(conn); //在delete前调用

    {
        std::unique_lock<std::mutex> l(connmtx_);
        conns_.erase(conn->fd());
    }
}

void TcpServer::errorcb(spConnection conn) //错误事件回调
{
    if(errorcb_) errorcb_(conn); //在delete前调用

    {
        std::unique_lock<std::mutex> l(connmtx_);
        conns_.erase(conn->fd());
    }
}

void TcpServer::messagecb(spConnection conn, std::string &msg) //消息包处理
{ 
    if(messagecb_) messagecb_(conn,msg);
}

void TcpServer::sendfinishcb(spConnection conn) //数据发送完成回调
{
    if(sendfinishcb_) sendfinishcb_(conn);
}

void TcpServer::epolltimeoutcb(EventLoop *loop) //事件循环超时回调
{
    if(epolltimeoutcb_) epolltimeoutcb_(loop);
}


void TcpServer::registnewconncb(std::function<void(spConnection conn)>fn) //连接事件回调注册
{
    connectcb_ = fn;
}
void TcpServer::registclosecb(std::function<void(spConnection conn)> fn) //关闭事件回调
{
    closecb_ = fn;
}
void TcpServer::registerrorcb(std::function<void(spConnection conn)> fn) //错误事件回调
{
    errorcb_ = fn;
}
void TcpServer::registmessagecb(std::function<void(spConnection conn, std::string &msg)> fn) //消息包处理回调
{
    messagecb_ = fn;
}
void TcpServer::registsendfinishcb(std::function<void(spConnection conn)> fn) //数据发送完成回调
{
    sendfinishcb_ = fn;
}
void TcpServer::registepolltimeoutcb(std::function<void(EventLoop *loop)> fn) //事件循环超时回调
{
    epolltimeoutcb_ = fn;
}