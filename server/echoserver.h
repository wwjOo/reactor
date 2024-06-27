#pragma once
#include <memory>

#include "tcpserver.h"
#include "threadpool.h"
class TcpServer;
class ThreadPool;

class EchoServer
{
private:
    std::unique_ptr<TcpServer> server_;
    std::unique_ptr<ThreadPool> threadpool_;    //工作线程池
public:
    EchoServer(const std::string &ip, const u_int16_t port, int timeout_ms = -1, int subthreadnum = 3, 
                int workthreadnum = 5, int conntimercycle_s = 30, int conntimeout_s = 50);
    ~EchoServer(); 

    void HandleNewConnect(spConnection conn); //新的连接回调
    void HandleClose(spConnection conn); //关闭事件回调
    void HandleError(spConnection conn); //错误事件回调
    void HandleMessage(spConnection conn, std::string &msg); //消息包处理回调
    void onmessage(spConnection conn, std::string &msg); //工作线程中处理
    void HandleSendComplete(spConnection conn); //数据发送完成回调
    void HandleEpollTimeout(EventLoop *loop); //事件循环超时回调

    void run(int timeout_ms = -1);
    void stop();
};