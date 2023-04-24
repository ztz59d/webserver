#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <thread>
#include <semaphore.h>
#include <mutex>
#include <condition_variable>

class Sem
{
private:
    sem_t m_sem;

public:
    Sem(int i = 0)
    {
        sem_init(&m_sem, 0, i);
    }
    ~Sem()
    {
        sem_destroy(&m_sem);
    }
    bool wait()
    {
        return sem_wait(&m_sem);
    }
    bool post()
    {
        return sem_post(&m_sem);
    }
};

class Locker
{
public:
    void lock()
    {
        m_mutex.lock();
    }
    void unlock()
    {
        m_mutex.unlock();
    }

private:
    std::mutex m_mutex;
};

class Cond
{
private:
    std::condition_variable m_cond;
    std::mutex m_mutex;

public:
    void wait()
    {
        std::unique_lock<std::mutex> guard(m_mutex);
        m_cond.wait(guard);
    }

    void signal()
    {
        m_cond.notify_one();
    }
};
#endif