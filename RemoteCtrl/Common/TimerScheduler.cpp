#include "pch.h"
#include "TimerScheduler.h"

#include <thread>

TimerScheduler::TimerScheduler()
    : isRelease(false)
{
    worker = std::thread([this]() {dealTimerTask(); });
}

TimerScheduler::~TimerScheduler()
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        isRelease = true;
        cv.notify_one();
    }
    if (worker.joinable()) { worker.join(); }
}

size_t TimerScheduler::post(const std::function<void()>& task, int delayMS)
{
    size_t id = nextId.fetch_add(1);
    auto time = std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMS);
    {
        std::lock_guard<std::mutex> lock(mtx);
        taskPQ.push(TimerTask(id, task, time));
    }
    cv.notify_one();
    return id;
}

size_t TimerScheduler::post(std::function<void()>&& task, int delayMS)
{
    size_t id = nextId.fetch_add(1);
    auto time = std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMS);
    {
        std::lock_guard<std::mutex> lock(mtx);
        taskPQ.push(TimerTask(id, std::move(task), time));
    }
    cv.notify_one();
    return id;
}

void TimerScheduler::Cancel(size_t taskID)
{
    std::lock_guard<std::mutex> lock(mtx);
    cancelled.insert(taskID);
    cv.notify_one();
}

void TimerScheduler::dealTimerTask()
{
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);

        while (!isRelease && (taskPQ.empty() || taskPQ.top().time > std::chrono::steady_clock::now())) {
            if (taskPQ.empty()) {
                cv.wait(lock);
            }
            else {
                cv.wait_until(lock, taskPQ.top().time);
            }
        }

        if (isRelease) { break; }

        TimerTask task = taskPQ.top();
        taskPQ.pop();

        if (cancelled.find(task.taskID) != cancelled.end()) {
            cancelled.erase(task.taskID);
            continue;
        }

        lock.unlock();
        threadPool.post(std::move(task.task));
    }
}

void TimerScheduler::Clear()
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        isRelease = true;
        cv.notify_one();
    }
    if (worker.joinable()) { worker.join(); }

    decltype(taskPQ) tempPQ;
    taskPQ.swap(tempPQ);
    threadPool.Clear();

    isRelease = false;
    worker = std::thread([this]() {dealTimerTask(); });
}
