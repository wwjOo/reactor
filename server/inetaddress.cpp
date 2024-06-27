#include "inetaddress.h"

inetaddress::inetaddress(const std::string &ip, const unsigned short port)
{
    addr_.sin_family = PF_INET;
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
    // addr_.sin_addr.s_addr = htonl(INADDR_ANY); //主机所有ip
    addr_.sin_port = htons(port);
}

inetaddress::inetaddress(const sockaddr_in recvaddr) : addr_(recvaddr)
{

}

inetaddress::~inetaddress()
{

}

const char *inetaddress::ip() const         
{
    return inet_ntoa(addr_.sin_addr);
}

unsigned short inetaddress::port() const  
{
    return ntohs(addr_.sin_port);
}

const sockaddr *inetaddress::addr() const  
{
    return (sockaddr *)&addr_;
}

void inetaddress::setaddr(const sockaddr_in &clientaddr)
{
    addr_ = clientaddr;
}