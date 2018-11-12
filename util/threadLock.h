#pragma once

#include <pthread.h>
#include <sys/time.h>

#include "exception.h"
#include <iostream>

namespace vTimer
{

class ThreadMutex
{
public:
    ThreadMutex()
    {
        int res =  pthread_mutex_init(&_mutex, NULL);
        if (res != 0)
            throw Exception("[ThreadMutex::ThreadMutex] pthread_mutex_init error", res);
    }

    virtual ~ThreadMutex()
    {
        int res = pthread_mutex_destroy(&_mutex);
        if (res != 0)
            throw Exception("[ThreadMutex::~ThreadMutex] pthread_mutex_destroy error", res);
    }

    void lock()
    {
        int res = pthread_mutex_lock(&_mutex);
        if (res != 0)
            throw Exception("[ThreadMutex::lock] pthread_mutex_lock error", res);
    }

    bool tryLock()
    {
        int res = pthread_mutex_trylock(&_mutex);
        if (res != 0 && res != EBUSY)
            throw Exception("[ThreadMutex::tryLock] pthread_mutex_trylock error", res);

        return res == 0;
    }

    void unlock()
    {
        int res = pthread_mutex_unlock(&_mutex);
        if (res != 0)
            throw Exception("[ThreadMutex::unlock] pthread_mutex_lock error", res);
    }

    friend class ThreadCond;
private:
    ThreadMutex(const ThreadMutex&);
    ThreadMutex& operator=(const ThreadMutex&);

private:
    pthread_mutex_t     _mutex;
};



class ThreadCond
{
public:
    ThreadCond()
    {
        int res = pthread_cond_init(&_cond, NULL);
        if (res != 0)
            throw Exception("[ThreadCond::ThreadCond] pthread_cond_init error", res);
    }

    virtual ~ThreadCond()
    {
        int res = pthread_cond_destroy(&_cond);
        if (res != 0)
            throw Exception("[ThreadCond::~ThreadCond] pthread_cond_destroy error", res);
    }

    void signal()
    {
        int res = pthread_cond_signal(&_cond);
        if (res != 0)
            throw Exception("[ThreadCond::signal] pthread_cond_signal error", res);
    }

    void broadcast()
    {
        int res = pthread_cond_broadcast(&_cond);
        if (res != 0)
            throw Exception("[ThreadCond::broadcast] broadcast error", res);
    }

    template<typename Mutex>
    void wait(Mutex &mutex)
    {
        int res = pthread_cond_wait(&_cond, &mutex._mutex);
        if (res != 0)
            throw Exception("[ThreadCond::wait] pthread_cond_wait error", res);
    }

    template<typename Mutex>
    bool timedWait(Mutex &mutex, int millsecond)
    {
        timespec ts;
        struct timeval tv;
        gettimeofday(&tv, NULL);
        ts.tv_sec = tv.tv_sec + millsecond / 1000 ;
        ts.tv_nsec = tv.tv_usec * 1000 + millsecond % 1000;

        int res = pthread_cond_timedwait(&_cond, &mutex._mutex, &ts);
        if (res != 0)
        {
            if (res != ETIMEDOUT)
                throw Exception("[ThreadCond::timedWait] pthread_cond_timedwait error", res);
            return false;
        }
        return true;
    }

private:
    ThreadCond(const ThreadCond&);
    ThreadCond& operator=(const ThreadCond&);

private:
    pthread_cond_t      _cond;
};


template<typename T>
class LockT
{
public:
    LockT(T& mutex) : _mutex(mutex)
    {
        _mutex.lock();
        _bLocked= true;
    }

    virtual ~LockT()
    {
        if (_bLocked)
            _mutex.unlock();
    }

    void lock()
    {
        if (_bLocked)
            throw Exception("thread has locked!");
        _mutex.lock();
        _bLocked = true;
    }

    void unlock()
    {
        if (!_bLocked)
            throw Exception("thread hasn't been locked!");
        _mutex.unlock();
        _bLocked = false;
    }

    void isLocked() const
    {
        return _bLocked;
    }

protected:
    T&          _mutex;
    bool        _bLocked;
};



template<class MUTEX, class COND>
class Monitor
{
public:

    typedef LockT<Monitor<MUTEX, COND> > Lock;

    Monitor() : _notify(0)
    {
    }

    virtual ~Monitor()
    {
    }

    void lock()
    {
        _mutex.lock();
        _notify = 0;
    }

    void unlock()
    {
        notifyImpl();
        _mutex.unlock();
    }

    void notify()
    {
        if (_notify != -1)
            ++_notify;
    }

    void notifyAll()
    {
        _notify = -1;
    }

    void wait()
    {
        notifyImpl();

        try
        {
            _cond.wait(_mutex);
        }
        catch(...)
        {
            _notify = 0;
            throw;
        }
        _notify = 0;
    }

    bool timedWait(int millsecond)
    {
        notifyImpl();
        bool rc;

        try
        {
            rc = _cond.timedWait(_mutex, millsecond);
        }
        catch(...)
        {
            _notify = 0;
            throw;
        }
        _notify = 0;
        return rc;
    }

private:

    void notifyImpl()
    {
        if (_notify !=0)
        {
            if (_notify == -1)
            {
                _cond.broadcast();
                return;
            }
            else
            {
                while (_notify > 0)
                {
                    _cond.signal();
                    --_notify;
                }
            }
        }
    }

    Monitor(const Monitor&);
    Monitor& operator=(const Monitor&);

private:
    int         _notify;     // 通知的次数
    MUTEX       _mutex;
    COND        _cond;
};


typedef Monitor<ThreadMutex, ThreadCond>  ThreadLock;

} // namespace vTimer
