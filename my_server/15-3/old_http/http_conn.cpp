#include "http_conn.h"

const char *ok_200_title = "OK";
const char *error_400_title = "Bda Request";
const char *error_400_form = "your request has bad syntas or is inherently invalid";
const char *error_403_title = "Forbidden";
const char *error_403_form = "Access denied";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested page does not exist";
const char *error_500_tile = "Internal Error";
const char *error_500_form = "There has been an unusual error serving the requested file";

const char *doc_root = "/var/www/html";

int set_nonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd, bool one_shot)
{
    epoll_event event;
    event.data.fd = fd;

    // 注意epoll的参数
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if (one_shot)
    {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    set_nonblocking(fd);
}

void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void modfd(int epollfd, int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int Http_conn::m_user_count = 0;
// int Http_conn::epoll_fd = -1;

void Http_conn::close_conn(bool real_close)
{
    if (real_close && (m_socket_fd != -1))
    {
        removefd(epoll_fd, m_socket_fd);
        m_socket_fd = -1;
        m_user_count--;
    }
}

void Http_conn::init(int socket_fd, const sockaddr_in &addr)
{
    m_socket_fd = socket_fd;
    m_address = addr;

    // 防止time_wait状态
    int reuse = 1;
    setsockopt(m_socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    addfd(epoll_fd, m_socket_fd, true);
    m_user_count++;

    init(m_socket_fd);
}
void Http_conn::init(int sockfd)
{
    m_check_state = CHECK_STATE_REQUEST_LINE;
    m_linger = false;

    m_socket_fd = sockfd;
    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
}

Http_conn::LINE_STATUS Http_conn::parse_line()
{
    char temp;
    for (; m_checked_idx < m_read_idx; m_checked_idx++)
    {
        temp = m_read_buf[m_checked_idx];
        if (temp == '\r')
        {
            if (m_checked_idx + 1 == m_read_idx)
            {
                return LINE_OPEN;
            }
            if (m_read_buf[m_checked_idx + 1] == '\n')
            {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n')
        {
            if ((m_checked_idx > 1) && (m_read_buf[m_checked_idx - 1] == 'r'))
            {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

bool Http_conn::read()
{
    if (m_read_idx >= READ_BUFFER_SIZE)
    {
        return false;
    }
    int bytes_read = 0;
    for (;;)
    {
        bytes_read = recv(m_socket_fd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        if (bytes_read == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
        }
        else if (bytes_read == 0)
        {
            return false;
        }
        m_read_idx += bytes_read;
    }
    return true;
}

Http_conn::HTTP_CODE Http_conn::parse_request_line(char *text)
{

    m_url = strpbrk(text, " '\t");
    if (!m_url)
    {
        return BAD_REQUEST;
    }
    *m_url++ = '\0';

    char *method = text;
    if (strcasecmp(method, "GET") == 0)
    {
        m_method = GET;
    }
    else
    {
        return BAD_REQUEST;
    }

    m_url += strspn(m_url, " \t");
    m_version = strpbrk(m_url, " \t");
    if (!m_version)
    {
        return BAD_REQUEST;
    }
    *m_url++ = '\0';
    m_version += strspn(m_version, " \t");
    if (strcasecmp(m_version, "HTTP/1.1") != 0)
    {
        return BAD_REQUEST;
    }

    if (strncasecmp(m_url, "HTTP://", 7) == 0)
    {
        m_url += 7;
        m_url = strchr(m_url, '/');
    }
    if (!m_url || m_url[0] != '/')
    {
        return BAD_REQUEST;
    }

    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

Http_conn::HTTP_CODE Http_conn::parse_header(char *text)
{
    if (text[0] == '\0')
    {
        if (m_content_length != 0)
        {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }

    else if (strncasecmp(text, "Connection:", 11) == 0)
    {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0)
        {
            m_linger = true;
        }
    }

    else if (strncasecmp(text, "Content-Length", 15) == 0)
    {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    }

    else if (strncasecmp(text, "Host:", 5) == 0)
    {
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    }

    else
    {
        printf("Oops unknown header %s\n", text);
    }

    return NO_REQUEST;
}

Http_conn::HTTP_CODE Http_conn::parse_content(char *text)
{
    if (m_read_idx >= (m_content_length + m_checked_idx))
    {
        text[m_read_idx] = '\0';
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

Http_conn::HTTP_CODE Http_conn::process_read()
{
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;

    while (((m_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK)) || ((line_status = parse_line()) == LINE_OK))
    {
        text = get_line();
        m_start_line = m_checked_idx;
        printf("got 1 http line :%s\n", text);

        switch (m_check_state)
        {
        case CHECK_STATE_REQUEST_LINE:
        {
            ret = parse_request_line(text);
            if (ret == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            break;
        }

        case (CHECK_STATE_HEADER):
        {
            ret = parse_request_line(text);
            if (ret == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            else if (ret == GET_REQUEST)
            {
                return do_request();
            }
            break;
        }

        case (CHECK_STATE_CONTENT):
        {
            ret = parse_request_line(text);
            if (ret == GET_REQUEST)
            {
                return do_request();
            }
            line_status = LINE_OPEN;
            break;
        }

        default:
        {
            return INTERNAL_ERROR;
        }
        }
    }
    return NO_REQUEST;
}

Http_conn::HTTP_CODE Http_conn::do_request()
{
    strcpy(m_real_file, doc_root);
    int len = strlen(doc_root);
    strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);
    if (stat(m_real_file, &m_file_stat) < 0)
    {
        return NO_REQUEST;
    }
    if (!(m_file_stat.st_mode & S_IROTH))
    {
        return FORBIDDEN_REQUEST;
    }
    if (S_ISDIR(m_file_stat.st_mode))
    {
        return BAD_REQUEST;
    }
    int fd = open(m_real_file, O_RDONLY);
    m_file_address = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;
}

void Http_conn::unmap()
{
    if (m_file_address)
    {
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = 0;
    }
}

bool Http_conn::write()
{
    int temp = 0;
    int bytes_have_sent = 0;
    int bytes_to_sent = m_write_idx;
    if (bytes_to_sent == 0)
    {
        modfd(epoll_fd, m_socket_fd, EPOLLIN);
        init(m_socket_fd);
        return true;
    }
    for (;;)
    {
        temp = writev(m_socket_fd, m_iv, m_iv_count);
        if (temp <= -1)
        {
            if (errno = EAGAIN)
            {
                modfd(epoll_fd, m_socket_fd, EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }

        bytes_to_sent -= temp;
        bytes_have_sent += temp;
        if (bytes_to_sent <= bytes_have_sent)
        {
            unmap();
            if (m_linger)
            {
                init(m_socket_fd);
                modfd(epoll_fd, m_socket_fd, EPOLLIN);
                return true;
            }
            else
            {
                modfd(epoll_fd, m_socket_fd, EPOLLIN);
                return false;
            }
        }
    }
}

bool Http_conn::add_response(const char *format, ...)
{
    if (m_read_idx >= WRITE_BUFFER_SIZE)
    {
        return false;
    }
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx))
    {
        return false;
    }
    m_write_idx += len;
    va_end(arg_list);
    return true;
}

bool Http_conn::add_status_line(int status, const char *title)
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool Http_conn::add_headers(int content_len)
{
    add_content_length(content_len);
    add_linger();
    add_blank_line();
}

bool Http_conn::add_content_length(int content_len)
{
    return add_response("Content-Length: %d\r\n", content_len);
}

bool Http_conn::add_linger()
{
    return add_response("Connection: %S\r\n", (m_linger == true) ? "keep-alive" : "close");
}

bool Http_conn::add_blank_line()
{
    return add_response(" %s", "\r\n");
}

bool Http_conn::add_content(const char *content)
{
    return add_response("%s", content);
}

bool Http_conn::process_write(HTTP_CODE ret)
{
    switch (ret)
    {
    case INTERNAL_ERROR:
    {
        add_status_line(500, error_500_tile);
        add_headers(strlen(error_500_form));
        if (!add_content(error_500_form))
        {
            return false;
        }
        break;
    }

    case BAD_REQUEST:
    {
        add_status_line(400, error_400_title);
        add_headers(strlen(error_400_form));
        if (!add_content(error_400_form))
        {
            return false;
        }
        break;
    }

    case NO_RESOURCES:
    {
        add_status_line(404, error_404_title);
        add_headers(strlen(error_404_form));
        if (!add_content(error_404_form))
        {
            return false;
        }
        break;
    }

    case FORBIDDEN_REQUEST:
    {
        add_status_line(403, error_403_title);
        add_headers(strlen(error_403_form));
        if (!add_content(error_403_form))
        {
            return false;
        }
        break;
    }

    case FILE_REQUEST:
    {
        add_status_line(200, ok_200_title);
        if (m_file_stat.st_size != 0)
        {
            add_headers(m_file_stat.st_size);
            m_iv[0].iov_base = m_write_buf;
            m_iv[0].iov_len = m_write_idx;
            m_iv[1].iov_base = m_file_address;
            m_iv[1].iov_len = m_file_stat.st_size;
            m_iv_count = 2;
            return true;
        }

        else
        {
            const char *ok_string = "<html><body></body></html>";
            add_headers(strlen(ok_string));
            if (!add_content(ok_string))
            {
                return false;
            }
        }
    }

    default:
        return false;
    }

    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    return true;
}

void Http_conn::process()
{
    HTTP_CODE read_ret = process_read();
    if (read_ret == NO_REQUEST)
    {
        modfd(epoll_fd, m_socket_fd, EPOLLIN);
        return;
    }

    bool write_ret = process_write(read_ret);
    if (!write_ret)
    {
        close_conn();
    }

    modfd(epoll_fd, m_socket_fd, EPOLLOUT);
}