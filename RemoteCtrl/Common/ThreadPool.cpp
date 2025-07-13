#include "pch.h"
#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threadNumer)
    : threadResouce(threadNumer), threadNum(threadNumer), isRelease(false)
{
    for (size_t i = 0; i < threadNumer; ++i) {
        workers.push_back(std::thread([this]() {work(); }));
    }
}

ThreadPool::~ThreadPool()
{
    isRelease = true;
    que.Release();
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::acquire()
{
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&]() { return threadResouce > 0 || isRelease; });
    --threadResouce;
}

void ThreadPool::release()
{
    std::lock_guard<std::mutex> lock(mtx);
    ++threadResouce;
    cv.notify_one();
}

void ThreadPool::post(const std::function<void()>& func)
{
    que.push(func);
}

void ThreadPool::post(std::function<void()>&& func)
{
    que.push(std::move(func));
}

void ThreadPool::work()
{
    while (!isRelease) {
        acquire();
        if (isRelease) { break; }
        while (true) {
            std::function<void()> func;
            if (que.pop(func)) {
                func();
                continue;
            }
            break;
        }
    }
}

void ThreadPool::Clear()
{
    que.Clear();
    threadResouce = threadNum;
    cv.notify_all();
}
