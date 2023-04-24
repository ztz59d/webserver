#ifndef SERVER_H
#define SERVER_H
#include "sys/socket.h"
#include "arpa/inet.h"
#include <string>
#include "stdlib.h"
#include "epoller.h"
#include <iostream>
#include <map>
#include "../http/http.h"
#include <mutex>
#include <../threadpool/pools.h>
class server
{
private:
    struct sockaddr_in address;
    int fd;
    int max_connect = 5;
    struct sockaddr client_addr;
    Epoller epoller;
    socklen_t length;
    std::map<int, HttpRequest *> http_conn;

public:
    server(/* args */);
    ~server();
    void deal_read(HttpRequest *request);

    void init();
};

server::server(/* args */) : epoller()
{
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    std::cout << "server socket created" << std::endl;
    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    std::cout << "server bind successful" << std::endl;
    if (listen(fd, 30) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "server now listening" << std::endl;
    epoller.add_fd(fd, EPOLLIN);
    for (;;)
    {
        int num = epoller.wait(0);
        if (num >= 0)
        {
            std::vector<epoll_event> events = epoller.get_events();
            int connfd;
            for (int i = 0; i < num; i++)
            {
                int sockfd = events[i].data.fd;
                if (sockfd == fd)
                {
                    connfd = accept(sockfd, &client_addr, &length);
                    std::cout << "connection established" << std::endl;
                    epoller.add_fd(fd, EPOLLIN);
                    http_conn[fd] = new HttpRequest(fd);
                    deal_read(http_conn[fd]);
                }
                else if (events[i].events & EPOLLIN)
                {
                    printf("data already received\n");
                    deal_read(http_conn[sockfd]);
                }
            }
        }
    }
}
void server::deal_read(HttpRequest *http_conn)
{
    // insert task into queue
    http_conn->tryDecode();
}

server::~server()
{
    close(fd);
}

#endif