#include "entry.h"

Entry::~Entry()
{
    _conn->close_conn();
    // delete _conn;
}