#include <thread>

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>

// #include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_IOTHREADS 10

int main(int argc, char *argv[])
{
    if (argc <= 2)
    {
        printf("please enter more than 2 arguments\n");
        exit(0);
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    struct linger tmp = {0, 1};
    setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);

    int old_option = fcntl(listenfd, F_GETFL);
    // assert(old_option & O_NONBLOCK);
    ret = listen(listenfd, 50);
    assert(ret >= 0);

    int pp[MAX_IOTHREADS][2];
    for (int i = 0; i < MAX_IOTHREADS; i++)
    {
        int old_option = fcntl(pp[i][0], F_GETFL);
        int new_option = old_option | O_NONBLOCK;
        fcntl(pp[i][0], F_SETFL, new_option);
        old_option = fcntl(pp[i][1], F_GETFL);
        new_option = old_option | O_NONBLOCK;
        fcntl(pp[i][1], F_SETFL, new_option);

        if (pipe(pp[i]) < 0)
        {
            printf("Couldn't pipe socket\n");
            exit(0);
        }
        printf("pp[0] : %d, pp[1] : %d\n", pp[i][0], pp[i][1]);
    }

    // IOThread *threads[MAX_IOTHREADS];
    // for (int i = 0; i < MAX_IOTHREADS; i++)
    // {
    //     threads[i] = new IOThread(pp[i][0]);
    //     std::thread t(&IOThread::run, threads[i]);
    //     t.detach();
    // }

    for (;;)
    {
        int i = 0;
        errno = 0;
        struct sockaddr_in client_addr;
        socklen_t client_addrlenght = sizeof(client_addr);
        int sock = accept(listenfd, (sockaddr *)&client_addr, &client_addrlenght);
        printf("socket connected %d\n", sock);
        if (sock < 0)
        {
            printf("Failed to accept %d, errno : %d\n", listenfd, errno);
            return 0;
        }
        i %= MAX_IOTHREADS;
        // write(pp[i][1], &sock, sizeof(sock));
        i++;
    }

    // delete[] threads;
    return 0;
}