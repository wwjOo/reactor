#include "timestamp.h"




Timestamp::Timestamp(/* args */)
{
    itimesec_ = time(0); //获取当前系统时间
}

Timestamp::~Timestamp()
{

}

Timestamp Timestamp::now()
{
    return Timestamp();
}

time_t Timestamp::toint() const
{
    return itimesec_;
}

std::string Timestamp::tostring() const
{
    char buf[128] = {0};
    tm *local_time = localtime(&itimesec_);
    snprintf(buf,128,"%4d-%02d-%02d %02d:%02d:%02d",
            local_time->tm_year + 1900,
            local_time->tm_mon + 1,
            local_time->tm_mday,
            local_time->tm_hour,
            local_time->tm_min,
            local_time->tm_sec);
    
    return buf;
}




