#include "epoll.h"


Epoll::Epoll() 
{
    epfd_ = epoll_create(1); //创建epoll
    if(epfd_ == -1) 
    {
        perror("epoll_create failed");
        exit(-1);
    }
}

Epoll::~Epoll()
{
    close(epfd_);
}

void Epoll::updatechannel(Channel *ch) //添加/更新channel到epoll红黑树上
{
    epoll_event ev;
    ev.data.ptr = ch;
    ev.events = ch->events(); 

    int op = ch->isinepoll() ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    int res = epoll_ctl(epfd_, op, ch->fd(), &ev);
    if(res == -1)
    {
        perror("epoll_ctl failed");
        exit(-1);
    }

    ch->setinepoll();
}

void Epoll::removechannel(Channel *ch) //删除channel
{
    if(!ch->isinepoll()) return;

    if(epoll_ctl(epfd_, EPOLL_CTL_DEL, ch->fd(), 0) == -1)
    {
        perror("epoll_ctl"); exit(-1);
    }
    ch->unsetinepoll();
}

std::vector<Channel*> Epoll::loop(int timeout) //监控事件，返回事件集合
{
    std::vector<Channel*> chs;

    bzero(revents_,sizeof(revents_)); //wait前清空
    int nums = epoll_wait(epfd_, revents_, sizeof(revents_)/sizeof(epoll_event), timeout);

    //reactor模型中，不建议使用信号，因为处理起来比较麻烦
    if(nums < 0) //错误
    {
        if(errno != EINTR)
        {
            perror("epoll_wait"); 
            exit(-1);
        }
        else
        {
            std::cout << "epoll_wait : interruped by a signal handler"<< std::endl;
        }
    }
    if(nums == 0) //超时
    {
        return chs;
    }

    for(int i=0; i<nums; i++)
    {
        Channel *ch = static_cast<Channel *>(revents_[i].data.ptr);
        ch->setrevents(revents_[i].events);
        chs.push_back(ch);
    }

    return chs; //编译器会自动优化值传递
}

int Epoll::epfd() //获取epfd
{
    return epfd_;
}