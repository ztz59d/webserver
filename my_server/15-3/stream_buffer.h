#ifndef __STREAM_BUFFER_H__
#define __STREAM_BUFFER_H__
#include <deque>
using std::deque;
class StreamBuffer
{
public:
    StreamBuffer(){};
    ~StreamBuffer(){};
    inline void clear()
    {
        m_buffer.clear();
        m_size = 0;
        m_curpos = 0;
    }
    int m_curpos;
    int m_size;

private:
    deque<char> m_buffer;
};
#endif