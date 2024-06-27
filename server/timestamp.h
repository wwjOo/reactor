#pragma once

#include <time.h>
#include <string>

class Timestamp
{
private:
    time_t itimesec_;    //整数表示的时间
public:
    Timestamp(/* args */);
    ~Timestamp();

    static Timestamp now();

    time_t toint() const;
    std::string tostring() const;
};
