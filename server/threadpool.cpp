#include "threadpool.h"


ThreadPool::ThreadPool(int thread_num, std::string type) : threadnum_(thread_num), stop_(false), type_(type)
{
    for(int i=0; i<thread_num; i++)
    {
        threads_.emplace_back([this,i]()
        {
            cout << type_ << "(" << syscall(SYS_gettid) << ")" << endl;
            
            while(true)
            {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> l(mtx_);
                    
                    //等待任务队列中有任务 || 线程终止
                    cond_.wait(l,[this](){ return stop_ == true || !taskqueue_.empty(); });

                    //终止线程，当收到线程结束符后，需要等待任务队列执行完成后再退出
                    if(stop_ == true && taskqueue_.empty()) 
                    {
                        cout << "TID:" << syscall(SYS_gettid) << " exit(" << this->type_ << ")" << endl;
                        return; 
                    }
                    
                    task = std::move(taskqueue_.front()); //采用移动语义，避免拷贝
                    taskqueue_.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    stop();
    for(auto &thread:threads_)
    {
        thread.join();
    }
}

void ThreadPool::addtask(std::function<void()> fn) //向任务队列添加任务
{
    if(stop_ == true) return;

    {
        std::unique_lock<std::mutex> l(mtx_);
        taskqueue_.push(fn);
    }

    cond_.notify_one();
}

void ThreadPool::stop() //终止线程池(陷入死循环的线程没法捕捉stop，故暂时无法结束)
{
    if(stop_ == true) return;

    stop_ = true;
    cond_.notify_all();
}

int ThreadPool::size() //获取线程池大小
{
    return threadnum_;
}