#pragma once
//最上层的底层类
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>

#include "eventloop.h"
#include "acceptor.h"
#include "threadpool.h"

class Acceptor;
class ThreadPool;
class EventLoop;

class TcpServer
{
private:
    std::unique_ptr<EventLoop> mainloop_;  //一个主事件循环，用于处理连接事件
    std::vector<std::unique_ptr<EventLoop>> subloops_; //多个从事件循环，用于处理服务
    int threadnum_; //线程池大小 (这里注意和线程池的初始化顺序，因为线程池会用到threadnum_,因此声明需要在threadpool_前面)
    std::unique_ptr<ThreadPool> threadpool_;    //线程池

    std::unique_ptr<Acceptor> acceptor_;    //监听端,一个TcpServer只有一个Accept对象

    std::mutex connmtx_;                            //连接对象集合的锁
    std::unordered_map<int, spConnection> conns_;   //连接的对象

    std::function<void(spConnection conn)> connectcb_;
    std::function<void(spConnection conn)> closecb_;
    std::function<void(spConnection conn)> errorcb_;
    std::function<void(spConnection conn, std::string &msg)> messagecb_;
    std::function<void(spConnection conn)> sendfinishcb_;
    std::function<void(EventLoop *loop)> epolltimeoutcb_;
public:

    TcpServer(const std::string &ip, const u_int16_t port, int eolltimeout_ms = -1, 
            int epollthreadnum = 3, int conntimercycle_s = 30, int conntimeout_s = 50);
    ~TcpServer();

    void start(int timeout_ms = -1); //启动事件循环
    void stop(); //停止所有事件循环

    //TcpServer层
    void newconnection(std::unique_ptr<Socket> clientsock); //新的连接
    void closecb(spConnection conn); //关闭事件回调
    void errorcb(spConnection conn); //错误事件回调
    void messagecb(spConnection conn, std::string &msg); //消息包处理回调
    void sendfinishcb(spConnection conn); //数据发送完成回调
    void epolltimeoutcb(EventLoop *loop); //事件循环超时回调

    //上层注册接口(关心什么事件就写什么事件)
    void registnewconncb(std::function<void(spConnection conn)> fn);//连接事件回调注册
    void registclosecb(std::function<void(spConnection conn)> fn); //关闭事件回调注册
    void registerrorcb(std::function<void(spConnection conn)> fn); //错误事件回调注册
    void registmessagecb(std::function<void(spConnection conn, std::string &msg)> fn); //消息包处理回调注册
    void registsendfinishcb(std::function<void(spConnection conn)> fn); //数据发送完成回调注册
    void registepolltimeoutcb(std::function<void(EventLoop *loop)> fn); //事件循环超时回调注册
};
