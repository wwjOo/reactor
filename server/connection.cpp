#include "connection.h"

//unique_ptr的赋值需要使用移动语义，注意智能指针的初始化
Connection::Connection(EventLoop *loop, std::unique_ptr<Socket> clientsock)
                        :loop_(loop), clientsock_(std::move(clientsock)),
                        disconnect_(false),clientch_(new Channel(clientsock_->fd(),loop_)),
                        timeoutthr_(loop_->getTimeoutThr())
{
    clientch_->registReadcb(std::bind(&Connection::readcb, this));
    clientch_->registWritecb(std::bind(&Connection::writecb,this));
    clientch_->registClosecb(std::bind(&Connection::closecb,this));
    clientch_->registErrorcb(std::bind(&Connection::errorcb,this));

    clientch_->enableread();
    clientch_->setET();
}

Connection::~Connection()
{   
    
}

int Connection::fd() const
{
    return clientsock_->fd();
}

std::string Connection::ip() const
{
    return clientsock_->ip();
}

uint16_t Connection::port() const
{
    return clientsock_->port();
}

Timestamp Connection::tmstamp() const
{
    return tmstamp_;
}

void Connection::readcb() //处理数据事件
{
    //读取数据处理
    while(true)
    {
        char rxbuf[1024];
        bzero(rxbuf,1024);
        int readn = recv(fd(), rxbuf, 1024, 0);
        if(readn > 0) // 正常接收
        {
            inputbuffer_.append(rxbuf, readn);
        }
        else if(readn == 0) //连接关闭
        {
            closecb();
            break;
        }
        else if(readn == -1) //异常
        {
            if(errno == EAGAIN) //数据已读完
            {
                while(inputbuffer_.size())
                {
                    std::string msg;
                    if(inputbuffer_.pickmsg(msg) == false) break;

                    /*=======*/
                    messagecb_(shared_from_this(), msg); //消息处理
                    tmstamp_ = Timestamp::now();
                    /*=======*/
                }
                break;
            }
            else
            {
                errorcb();
                break;
            }
        }
    }
}

void Connection::writecb() //写事件回调
{
    int sendn = ::send(fd(), outputbuffer_.data(), outputbuffer_.size(), 0);
    if(sendn > 0)
    {
        outputbuffer_.erase(0, sendn);
    }
    
    //如果发送buffer没有数据了，则不再关注写事件
    if(outputbuffer_.size() == 0)
    {
        clientch_->disablewrite(); 
        sendfinishcb_(shared_from_this());
    }
}

void Connection::closecb()
{
    disconnect_ = true;
    clientch_->removefromepoll();
    closecb_(shared_from_this());
    
    loop_->wakeup();
}

void Connection::errorcb()
{
    disconnect_ = true;
    clientch_->removefromepoll();
    errorcb_(shared_from_this());
    
    loop_->wakeup();
}

void Connection::registclosecb(std::function<void(spConnection)> fn)
{
    closecb_ = fn;
}

void Connection::registerrorcb(std::function<void(spConnection)> fn)
{
    errorcb_ = fn;
}

void Connection::registmessagecb(std::function<void(spConnection, std::string&)> fn)
{
    messagecb_ = fn;
}

void Connection::registsendfinishcb(std::function<void(spConnection)> fn)
{
    sendfinishcb_ = fn;
}

void Connection::send(const char *str, size_t size) //发送message函数
{
    //连接已经断开，则直接返回
    if(disconnect_) return;
    
    //这里该函数结束后str将被释放内存，若想加入任务队列，必须使用智能指针
    std::shared_ptr<std::string> msg(new std::string(str));
    
    //如果在IO线程中执行，则此时访问outputbuffer_是线程安全的
    //如果在工作线程中执行，则此时访问不是线程安全的
    //为了不使用互斥锁，将其加入到IO线程的唤醒队列中处理发送
    if(loop_->isinloopthread() == true)
    {
        onsend(msg,size);
    }
    else
    {
        //加入到IO线程的唤醒队列，并唤醒
        loop_->addtask(std::bind(&Connection::onsend, this, msg, size));
    }
}

void Connection::send(const std::string &str)
{
    //连接已经断开，则直接返回
    if(disconnect_) return;
    
    //这里该函数结束后str将被释放内存，若想加入任务队列，必须使用智能指针
    std::shared_ptr<std::string> msg(new std::string(str));
    
    //如果在IO线程中执行，则此时访问outputbuffer_是线程安全的
    //如果在工作线程中执行，则此时访问不是线程安全的
    //为了不使用互斥锁，将其加入到IO线程的唤醒队列中处理发送
    if(loop_->isinloopthread() == true)
    {
        onsend(msg,str.size());
    }
    else
    {
        //加入到IO线程的唤醒队列，并唤醒
        loop_->addtask(std::bind(&Connection::onsend, this, msg, str.size()));
    }
}

void Connection::onsend(std::shared_ptr<std::string> str, size_t size)
{
    outputbuffer_.appendwithmode(str.get()->c_str(),size); 
    clientch_->enablewrite(); //打开当前sock的写事件，当触发写事件后，进行发送，发送完后，再关闭写事件
}

bool Connection::istimeout(time_t now) //判断是否超时
{
    if(timeoutthr_ == -1) return false;

    return (now - tmstamp_.toint()) > timeoutthr_;
}

bool Connection::isconnclose() //连接是否已经关闭
{
    return disconnect_;
}