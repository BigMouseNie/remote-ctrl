#pragma once

#include "ThreadPool.h"

#include <functional>
#include <chrono>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_set>

class TimerScheduler
{
public:
    TimerScheduler();
    ~TimerScheduler();

    size_t post(const std::function<void()>& task, int delayMS);
    size_t post(std::function<void()>&& task, int delayMS);
    void Cancel(size_t taskID);
    void Clear();

private:
    struct TimerTask {
        size_t taskID;
        std::function<void()> task;
        std::chrono::steady_clock::time_point time;
        TimerTask(size_t taskID, const std::function<void()>& task, std::chrono::steady_clock::time_point time) :
            taskID(taskID), task(task), time(time) {}

        bool operator> (const TimerTask& other) const {
            return time > other.time;
        }
    };

    std::atomic<size_t> nextId = 1;
    std::mutex mtx;
    std::priority_queue<TimerTask, std::vector<TimerTask>, std::greater<>> taskPQ;
    std::unordered_set<size_t> cancelled;

    std::condition_variable cv;
    bool isRelease;
    ThreadPool threadPool;
    std::thread worker;
    void dealTimerTask();
};
