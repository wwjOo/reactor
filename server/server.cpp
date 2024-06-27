#include "echoserver.h"
#include <signal.h>
using namespace std;

EchoServer *echoserver;

void stop(int sig)
{
    cout << "~~~GOODBYE~~~" << endl;
    
    delete echoserver;
    
    exit(0);
}

int main(int argc, char const *argv[])
{
    if(argc != 3)
    {
        cout << "params error!" << endl;
        cout << "Usage1 : ./server-test 10.0.12.10 5005" << endl;
        cout << "Usage2 : make server" << endl;
        return -1;
    }

    signal(SIGINT, stop);   //2
    signal(SIGTERM, stop);  //15

    const int timeout_ms = -1;
    echoserver = new EchoServer(argv[1],atoi(argv[2]),timeout_ms,3, 4, 5, 15);
    echoserver->run(timeout_ms);

    return 0;
}


