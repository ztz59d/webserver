#ifndef ENTRY_H
#define ENTRY_H
// #include "http_conn.h"
#include "http_conn.hpp"

class Http_connection;
class Entry
{
public:
    Http_connection *_conn;
    Entry(Http_connection *conn) : _conn(conn){};
    ~Entry();
    void reset(Http_connection *conn);
};

#endif