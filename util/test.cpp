#include <iostream>
#include "thread.h"
#include <unistd.h>

using namespace std;
using namespace vTimer;

class mythread : public Thread, ThreadLock
{
public:
    mythread()
    {
        _bTerminate = false;
    }

    void terminate()
    {
        _bTerminate = true;

        {
            ThreadLock::Lock lock(*this);
            notifyAll();
        }
    }

    void doSomething()
    {
        cout << "doSomething" << endl;
    }

private:
    virtual void run()
    {
        while(!_bTerminate)
        {
            doSomething();
            {
                ThreadLock::Lock lock(*this);
                timedWait(1000);
            }
        }
    }

private:
    bool _bTerminate;
};

int main()
{
    try
    {
        mythread mt;
        mt.start();
        sleep(5);
        mt.terminate();
        mt.join();
    }
    catch(exception &ex)
    {
        cout << ex.what() << endl;
    }
    catch(...)
    {
        cout << "unknown exception" << endl;
    }
    return 0; 
}
