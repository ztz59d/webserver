#ifndef TIMER_H
#define TIMER_H

#include <list>
#include <memory>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <iostream>
#include <unistd.h>
#include <mutex>
#include "entry.h"
// #include "http_conn.h"

class Entry;
class Timer
{
    typedef std::list<std::shared_ptr<Entry>> TimerList;
    typedef std::list<TimerList> TimerQueue;

public:
    TimerQueue m_timerQueue;
    TimerQueue::iterator m_current;
    int m_gap;
    int m_circle;
    int epoll_fd;
    int timer_fd;
    std::mutex m_lock;

    Timer(int epollfd, int gap = 1, int circle = 30) : epoll_fd(epollfd), m_gap(gap), m_circle(circle), m_timerQueue(circle, std::list<std::shared_ptr<Entry>>()), m_current(m_timerQueue.begin())
    {
        // printf("m_circle = %d, m_gap = %d\n", m_circle, m_gap);

        struct itimerspec timer = {};
        timer.it_interval.tv_sec = gap;
        timer.it_interval.tv_nsec = 0;
        timer.it_value.tv_sec = gap;
        timer.it_value.tv_nsec = 0;
        timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        timerfd_settime(timer_fd, 0, &timer, NULL);
        // printf("timerfd : %d\n", timer_fd);

        // epoll_fd = epoll_create(1);
        struct epoll_event ev = {};
        ev.events = EPOLLIN; //
        ev.data.fd = timer_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &ev);
    };

    // void loop()
    // {
    //     struct epoll_event events[5];
    //     long long timer_buf;
    //     while (true)
    //     {
    //         int n = epoll_wait(epoll_fd, events, 5, -1);
    //         for (int i = 0; i < n; i++)
    //         {
    //             if (events[i].data.fd == timer_fd)
    //             {
    //                 read(timer_fd, &timer_buf, sizeof(timer_buf));
    //                 addTimer(std::make_shared<Entry>());
    //                 expired();
    //             }
    //         }
    //     }

    //     // int x = ({char* ret; ret = "aaaa"; ret; });
    // }

    // typedef std::list<std::shared_ptr<Entry>> TimerList;
    // typedef std::list<TimerList> TimerQueue;
    void expired()
    {
        // printf("size of m_timerQueue : %ld\n", m_timerQueue.size());
        if (m_timerQueue.empty())
        {
            return;
        }
        if (++m_current == m_timerQueue.end())
        {
            m_current = m_timerQueue.begin();
        }
        m_current->clear();
    }

    void addTimer(std::shared_ptr<Entry> entry)
    {
        if (entry)
        {
            m_current->push_back(entry);
        }
    }
};

#endif