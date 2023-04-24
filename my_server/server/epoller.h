#ifndef EPOLLER_H
#define EPOLLER_H
#include <sys/epoll.h> //epoll_ctl()
#include "unistd.h"    //
#include <vector>
#include <memory>

class Epoller
{
private:
    /* data */

    int epollFd;
    std::vector<epoll_event> events;

public:
    static int MAX_FD;
    Epoller(/* args */);
    Epoller(int size);
    ~Epoller();
    int add_fd(int fd, uint32_t event);
    int mod_fd(int fd, uint32_t event);
    int del_fd(int fd);
    int wait(int timeout_ms);
    std::vector<epoll_event> &get_events();
};

Epoller::Epoller(/* args */) : epollFd(epoll_create(MAX_FD)), events(MAX_FD){

                                                              };

Epoller::Epoller(int size) : epollFd(epoll_create(size)), events(size){

                                                          };

Epoller::~Epoller()
{
    close(epollFd);
}

int Epoller::add_fd(int fd, uint32_t event)
{
    if (fd < 0)
        return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = event;
    return 0 == epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev);
}

int Epoller::mod_fd(int fd, uint32_t event)
{
    if (fd < 0)
        return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = event;
    return 0 == epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev);
}
int Epoller::del_fd(int fd)
{
    if (fd < 0)
        return false;
    epoll_event ev = {0};
    return 0 == epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev);
}

int Epoller::wait(int timeoutMs)
{
    int count = epoll_wait(epollFd, &events[0], static_cast<int>(events.size()), timeoutMs);
}
std::vector<epoll_event> &Epoller::get_events()
{
    return events;
}

int Epoller::MAX_FD = 512;
#endif