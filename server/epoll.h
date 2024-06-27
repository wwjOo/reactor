#pragma once
#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <vector>
#include <iostream>

#include "channel.h"
class Channel;

class Epoll
{
private:
    static const int MaxSize = 10; 
    int epfd_ = -1;                 //在构造函数中创建epoll描述符号
    epoll_event revents_[MaxSize];  //存放epoll_wait返回的事件集合   
public:
    Epoll();    //创建epoll
    ~Epoll();

    void updatechannel(Channel *ch);                //添加/更新channel到epoll红黑树上
    void removechannel(Channel *ch);                //删除channel
    std::vector<Channel*> loop(int timeout = -1);   //监控事件，返回事件集合
    int epfd(); //获取epfd
};

