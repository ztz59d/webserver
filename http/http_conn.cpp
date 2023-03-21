#include "http_conn.hpp"
void Http_connection::close()
{
    if (m_socket_fd == -1)
    {
        return;
    }
    std::shared_ptr<Entry> entry_ptr = m_entry.lock();
    if (entry_ptr)
    {
        entry_ptr->reset(&dummy);
        std::cout << "connection closed" << std::endl;
    }
    reset();
};
Http_connection Http_connection::dummy(-1, -1);