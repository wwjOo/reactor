#include "channel.h"


Channel::Channel(int fd, EventLoop *loop) 
{
    fd_ = fd;
    loop_ = loop;

    if(loop == nullptr)
    {
        std::cerr << "loop == nullptr" << std::endl;
        exit(-1);
    }
}

Channel::~Channel()
{
    //ep和fd不属于channel，不要进行关闭，ep属于Epoll类，fd属于Socket类 
}

int Channel::fd() //返回fd
{
    return fd_;
}

void Channel::setET() //设置为边缘触发
{
    events_ |= EPOLLET;
    loop_->updatechannel(this);
}

void Channel::enableread() //使能读事件
{
    events_ |= EPOLLIN;
    loop_->updatechannel(this);
}

void Channel::disableread() //关闭读事件
{
    events_ &= ~EPOLLIN;
    loop_->updatechannel(this);
}

void Channel::enablewrite() //使能写事件
{
    events_ |= EPOLLOUT;
    loop_->updatechannel(this);
}

void Channel::disablewrite() //关闭写事件
{
    events_ &= ~EPOLLOUT;
    loop_->updatechannel(this);
}

void Channel::disableall() //关闭所有事件
{
    events_ = 0;
    loop_->updatechannel(this);
}

bool Channel::isinepoll() //获取inepoll值
{
    return inepoll_;
}

void Channel::setinepoll() //将inepoll设为true
{
    inepoll_ = true;
}

void Channel::unsetinepoll() //将inepoll设为false
{
    inepoll_ = false;
}

void Channel::removefromepoll() //从epoll中删除
{
    disableall();
    loop_->removechannel(this);
}

void Channel::setrevents(uint32_t revs) //设置revents_
{
    revents_ = revs;
}

uint32_t Channel::events() //返回events_
{
    return events_;
}

uint32_t Channel::revents() //返回revents_
{
    return revents_;
}

void Channel::registReadcb(std::function<void()> fn) //注册回调函数
{
    readcb_ = fn;
}

void Channel::registWritecb(std::function<void()> fn) //注册回调函数
{
    writecb_ = fn;
}

void Channel::registClosecb(std::function<void()> fn) //注册回调函数
{
    closecb_ = fn;
}

void Channel::registErrorcb(std::function<void()> fn) //注册回调函数
{
    errorcb_ = fn;
}

void Channel::handleEvent() //处理事件
{
    if(revents_ & EPOLLHUP)
    {
        closecb_();
    }
    else if(revents_ & (EPOLLIN|EPOLLPRI))
    {
        readcb_();
    }
    else if(revents_ & EPOLLOUT)
    {
        writecb_();
    }
    else
    {
        errorcb_();
    }
}