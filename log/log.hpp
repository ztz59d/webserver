#ifndef _LOG_H_
#define _LOG_H_
#include <string>
#include <list>
#include <mutex>
#include <condition_variable>
#include <iostream>
class log
{
private:
    std::list<std::string> m_wait_queue;
    std::list<std::string> m_disk_queue;
    std::mutex m_mutex;
    std::condition_variable cond;
    FILE *file;

public:
    log(char *f) : file(fopen(f, "ab"))
    {
        if (file == NULL)
        {
            std::cout << "file open failed" << std::endl;
            exit(1);
        }
    };
    ~log() = default;

    inline void LOG(std::string &&line)
    {
        {
            std::unique_lock<std::mutex> guard(m_mutex);
            m_wait_queue.emplace_back(std::forward<std::string>(line));
        }
        cond.notify_all();
    }
    inline void flush()
    {
        for (;;)
        {
            while (!m_disk_queue.empty())
            {
                fwrite(m_disk_queue.front().c_str(), 1, m_disk_queue.front().size() + 1, file);
                // std::cout << m_disk_queue.front().size() << std::endl;
                // std::cout << sizeof(m_disk_queue.front().c_str()) << std::endl;
                // std::cout << m_disk_queue.front() << std::endl;
                // fflush(file);
                m_disk_queue.pop_front();
            }
            fflush(file);
            {
                std::unique_lock<std::mutex> guard(m_mutex);
                m_disk_queue.splice(m_disk_queue.begin(), m_wait_queue);
                if (m_disk_queue.empty())
                {
                    cond.wait(guard);
                }
            }
        }
    }
};

#endif