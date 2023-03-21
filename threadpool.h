#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include <thread>
#include "locker.h"
#include <vector>
#include <iostream>
#include <memory>
template <typename T>
class ThreadPool
{
public:
    ThreadPool(int thread_num = 8, int max_requests = 10000);
    ~ThreadPool();
    bool append(std::weak_ptr<Entry> request);
    static void *worker(void *arg);
    void run();

private:
    int m_thread_number;
    int m_max_requests;
    std::vector<std::shared_ptr<std::thread>> m_threads;
    std::list<T *> m_work_queue;
    std::mutex m_queue_locker;
    // std::unique_lock<std::mutex> lock;
    // Sem m_queue_state;
    std::condition_variable cond;
    bool m_stop;
};

template <typename T>
ThreadPool<T>::ThreadPool(int thread_num, int max_requests) : m_thread_number(thread_num), m_max_requests(max_requests), m_stop(false)
{
    if ((thread_num <= 0) || (max_requests <= 0))
    {
        throw "thread and request number must be greater than 0";
    }
    for (int i = 0; i < thread_num; i++)
    {
        std::cout << "creating the " + std::to_string(i) + "th thread" << std::endl;

        // 这里的thread可以用std::thread t(&ThreadPool::run, this);
        m_threads.emplace_back(std::make_shared<std::thread>(std::thread(worker, this)));
    }
}

template <typename T>
ThreadPool<T>::~ThreadPool()
{
    // 线程没有detach之后怎么结束呢
    for (auto &thread : m_threads)
    {
        thread->~thread();
    }
    m_stop = true;
}

template <typename T>
bool ThreadPool<T>::append(std::weak_ptr<Entry> request)
{
    m_queue_locker.lock();
    if (m_work_queue.size() >= m_max_requests)
    {
        m_queue_locker.unlock();
        return false;
    }
    m_work_queue.emplace_back(request);
    m_queue_locker.unlock();
    cond.notify_one();

    return true;
}

template <typename T>
void *ThreadPool<T>::worker(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;
    pool->run();
    return pool;
}

template <typename T>
void ThreadPool<T>::run()
{
    while (!m_stop)
    {
        T *request = nullptr;
        {
            std::unique_lock<std::mutex> lock(m_queue_locker);
            if (m_work_queue.empty())
            {
                cond.wait(lock);
                continue;
            }
            request = m_work_queue.front();
            m_work_queue.pop_front();
            if (request == nullptr)
            {
                continue;
            }
        }
        request->process();
    }
}

#endif