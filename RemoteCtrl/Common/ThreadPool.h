#pragma once

#include "BlockingQueue.h"

#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPool
{
public:
    explicit ThreadPool(size_t threadNumer = 2);
    ~ThreadPool();

    void post(const std::function<void()>& func);
    void post(std::function<void()>&& func);
    void Clear();

private:
    void work();
    void acquire();
    void release();

private:
    BlockingQueue<std::function<void()>> que;
    std::vector<std::thread> workers;

    std::mutex mtx;
    std::condition_variable cv;
    size_t threadNum;
    size_t threadResouce;
    std::atomic<bool> isRelease;

};
