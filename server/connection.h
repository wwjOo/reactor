#pragma once
#include <functional>
#include <cstring>
#include <atomic>
#include <memory>

#include "channel.h"
#include "buffer.h"
#include "eventloop.h"
#include "timestamp.h"

class Channel;
class Buffer;
class Timestamp;
class EventLoop;

class Connection;
using spConnection = std::shared_ptr<Connection>;

class Connection : public std::enable_shared_from_this<Connection>
{
private:
    EventLoop *loop_;                   //不属于本类
    std::unique_ptr<Socket> clientsock_;//不属于本类,但在本类释放
    std::unique_ptr<Channel> clientch_; //属于本类，需要释放

    std::atomic_bool disconnect_;       //是否已经关闭

    std::function<void(spConnection)> closecb_;  //处理关闭事件
    std::function<void(spConnection)> errorcb_;  //处理错误事件
    std::function<void(spConnection, std::string&)> messagecb_; //处理报文
    std::function<void(spConnection)> sendfinishcb_; //数据发送完毕回调

    Buffer inputbuffer_;    //Buffer内部仅有一个string，stl容器本身就是在堆区开辟内存的
    Buffer outputbuffer_;   //Buffer内部仅有一个string，stl容器本身就是在堆区开辟内存的

    Timestamp tmstamp_;     //时间戳，用于处理超时事件 (在IO读线程事件中更新)
    int timeoutthr_;        //超时阈值，单位：s (-1代表不检测超时)
public:
    Connection(EventLoop *loop, std::unique_ptr<Socket> clientsock);
    ~Connection();

    int fd() const;
    std::string ip() const;
    uint16_t port() const;
    Timestamp tmstamp() const;

    void readcb();    //读事件回调
    void writecb();   //写事件回调
    void closecb();   //关闭事件回调
    void errorcb();   //错误事件回调

    void registclosecb(std::function<void(spConnection)> fn);
    void registerrorcb(std::function<void(spConnection)> fn);
    void registmessagecb(std::function<void(spConnection, std::string&)> fn);
    void registsendfinishcb(std::function<void(spConnection)> fn);

    void send(const char *str, size_t size); //发送message函数
    void send(const std::string &str);
    void onsend(std::shared_ptr<std::string> str, size_t size); //单独出来

    bool istimeout(time_t now); //判断是否超时

    bool isconnclose(); //连接是否已经关闭
};
