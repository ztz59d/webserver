#ifndef ENTRY_H
#define ENTRY_H
// #include "http_conn.h"
#include "http_conn.h"
class Http_conn;
class Entry
{
public:
    Http_conn *_conn;
    Entry(Http_conn *conn) : _conn(conn){};
    ~Entry();
};

#endif