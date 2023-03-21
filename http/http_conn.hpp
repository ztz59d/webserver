#ifndef _HTTP_CONN_H_
#define _HTTP_CONN_H_

#include "../buffer/buffer.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <regex>
#include <sys/stat.h>
#include "entry.h"
using std::cout;
using std::endl;

class Entry;
class Buffer;
class Http_connection
{
public:
    Http_connection(int sockfd, int epollfd)
    {
        m_socket_fd = sockfd;
        m_epoll_fd = epollfd;
        m_check_state = CHECK_STATE_REQUEST_LINE;
    };
    ~Http_connection() = default;
    enum CHECK_STATE
    {
        CHECK_STATE_REQUEST_LINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum METHOD
    {
        NONE = 0,
        GET,
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
        NO_REQUEST = 0,
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

    inline LINE_STATUS parse_line()
    {
        int i = 1;
        if (m_read_buffer.ReadableBytes() < 2)
        {
            return LINE_OPEN;
        }
        if (*(m_read_buffer.Peek()) == '\n')
        {
            return LINE_BAD;
        }
        while (i < m_read_buffer.ReadableBytes())
        {
            if (*(m_read_buffer.Peek() + i) == '\n' && *(m_read_buffer.Peek() + i - 1) == '\r')
            {
                return LINE_OK;
            }
            else if (*(m_read_buffer.Peek() + i) == '\n')
            {
                return LINE_BAD;
            }
            else if (*(m_read_buffer.Peek() + i - 1) == '\r')
            {
                return LINE_BAD;
            }
            i++;
        }

        return LINE_OPEN;
    }

    inline HTTP_CODE parse_request_line()
    {
        int i = 0;
        char method[20] = {0};
        // if (parse_line() != LINE_OK)
        // {
        //     return NO_REQUEST;
        // }
        while (i < 20 && *(i + m_read_buffer.Peek()) != ' ' && i < m_read_buffer.ReadableBytes())
        {
            method[i] = *(i + m_read_buffer.Peek());
            i++;
        }
        if (strcasecmp(method, "GET") == 0)
        {
            m_method = GET;
        }
        else if (strcasecmp(method, "PUT") == 0)
        {
            m_method = PUT;
        }
        else
        {
            return BAD_REQUEST;
        }
        i++;
        while (*(i + m_read_buffer.Peek()) != ' ' && i < m_read_buffer.ReadableBytes())
        {
            m_url.push_back(*(i + m_read_buffer.Peek()));
            i++;
        }
        if (m_url.empty())
        {
            return BAD_REQUEST;
        }
        if (m_url.back() == '/')
        {
            m_url.append("htdocs/targetfile.txt");
            m_path = m_url;
        }
        i++;
        while (*(i + m_read_buffer.Peek()) != '\r' && i < m_read_buffer.ReadableBytes())
        {
            m_version.push_back(*(i + m_read_buffer.Peek()));
            i++;
        }
        if (m_version.empty())
        {
            return BAD_REQUEST;
        }
        while (*(i + m_read_buffer.Peek()) != '\n')
        {
            i++;
        }
        m_read_buffer.Retrieve(i + 1);
        // switch (m_method)
        // {
        // case GET:
        //     return GET_REQUEST;
        //     break;

        // default:
        //     break;
        // }

        m_check_state = CHECK_STATE_HEADER;
        return NO_REQUEST;
    }

    inline HTTP_CODE parse_header()
    {
        int i = 0;
        std::string header, value;
        if (*(i + m_read_buffer.Peek()) == '\r' && *(1 + i + m_read_buffer.Peek()) == '\n')
        {
            m_check_state = CHECK_STATE_CONTENT;
            m_read_buffer.Retrieve(2);
            return NO_REQUEST;
        }
        while (*(i + m_read_buffer.Peek()) != ':' && i < m_read_buffer.ReadableBytes())
        {
            header.push_back(*(i + m_read_buffer.Peek()));
            i++;
        }
        i++;
        while (*(i + m_read_buffer.Peek()) == ' ')
        {
            i++;
        }
        while (*(i + m_read_buffer.Peek()) != '\r' && *(1 + i + m_read_buffer.Peek()) != '\n' && i + 1 < m_read_buffer.ReadableBytes())
        {
            value.push_back(*(i + m_read_buffer.Peek()));
            i++;
        }
        // cout << "file: " << header << ' ' << value << "  " << endl;

        if (header.empty() || value.empty())
        {
            return BAD_REQUEST;
        }
        m_headers[header] = value;
        m_read_buffer.Retrieve(i + 2);
        // cout << "file: " << *m_read_buffer.Peek() << endl;
        return NO_REQUEST;
    }
    inline LINE_STATUS parse_content()
    {
        if (m_headers.find("Content-length") == m_headers.end() || m_headers.find("Content-Length") == m_headers.end() || m_headers.find("content-length") == m_headers.end())
        {
            return LINE_OK;
        }
        int length = std::stoi(m_headers["Content-length"]);
        if (m_read_buffer.ReadableBytes() < length)
        {
            return LINE_OPEN;
        }
        return LINE_OK;
    }

    inline void read()
    {
        int ErrorCode = 0;
        if (m_read_buffer.ReadFd(m_socket_fd, &ErrorCode) == 0)
        {
            m_no_more_requests = true;
        };
    }
    inline bool write()
    {
        int ErrorCode = 0;
        int len = m_write_buffer.WriteFd(m_socket_fd, &ErrorCode);
        if (m_write_buffer.ReadableBytes() > 0)
        {
            return false;
        }
        return true;
    };

    inline HTTP_CODE parse_http()
    {
        LINE_STATUS line_status = LINE_OK;
        while ((m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = parse_line()) == LINE_OK))
        {
            line_status = LINE_OK;
            switch (m_check_state)
            {
            case CHECK_STATE_REQUEST_LINE:
                if (parse_request_line() == BAD_REQUEST)
                {
                    return BAD_REQUEST;
                }
                break;
            case CHECK_STATE_HEADER:
                if (parse_header() == BAD_REQUEST)
                {
                    return BAD_REQUEST;
                }
                break;
            case CHECK_STATE_CONTENT:
                if (parse_content() != LINE_OK)
                {
                    return NO_REQUEST;
                }
                else
                {
                    return GET_REQUEST;
                }
                break;
            default:
                return INTERNAL_ERROR;
            }
        }
        return NO_REQUEST;
    }

    inline void not_found()
    {
        // cout << "file: " << m_path << " not found" << endl;
        m_write_buffer.Append("HTTP/1.1 404 NOT FOUND\r\n");
        m_write_buffer.Append("Server: jdbhttpd/0.1.0\r\n");
        m_write_buffer.Append("Content-Type: text/html\r\n");
        m_write_buffer.Append("Content-length: 169\r\n");
        m_write_buffer.Append("\r\n");
        m_write_buffer.Append("<HTML><TITLE>Not Found</TITLE>\r\n");
        m_write_buffer.Append("<BODY><P>The server could not fulfill\r\n");
        m_write_buffer.Append("your request because the resource specified\r\n");
        m_write_buffer.Append("is unavailable or nonexistent.\r\n");
        m_write_buffer.Append("</BODY></HTML>\r\n");
    }

    inline void bad_request()
    {
        m_write_buffer.Append("HTTP/1.1 400 BAD REQUEST\r\n");
        m_write_buffer.Append("Server: jdbhttpd/0.1.0\r\n");
        m_write_buffer.Append("Content-type: text/html\r\n");
        m_write_buffer.Append("Content-length: 80\r\n");
        m_write_buffer.Append("\r\n");
        m_write_buffer.Append("<P>Your browser sent a bad request, ");
        m_write_buffer.Append("such as a POST without a Content-Length.\r\n");
    }
    inline void add_good_request()
    {
    }
    inline void add_headers()
    {
        m_write_buffer.Append("HTTP/1.0 200 OK\r\n");
        m_write_buffer.Append("Server: jdbhttpd/0.1.0\r\n");
        m_write_buffer.Append("Content-Type: text/html\r\n");
    }
    inline void add_content_length(int length)
    {
        std::string len = std::to_string(length);
        m_write_buffer.Append(std::string("Content-length: ") + len + "\r\n");
        m_write_buffer.Append("\r\n");
    }
    inline void add_content(FILE *resource)
    {
        char buf[4096];
        // fgets(buf, sizeof(buf), resource);
        while (!feof(resource))
        {
            fgets(buf, sizeof(buf), resource);
            m_write_buffer.Append(buf, sizeof(buf));
        }
    }

    void serve_file(const char *filename)
    {
        FILE *resource = NULL;
        resource = fopen(filename, "r");

        if (resource == NULL)
            not_found();
        else
        {
            struct stat statbuf;
            stat(filename, &statbuf);
            int size = statbuf.st_size;
            add_headers();
            add_content_length(size);
            add_content(resource);
            fclose(resource);
        }
    }

    inline HTTP_CODE do_request()
    {
        m_http_code = parse_http();
        switch (m_http_code)
        {
        case BAD_REQUEST:
            bad_request();
            return BAD_REQUEST;
        case NO_REQUEST:
            return NO_REQUEST;
        default:
            break;
        }

        switch (m_method)
        {
        case GET:
            serve_file(m_path.c_str());
            break;

        default:
            break;
        }
        return NO_REQUEST;
    }

    inline void process()
    {
        read();
        do_request();

        while (!write())
        {
        }
        if (m_no_more_requests)
        {
            close();
        }
    }
    void close();
    inline void reset()
    {
        m_read_buffer.Clean();
        m_write_buffer.Clean();
        m_method = NONE;
        m_socket_fd = -1;
        m_epoll_fd = -1;
        m_check_state = CHECK_STATE_REQUEST_LINE;
        m_http_code = NO_REQUEST;
        m_no_more_requests = false;
    }

    std::weak_ptr<Entry> m_entry;

private:
    Buffer m_read_buffer;
    Buffer m_write_buffer;
    int m_socket_fd;
    int m_epoll_fd;
    METHOD m_method;
    CHECK_STATE m_check_state;
    HTTP_CODE m_http_code;
    std::unordered_map<std::string, std::string> m_headers;

    std::string m_url;
    std::string m_path;
    std::string m_version;
    std::string m_content;
    bool m_no_more_requests;
    static Http_connection dummy;
};
#endif