#include "eventloop.h"


EventLoop::EventLoop(bool ismainloop, int timer_cycle_s, int conn_timeout_s):ismainloop_(ismainloop),conntimeout_thr_(conn_timeout_s),
                                ep_(new Epoll),stop_(false),
                                eventfd_(eventfd(0,EFD_NONBLOCK)),
                                wakeupch_(new Channel(eventfd_,this)),
                                timerfd_(timerfd(timer_cycle_s)),
                                timerch_(new Channel(timerfd_, this))
{
    if(conntimeout_thr_ != -1)
    {
        wakeupch_->registReadcb(std::bind(&EventLoop::wakeupcb,this));
        wakeupch_->enableread();

        timerch_->registReadcb(std::bind(&EventLoop::timercb,this));
        timerch_->enableread();
    }
}

EventLoop::~EventLoop()
{
    
}

void EventLoop::updatechannel(Channel *ch) //添加/更新channel到事件循环的epoll红黑树上
{
    ep_->updatechannel(ch);
}

void EventLoop::removechannel(Channel *ch) //删除channel
{
    ep_->removechannel(ch);
}

void EventLoop::run(int timeout_ms)  //运行事件循环
{
    threadid_ = syscall(SYS_gettid); //记录所在线程id
    while(stop_ == false)
    {  
        std::vector<Channel *> chs = ep_->loop(timeout_ms);

        if(chs.empty()) //超时
        {
            timeoutcb_(this);
        }

        for(auto &ch:chs)
        {
            ch->handleEvent();
        }
    }
}

int EventLoop::epfd() //获取事件循环对应的epoll_fd
{
    return ep_->epfd();
}

bool EventLoop::isinloopthread() //是否处于事件循环线程中
{
    return syscall(SYS_gettid) == threadid_;
}

void EventLoop::registtimeoutcb(std::function<void(EventLoop *loop)> fn) //注册超时回调函数
{
    timeoutcb_ = fn;
}

void EventLoop::registremoveconncb(std::function<void(spConnection)> fn) //注册超时删除函数
{
    removeconncb_ = fn;
}

void EventLoop::addtask(std::function<void()> fn) //添加任务到唤醒队列
{
    {
        std::unique_lock<std::mutex> l(mtx_);
        taskqueue_.push(fn);
    }
    wakeup();
}

void EventLoop::wakeup() //唤醒
{
    uint64_t buf = 1;
    write(eventfd_, &buf, sizeof(uint64_t));
}

void EventLoop::wakeupcb() //唤醒后的事件
{
    uint64_t val;
    read(eventfd_, &val, EFD_NONBLOCK);

    std::function<void()> task;
    while(true)
    {
        {
            std::unique_lock<std::mutex> l(mtx_);
            if(taskqueue_.empty()) break;

            task = std::move(taskqueue_.front());
            taskqueue_.pop();
        }
        task();
    }
}

void EventLoop::addconn(spConnection conn) //添加连接到集合
{
    {
        std::unique_lock<std::mutex> l(connsmtx_);
        conns_[conn->fd()] = conn;
    }
}

int EventLoop::timerfd(int cycle_s)
{
    int tfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);

    memset(&timerconfig_,0,sizeof(itimerspec));
    timerconfig_.it_value.tv_sec = cycle_s;  //设置定时时间
    timerconfig_.it_value.tv_nsec = 0;
    timerfd_settime(tfd,0,&timerconfig_,0);

    return tfd;
}

void EventLoop::timercb() //定时器事件
{
    timerfd_settime(timerfd_,0,&timerconfig_,0); //重设定时器

    if(!ismainloop_) //只关注conn
    {
        //将超时的连接删除（将会变更conn_的迭代器）
        for(auto it = conns_.begin(); it != conns_.end();  )
        {
            if(it->second->istimeout(time(0)) || it->second->isconnclose())
            {
                removeconncb_(it->second); //必须在前面
                
                {
                    std::unique_lock<std::mutex> l(connsmtx_);
                    it = conns_.erase(it);
                }
            }
            else
            {
                it++;
            }
        }
    }
}

int EventLoop::getTimeoutThr() //当前事件循环超时的阈值
{
    return conntimeout_thr_;
}

void EventLoop::stop() //停止事件循环
{
    stop_ = true;
    wakeup();
}