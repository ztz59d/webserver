#ifndef EPOLLER_H
#define EPOLLER_H

#include "sys/epoll.h"
#include "fcntl.h"
#include "stdio.h"
#include "unistd.h"
class Epoller
{
public:
    int m_epollfd;
    int m_maxfd;
    epoll_event *events;

public:
    Epoller(int max_fds = 100);
    ~Epoller();
    bool addfd(int fd, bool one_shot);
    bool removefd(int fd);
    bool modfd(int fd, int event);
    int wait(int timeout);
    inline int get_epollfd(void)
    {
        return m_epollfd;
    }
};

Epoller::Epoller(int max_fds) : m_maxfd(max_fds), m_epollfd(epoll_create(max_fds)), events(new epoll_event[max_fds])
{
    if (m_epollfd < 0 || events == nullptr)
    {
        printf("epoll_create failed\n");
    }
}

Epoller::~Epoller()
{
    if (events != nullptr)
    {
        delete[] events;
    }
}

int Epoller::wait(int timeout)
{
    int n = epoll_wait(m_epollfd, events, m_maxfd, timeout);

    return n;
}

bool Epoller::addfd(int fd, bool one_shot)
{
    epoll_event event;
    event.data.fd = fd;
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);

    // 注意epoll的参数
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if (one_shot)
    {
        event.events |= EPOLLONESHOT;
    }
    int res = epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &event);
    if (res == -1)
    {
        return false;
    }
    return true;
}

bool Epoller::removefd(int fd)
{
    int res = epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, 0);

    if (res == -1)
    {
        return false;
    }
    close(fd);
    return true;
}

bool Epoller::modfd(int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    int res = epoll_ctl(m_epollfd, EPOLL_CTL_MOD, fd, &event);
    if (res == -1)
    {
        return false;
    }
    return true;
}

#endif
