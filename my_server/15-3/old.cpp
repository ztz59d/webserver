#include "http_conn.h"
#include "locker.h"
#include "threadpool.h"

#define MAX_FD 65533
#define MAX_EVENT_NUMBER 10000

extern int addfd(int epoll_fd, int fd, bool ont_shot);
extern int removefd(int epoll_fd, int fd);

void addsig(int sig, void handler(int), bool restart = true)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
    {
        // 一般情况下 ，进程正在执行某个系统调用，那么在该系统调用返回前信号是不会被递送的。但慢速系统调用除外，如读写终端、网络、磁盘，以及wait和pause。这些系 统调用都会返回-1，errno置为EINTR当系统调用被中断时，我们可以选择使用循环再次调用，或者设置重新启动该系统调用 (SA_RESTART)。
        // 一旦给信号设置了SA_RESTART标记，那么当执行某个阻塞系统调用时，收到该信号时，进程不会返回，而是重新执行该系统调用。
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void show_error(int connfd, const char *info)
{
    std::cout << info << std::endl;
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int main(int argc, char *argv[])
{
    if (argc <= 0)
    {
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    addsig(SIGPIPE, SIG_IGN);
    ThreadPool<Http_conn> *pool = nullptr;
    try
    {
        pool = new ThreadPool<Http_conn>;
    }
    catch (...)
    {
        std::cerr << -1 << '\n';
    }

    Http_conn *users = new Http_conn[MAX_FD];
    assert(users);
    int user_count = 0;

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    struct linger tmp = {1, 0};
    setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);

    ret = listen(listenfd, 5);
    assert(ret >= 0);
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd, false);
    Http_conn::epoll_fd = epollfd;

    for (;;)
    {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR))
        {
            std::cout << "epoll failure" << std::endl;
            break;
        }

        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrlenght = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlenght);

                if (connfd < 0)
                {
                    std::cout << "errno : " << errno << std::endl;
                    continue;
                }

                if (Http_conn::m_user_count >= MAX_FD)
                {
                    show_error(connfd, "Internal Server Busy");
                    continue;
                }

                users[connfd].init(connfd, client_address);
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                users[sockfd].close_conn();
            }

            else if (events[i].events & EPOLLIN)
            {
                if (users[sockfd].read())
                {
                    pool->append(users + sockfd);
                }
                else
                {
                    users[sockfd].close_conn();
                }
            }

            else if (events[i].events & EPOLLOUT)
            {
                if (!users[sockfd].write())
                {
                    users[sockfd].close_conn();
                }
            }
            else
            {
            }
        }
    }

    close(epollfd);
    close(listenfd);
    delete[] users;
    delete pool;
    return 0;
}