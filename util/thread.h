#pragma once

#include <pthread.h>

#include "threadLock.h"
#include "exception.h"

namespace vTimer
{

class Thread
{
public:
    Thread() : _bRunning(false), _tid((pthread_t)-1) {}

    virtual ~Thread() {}

    virtual void run() = 0;

    static void threadEntry(Thread *pThread)
    {
        pThread->_bRunning = true;
        {
            ThreadLock::Lock sync(pThread->_lock);
            pThread->_lock.notifyAll();
        }

        try
        {
            pThread->run();
        }
        catch(...)
        {
            pThread->_bRunning = false;
            throw;
        }
        pThread->_bRunning = false;
    }

    pthread_t getTid() const { return _tid;}

    void start()
    {   
        ThreadLock::Lock lock(_lock);

        if (_bRunning)
            throw Exception("[Thread::start] thread has been start");

        int ret = pthread_create(&_tid, 0, (void *(*)(void *) )&threadEntry, (void *)this);
        if (ret !=0 )
            throw Exception("[Thread::start] thread start error", ret);

        _lock.wait();
    }   

    bool isAlive() const { return _bRunning;}

    void join()
    {
        int ret = pthread_join(_tid, NULL);
        if (ret != 0)
            throw Exception("[Thread::join] pthread_join error", ret);
    }

    void detach()
    {
        int ret = pthread_detach(_tid);
        if (ret != 0)
            throw Exception("[Thread::detach] pthread detach error", ret);
    }

    void yield()
    {
        sched_yield();
    }

private:
    bool        _bRunning;
    pthread_t   _tid;
    ThreadLock  _lock;
};

} // namespace vTimer
