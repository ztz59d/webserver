// #ifndef HTTP_CONN_H
// #define HTTP_CONN_H

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

#include "locker.h"
#include "timer.h"
#include "entry.h"
#include "/buffer/buffer.h"

class Entry;
class Buffer;
class Http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    int epoll_fd;
    static int m_user_count;
    std::weak_ptr<Entry> m_entry;

    enum CHECK_STATE
    {
        CHECK_STATE_REQUEST_LINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };

    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATCH
    };

    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCES,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

private:
    int m_socket_fd;
    sockaddr_in m_address;

    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_idx;
    int m_checked_idx;
    int m_start_line;
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;

    CHECK_STATE m_check_state;
    METHOD m_method;

    char m_real_file[FILENAME_MAX];
    char *m_url;
    char *m_version;
    char *m_host;
    int m_content_length;
    bool m_linger;

    char *m_file_address;
    struct stat m_file_stat;

    struct iovec m_iv[2];
    int m_iv_count;

    Buffer read_buffer;
    Buffer write_buffer;
    int ErrorCode;

public:
    Http_conn(int sockfd, int epollfd)
    {
        m_socket_fd = sockfd;
        epoll_fd = epollfd;
    };
    ~Http_conn(){};

    void init(int socketfd)
    {
        m_socket_fd = socketfd;
        read_buffer.re
    };
    void close_conn(bool real_close = false)
    {
        if (real_close)
        {
        }
        else
        {
            m_socket_fd = 0;
            epoll_fd = 0;
            write_buffer.Clean();
            read_buffer.Clean();
        }
    };
    void process();

    // io非阻塞
    inline bool read()
    {
        read_buffer.ReadFd(m_socket_fd, &ErrorCode);
        if (ErrorCode != errno)
        {
            return true;
        }
        return false;
    };
    inline bool write()
    {
        int len = write_buffer.WriteFd(m_socket_fd, &ErrorCode);
        if (write_buffer.ReadableBytes() > 0)
        {
            // epoll_event event;
            // event.data.fd = m_socket_fd;

            // event.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
            // epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_socket_fd, &event);
            return false;
        }
        return true;
    };
    inline bool ParseHttp()
    {
        char method[30];
        char url[1024];
        char path[512];
        struct stat st;
        int num_line = read_buffer.get_line(read_buffer.readPos_);
        if (num_line <= 0)
        {
            return false;
        }
        int i = 0, j = 0;
        while (*(read_buffer.BeginPtr_() + i) != ' ' && (i < num_line) && i < sizeof(method) - 1)
        {
            method[i] = *(read_buffer.BeginPtr_() + i);
            i++;
        }
        j = i;
        method[i] = '\0';

        if (strcasecmp(method, "GET") != 0)
        {
            return false;
        }
        i = 0;
        while (*(read_buffer.BeginPtr_() + j) == ' ' && (j < num_line))
        {
            j++;
        }
        while (*(read_buffer.BeginPtr_() + j) != ' ' && (i < sizeof(url) - 1) && (j < num_line))
        {
            url[i] = *(read_buffer.BeginPtr_() + j);
            i++;
            j++;
        }
        url[i] = '\0';

        if (strcasecmp(method, "GET") == 0)
        {
            char *query_string = url;
            while ((*query_string != '?') && (*query_string != '\0'))
                query_string++;
            if (*query_string == '?')
            {
                *query_string = '\0';
                query_string++;
            }
            printf("query_string: %s\n", query_string);
        }
        sprintf(path, "htdocs%s", url);
        if (path[strlen(path) - 1] == '/')
            strcat(path, "index.html");
        if (stat(path, &st) == -1)
        {
            while ((num_line > 0) && strcmp("\n", read_buffer.BeginPtr_())) /* read & discard headers */
            {
                num_line = read_buffer.get_line(j);
                j += num_line;
            }
            // not_found(client);
        }
        else
        {
                }
    }
    inline bool Process()
    {
        read();
    };
    inline void cleanup()
    {
        m_socket_fd = 0;
        read_buffer.Clean();
        write_buffer.Clean();
    };
    // void init(int socketfd);

private:
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE);

    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_header(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line()
    {
        return m_read_buf + m_start_line;
    }
    LINE_STATUS parse_line();

    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

    void bad_request(int client)
    {
        char buf[1024];

        sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
        send(client, buf, sizeof(buf), 0);
        sprintf(buf, "Content-type: text/html\r\n");
        send(client, buf, sizeof(buf), 0);
        sprintf(buf, "\r\n");
        send(client, buf, sizeof(buf), 0);
        sprintf(buf, "<P>Your browser sent a bad request, ");
        send(client, buf, sizeof(buf), 0);
        sprintf(buf, "such as a POST without a Content-Length.\r\n");
        send(client, buf, sizeof(buf), 0);
    }

    void cat(int client, FILE *resource)
    {
        char buf[1024];

        fgets(buf, sizeof(buf), resource);
        while (!feof(resource))
        {
            send(client, buf, strlen(buf), 0);
            fgets(buf, sizeof(buf), resource);
        }
    }

    void cannot_execute(int client)
    {
        char buf[1024];

        sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-type: text/html\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
        send(client, buf, strlen(buf), 0);
    }

    void error_die(const char *sc)
    {
        perror(sc);
        exit(1);
    }

    void not_found(int client)
    {
        char buf[1024];

        sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, SERVER_STRING);
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: text/html\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "your request because the resource specified\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "is unavailable or nonexistent.\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "</BODY></HTML>\r\n");
        send(client, buf, strlen(buf), 0);
    }

    void serve_file(int client, const char *filename)
    {
        FILE *resource = NULL;
        int numchars = 1;
        char buf[1024];

        buf[0] = 'A';
        buf[1] = '\0';
        while ((numchars > 0) && strcmp("\n", buf)) /* read & discard headers */
            numchars = get_line(client, buf, sizeof(buf));

        resource = fopen(filename, "r");
        if (resource == NULL)
            not_found(client);
        else
        {
            headers(client, filename);
            cat(client, resource);
        }
        fclose(resource);
    }

    void unimplemented(int client)
    {
        char buf[1024];

        sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, SERVER_STRING);
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: text/html\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "</TITLE></HEAD>\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "</BODY></HTML>\r\n");
        send(client, buf, strlen(buf), 0);
    }
};

#endif