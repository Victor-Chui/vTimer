#include <iostream>
#include <unistd.h>
#include <time.h>
#include "vTimer.h"

static void test(void *arg, void *res)
{
    char *px = (char*)arg;
    std::cout << "px: " << px << std::endl;
    if (res != NULL)
        *(int *)res = 0; 
}

int main()
{
    char xx[10] = "hello";
    char xx2[10] = "hello2";

    vTimer::TimeEvent event;
    //event.setTimestamp(time(NULL) + 4);
    event.setProc(test);
    event.setRepeat(1);
    int *res = new int;
    event.setArg(xx);
    event.setRes(res);

    vTimer::TimeEvent event2;
    event2.setProc(test);
    event2.setRepeat(5);
    int *res2 = new int;
    event2.setArg(xx2);
    event2.setRes(res2);

    vTimer::vTimer timer;
    int id1 = timer.addTimeEvent(&event);
    int id2 = timer.addTimeEvent(&event2);
    std::cout << "id1: " << id1 << std::endl;
    std::cout << "id2: " << id2 << std::endl;
    timer.start();
    while(1)
    {
        sleep(5);
        timer.deleteEvent(id1);
    }
    //timer.terminate();
    //timer.join();
    std::cout << "res--:" << *res << std::endl;
    return 0;
}
