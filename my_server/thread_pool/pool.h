#ifndef POOL_H
#define POOL_H

#include <mutex>
#include <memory>
#include <functional>
#include <list>
#include <mutex>
#include <condition_variable>
#include <thread>

class Pool
{
private:
    std::list<std::function<void()>> queue;
    std::mutex mu;
    std::condition_variable con;
    std::unique_lock<std::mutex> ulock;

    int threads;

public:
    Pool(int num = 10);
    ~Pool() = default;

    void init();
    void threading();
    void add_task(std::function<void()> &&task);
};

Pool::Pool(int num) : ulock(mu), threads(num)
{
}

void Pool::init()
{
    for (int i = 0; i < threads; i++)
    {
        std::thread t(&Pool::threading, this);
        t.detach();
    }
}
void Pool::threading()
{
    for (;;)
    {
        ulock.lock();
        if (queue.empty())
        {
            con.wait(ulock);
            ulock.unlock();
        }

        else
        {
            std::function<void()> task = std::move(queue.front());
            queue.pop_front();
            ulock.unlock();
            task();
        }
    }
}

void Pool::add_task(std::function<void()> &&task)
{
    ulock.lock();
    queue.emplace_back(std::forward<std::function<void()>>(task));
    ulock.unlock();
    con.notify_one();
}
#endif