#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <memory.h>
#define PORT 8080
void process_conn_client(int fd)
{
    int size = 0;
    char *message = "sending message";
    // char *buffer = malloc(sizeof(char) * 1024);
    write(fd, message, strlen(message) + 1);
    // read(fd, buffer, 1024);
    printf("%s\n", "sending message");
}
int main(int argc, char *argv[])
{
    int ss;
    struct sockaddr_in server_addr;
    if ((ss = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("%s\n", "socket error");
        return -1;
    }
    bzero(&server_addr, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "222.200.184.59", &server_addr.sin_addr.s_addr);
    if (connect(ss, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) < 0)
    {
        printf("%s\n", "connect error");
        return -1;
    }
    process_conn_client(ss);
    close(ss);

    return 0;
}
