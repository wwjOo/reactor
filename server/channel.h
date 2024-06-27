#pragma once
//该类对socket和epoll进行关联封装,以便不同的channel(socket)执行操作

#include <iostream>
#include <functional>

#include "inetaddress.h"
#include "socket.h"
#include "eventloop.h"

class EventLoop;

class Channel
{
private:
    int fd_ = -1;               //一个channel对应一个fd 
    EventLoop *loop_ = nullptr; //非当前类成员,channel关联的事件循环，一个channel只对应一个事件循环
    bool inepoll_ = false;      //用来调整所属epoll，如果未添加，掉用eoll_ctrl用EPOLL_CTL_ADD, 否则用EPOLL_CTL_MOD
    uint32_t events_ = 0;       //channel关联的监控事件，listenfd和clientfd用EPOLLIN，clientfd还可能用EPOLLOUT
    uint32_t revents_ = 0;      //存放channel已发生的事件
    
    std::function<void()> readcb_;  //读事件回调函数
    std::function<void()> writecb_; //读事件回调函数
    std::function<void()> closecb_; //连接关闭回调函数
    std::function<void()> errorcb_; //出错回调函数   
public:
    Channel(int fd, EventLoop *loop);
    ~Channel();

    int fd();                       //返回fd
    void setET();                   //设置为边缘触发
    void enableread();              //使能读事件
    void disableread();             //关闭读事件
    void enablewrite();             //使能写事件
    void disablewrite();            //关闭写事件
    void disableall();              //关闭所有事件
    bool isinepoll();               //获取inepoll值
    void setinepoll();              //将inepoll设为true
    void unsetinepoll();            //将inepoll设为false
    void removefromepoll();         //从epoll中删除
    void setrevents(uint32_t revs); //设置revents_
    uint32_t events();              //返回events_
    uint32_t revents();             //返回revents_
    
    void registReadcb(std::function<void()> fn);    //注册回调函数
    void registWritecb(std::function<void()> fn);   //注册回调函数
    void registClosecb(std::function<void()> fn);   //注册回调函数
    void registErrorcb(std::function<void()> fn);   //注册回调函数
    void handleEvent();                             //处理返回的事件
};


