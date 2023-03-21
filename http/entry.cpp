#include "entry.h"

Entry::~Entry()
{
    _conn->close();
    // delete _conn;
}
void Entry::reset(Http_connection *conn)
{
    _conn = conn;
}
