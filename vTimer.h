#pragma once

#include <vector>
#include <iostream>
#include <time.h>
#include "util/thread.h"

namespace vTimer
{

typedef void TimeProc(void *arg, void *res);

class TimeEvent
{
public:

    TimeEvent() : proc(NULL), arg(NULL), res(NULL), timestamp(0), repeat(0) {}

    virtual ~TimeEvent() {}

    void setId(unsigned int ID)
    {
        id = ID;
    }

    unsigned int getId() const
    {
        return id;
    }

    void setProc(TimeProc *timeProc)
    {
        proc = timeProc;
    }

    TimeProc *getProc() const
    {
        return proc;
    }

    void setTimestamp(const long t)
    {
        timestamp = t;
    }

    long getTimestamp() const
    {
        return timestamp;
    }

    void setArg(void *a)
    {
        arg = a;
    }

    void *getArg() const
    {
        return arg;
    }

    void setRes(void *r)
    {
        res = r;
    }

    void *getRes() const
    {
        return res;
    }

    void setRepeat(unsigned int second)
    {
        repeat = second;
    }

    unsigned int getRepeat() const
    {
        return repeat;
    }


private:
    unsigned int            id;         // 事件id
    TimeProc                *proc;      // 处理函数
    void                    *arg;       // 入参
    void                    *res;       // 出参
    long                    timestamp;  // 事件发生时间戳
    unsigned int            repeat;     // 重复的秒数
};

/**
 * @brief 定时器
 *        开启线程，利用小根堆存时间事件，pop出最近的事件，假如到时间就执行，
 *        一直pop，直到堆顶离执行还有x秒，线程等待x秒，等待可以采用epoll_wait
 *        的超时，这里采用的是线程锁信号量pthread_cond_timedwait的超时。
 *
 */
class vTimer : public Thread, ThreadLock
{
public:
    vTimer() : bTerminate(false) {}

    virtual ~vTimer() {}

    /**
     * @brief 终止定时器
     */
    void terminate()
    {
        bTerminate = true;
        {
            ThreadLock::Lock lock(*this);
            notifyAll();
        }
    }

    /**
     * @brief 添加事件时间
     *
     * @param event   时间事件
     * @return        事件id
     */
    unsigned int addTimeEvent(TimeEvent *event)
    {
        event->setId(++ID);
        if (event->getTimestamp() <= 0 && event->getRepeat() <= 0)
            return 0;
        if (event->getTimestamp() == 0)
            event->setTimestamp( (long)time(NULL) + event->getRepeat() );
        events.push_back(event);
        EventHeapUp(events.size() );
        return ID;
    }

    /**
     * @brief 弹出时间事件
     *
     * @return 时间事件
     */
    TimeEvent *popTimeEvent()
    {
        if (events.size() == 0)
            return NULL;

        TimeEvent *event = events[0];
        events[0] = events[events.size() - 1];
        events.pop_back();
        if (events.size() > 0)
            EventHeapDown(0);

        return event;
    }

    /**
     * @brief 获取最近的时间事件
     *
     * @return 时间事件
     */
    TimeEvent *peakTimeEvent()
    {
        if (events.size() == 0)
            return NULL;
        return events[0];
    }

    /**
     * @brief 删除事件
     *
     * @param id 事件id
     */
    void deleteEvent(unsigned int id)
    {
        delEvents.push_back(id);
        notifyAll();
    }

private:
    /**
     * @brief 消费事件
     *
     * @return 返回剩余毫秒，大于0则等待
     */
    int consumeEvent()
    {
        TimeEvent *event = peakTimeEvent();
        if (event == NULL)
            return 1000;
        long timestamp = event->getTimestamp();
        time_t now = time(NULL);
        long left = timestamp - (long)now;
        if (left <= 0)
        {
            event = popTimeEvent();
            event->getProc()(event->getArg(), event->getRes());
            if (event->getRepeat() > 0)
            {
                event->setTimestamp( (long)time(NULL) + event->getRepeat() );
                pushTimeEvent(event);
            }
        }
        return left * 1000;
    }

    void EventHeapUp(const int end_idx)
    {
        int now_idx = end_idx - 1;
        int parent_idx = (now_idx - 1) / 2;
        TimeEvent *event = events[now_idx];
        TimeEvent *parent_event = events[parent_idx];
        while (now_idx > 0 && parent_idx >= 0 && 
        event->getTimestamp() < parent_event->getTimestamp() )
        {
            events[now_idx] = events[parent_idx];
            now_idx = parent_idx;
            parent_idx = (now_idx - 1) / 2;
        }
        events[now_idx] = event;
    }

    void EventHeapDown(const int begin_idx)
    {
        int now_idx = begin_idx;
        TimeEvent *event = events[now_idx];
        int child_idx = (now_idx + 1) * 2;
        int eventSize = events.size();
        while (child_idx <= eventSize )
        {
            if (child_idx == eventSize 
                || events[child_idx - 1]->getTimestamp() < events[child_idx]->getTimestamp() )
                --child_idx;

            if (event->getTimestamp() < events[child_idx]->getTimestamp() )
                break;

            events[now_idx] = events[child_idx];
            now_idx = child_idx;
            child_idx = (now_idx + 1) * 2;
        }
        events[now_idx] = event;
    }

    void pushTimeEvent(TimeEvent *event)
    {
        if (event->getTimestamp() <= 0 && event->getRepeat() <= 0)
            return;
        if (event->getTimestamp() == 0)
            event->setTimestamp( (long)time(NULL) + event->getRepeat() );
        events.push_back(event);
        EventHeapUp(events.size() );
    }

    void delEvent(unsigned int id)
    {
        ThreadLock::Lock lock(*this);
        std::vector<TimeEvent*>::iterator it;
        it = events.begin();
        while(it != events.end() )
        {
            if ((*it)->getId() == id)
            {
                it = events.erase(it);
            }
            else
                ++it;
        }
    }

    void checkDelEvent()
    {
        std::vector<unsigned int>::iterator it;
        for (it = delEvents.begin(); it != delEvents.end(); ++it)
        {
            delEvent(*it);
        }
    }

    virtual void run()
    {
        while(!bTerminate)
        {
            checkDelEvent();
            int leftTime = consumeEvent();
            if (leftTime > 0)
            {
                ThreadLock::Lock lock(*this);
                timedWait(leftTime);
            }
        }
    }
    
private:
    std::vector<TimeEvent*>     events;        //时间事件
    std::vector<unsigned int>   delEvents;     //删除的事件
    bool                        bTerminate;    //终止标记
    unsigned int                ID;            //生成事件ID
};

};
