#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <unistd.h>
#include <sys/syscall.h>
#include <string>
using namespace std;


class ThreadPool
{
private:
    std::vector<std::thread> threads_;              //线程池本体
    std::queue<std::function<void()>> taskqueue_;   //任务队列
    std::mutex mtx_;                                //任务队列需要互斥访问
    std::condition_variable cond_;                  //用于实现线程的生产者消费者模型
    std::atomic<bool> stop_;                        //终止线程标志
    std::string type_;                              //线程池类型 “WORK”：工作线程， “IO”：为处理连接的IO子线程
    int threadnum_;                                 //线程数量
public:
    ThreadPool(int thread_num, std::string type);
    ~ThreadPool();
    void addtask(std::function<void()> fn);         //向任务队列添加任务
    void stop();                                    //终止线程池
    int size();                                     //获取线程池大小
};
