#ifndef MY_THREAD_H
#define MY_THREAD_H

#include "timer.h"
#include <functional>
#include "epoller.h"
// #include "threadpool.h"
#include <unordered_map>
#include <sys/eventfd.h>

// extern int addfd(int epoll_fd, int fd, bool ont_shot);
// extern int removefd(int epoll_fd, int fd);
// int set_nonblocking(int fd);
#include "http_conn.h"
class Http_conn;

// void OnProcess(HttpConn *client)
// {
//     if (client->process())
//     {
//         epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
//     }
//     else
//     {
//         epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
//     }
// }

class Epoller;
class Timer;
class BaseThread
{
public:
    virtual ~BaseThread(){};
    virtual void run() = 0;
};

class IOThread : public BaseThread
{
    // 主线程的fd队列
    // 读写

    // std::list<std::pair<int, Http_conn *>> m_connections;
    Epoller m_epoller;
    Timer m_timer;
    int m_eventfd;
    int m_max_fds;
    int m_fds;
    epoll_event *events;
    std::mutex m_mutex;
    std::unordered_map<int, Http_conn *> m_map;
    // ThreadPool<Http_conn> *pool;

public:
    IOThread(int eventfd, int max_fds = 101, int tic = 1, int ttl = 30) : m_eventfd(eventfd), m_max_fds(max_fds - 1), m_epoller(max_fds - 1), m_timer(m_epoller.get_epollfd(), tic, ttl), events(m_epoller.events), m_fds(0)
    {
        epoll_event event;
        event.data.fd = m_eventfd;
        int old_option = fcntl(m_eventfd, F_GETFL);
        int new_option = old_option | O_NONBLOCK;
        fcntl(m_eventfd, F_SETFL, new_option);

        // 注意epoll的参数
        event.events = EPOLLIN;
        // event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        epoll_ctl(m_epoller.get_epollfd(), EPOLL_CTL_ADD, m_eventfd, &event);
        // m_epoller.removefd(7);
        printf("m_eventfd : %d\n", m_eventfd);
    };
    ~IOThread()
    {
        // delete[] events;
    }

    virtual void run()
    {
        Http_conn dummy(0, 0);
        for (;;)
        {
            int n = 0;
            n = m_epoller.wait(-1);
            if (n < 0)
            {
                printf("epoll_wait error");
                continue;
            }
            // sleep(1);
            for (int i = 0; i < n; i++)
            {
                if ((events[i].data.fd == m_timer.timer_fd) && i != n - 1)
                {
                    std::swap(events[i], events[n - 1]);
                }
                if (events[i].data.fd == m_timer.timer_fd)
                {
                    long long timer_buf;
                    read(m_timer.timer_fd, &timer_buf, sizeof(timer_buf));
                    m_timer.expired();

                    // printf("timer expired\n");
                }
                else if (events[i].data.fd == m_eventfd)
                {
                    int fd = 0;
                    int num_read = 0;
                    printf("event : %d\n", events[i].data.fd);
                    // if(m_fds >= m_max_fds)
                    // {
                    //     close()
                    // }

                    while (((num_read = read(m_eventfd, (void *)&fd, sizeof(int))) > 0))
                    {
                        if (m_map.find(fd) == m_map.end())
                        {
                            m_map[fd] = new Http_conn(fd, m_eventfd);
                            // init
                            std::shared_ptr<Entry> entry_ptr = std::make_shared<Entry>(m_map[fd]);
                            m_map[fd]->m_entry = entry_ptr;
                            m_timer.addTimer(entry_ptr);
                            printf("thread %ld got new connection %d, current time : %ld\n", pthread_self(), fd, time(NULL));
                        }
                        else
                        {
                            std::shared_ptr<Entry> entry_ptr = m_map[fd]->m_entry.lock();
                            if (entry_ptr)
                            {
                                entry_ptr->_conn = &dummy;
                            }

                            entry_ptr = std::make_shared<Entry>(m_map[fd]);
                            m_map[fd]->m_entry = entry_ptr;
                            m_timer.addTimer(entry_ptr);
                            printf("thread %ld got new connection %d, current time : %ld\n", pthread_self(), fd, time(NULL));
                        }
                        m_fds++;
                        m_map[fd]->init(fd);
                        m_epoller.addfd(fd, true);
                    }
                }
                else if (events[i].events & EPOLLIN)
                {
                    std::shared_ptr<Entry> entry_ptr = m_map[events[i].data.fd]->m_entry.lock();
                    if (!m_map[events[i].data.fd]->Process())
                    {
                        m_epoller.addfd(events[i].data.fd, true);
                        m_timer.addTimer(entry_ptr);
                    }
                    printf("got read-in event \n");
                }
                else if (events[i].events & EPOLLOUT)
                {
                    m_map[events[i].data.fd]->write();
                    m_epoller.addfd(events[i].data.fd, true);
                }
            }
        }
    }
};

#endif