#pragma once
//该部分将epoll和事件的循环监视进行封装，因为一个程序中不止有一个epoll的循环监视

#include <functional>
#include <unistd.h>
#include <sys/syscall.h>
#include <memory>
#include <queue>
#include <mutex>
#include <unordered_map>
#include <sys/eventfd.h>
#include <sys/timerfd.h>

#include "connection.h"
#include "epoll.h"
class Channel;
class Epoll;
class Connection;
using spConnection = std::shared_ptr<Connection>;

class EventLoop
{
private:
    bool ismainloop_;           //是否为主事件循环

    std::unique_ptr<Epoll> ep_; //每个事件循环只有一个Epoll，构造时在堆区开辟
    std::function<void(EventLoop *loop)> timeoutcb_;    //等待超时回调函数
    pid_t threadid_;                                    //所在线程id

    std::queue<std::function<void()>> taskqueue_;       //发送唤醒处理队列
    std::mutex mtx_;                                    //唤醒队列互斥锁
    int eventfd_;                                       //唤醒fd
    std::unique_ptr<Channel> wakeupch_;                 //唤醒通道

    itimerspec timerconfig_;                            //定时器配置
    int timerfd_;                                       //定时器fd
    std::unique_ptr<Channel> timerch_;                  //定时器通道
    int conntimeout_thr_;                               //连接超时阈值，单位：s
    std::function<void(spConnection)> removeconncb_;    //关闭超时的连接回调，将调用tcpserver中的close                     

    std::unordered_map<int, spConnection> conns_;       //处于该事件循环的连接 (在主线程中添加,在IO事件中使用，需要加锁)
    std::mutex connsmtx_;                               //conns_互斥锁
public:
    EventLoop(bool ismainloop, int timer_cycle_s, int conn_timeout_s);   //参数为定时器周期，单位：s;  连接超时事件，单位：s;
    ~EventLoop();

    void updatechannel(Channel *ch);//添加/更新channel到事件循环的epoll红黑树上
    void removechannel(Channel *ch);//删除channel
    void run(int timeout_ms = -1);  //执行事件循环
    int epfd();                     //获取事件循环对应的epoll_fd
    bool isinloopthread();          //是否处于事件循环线程中

    void registtimeoutcb(std::function<void(EventLoop *loop)> fn); //注册超时回调函数
    void registremoveconncb(std::function<void(spConnection)> fn); //注册超时删除函数
    void addtask(std::function<void()>);    //添加任务到唤醒队列
    void wakeup();      //唤醒
    void wakeupcb();    //唤醒后的事件

    void addconn(spConnection conn); //添加连接到集合

    int timerfd(int cycle_s); //创建定时器
    void timercb();     //定时器事件
    int getTimeoutThr();//当前事件循环超时的阈值

    std::atomic<bool> stop_; //停止事件循环标记
    void stop();             //停止事件循环
};
