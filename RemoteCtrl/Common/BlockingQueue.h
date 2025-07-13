#pragma once

#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>

template <typename T>
class BlockingQueue
{
public:
	BlockingQueue() : isRelease(false) {}
	~BlockingQueue()
	{
		Release();
	}

	BlockingQueue(const BlockingQueue&) = delete;
	BlockingQueue(BlockingQueue&&) = delete;
	BlockingQueue& operator=(const BlockingQueue&) = delete;
	BlockingQueue& operator=(BlockingQueue&&) = delete;

	bool pop(T& outElem);
    void push(T&& elem);
    void push(const T& elem);
	void Blocking();
	void Release();
	void Clear();

private:
	std::mutex popMtx;
	std::queue<T> popQue;
	std::mutex pushMtx;
	std::queue<T> pushQue;
	std::condition_variable noEmpty;
	std::atomic<bool> isRelease;
	size_t SwapQueue();
};

template <typename T>
bool BlockingQueue<T>::pop(T& outElem)
{
	std::lock_guard<std::mutex> popLock(popMtx);
	if (popQue.empty() && SwapQueue() == 0) {
		return false;
	}
	outElem = std::move(popQue.front());
	popQue.pop();
	return true;
}

template <typename T>
void BlockingQueue<T>::push(T&& elem)
{
    std::lock_guard<std::mutex> pushLock(pushMtx);
    pushQue.push(std::move(elem));
    noEmpty.notify_one();
}

template <typename T>
void BlockingQueue<T>::push(const T& elem)
{
    std::lock_guard<std::mutex> pushLock(pushMtx);
    pushQue.push(elem);
    noEmpty.notify_one();
}

template <typename T>
size_t BlockingQueue<T>::SwapQueue()
{
	std::unique_lock<std::mutex> pushUniLock(pushMtx);
	noEmpty.wait(pushUniLock, [this]() {
		return !pushQue.empty() || isRelease;
	});
	if (isRelease) { ; }
	std::swap(popQue, pushQue);
	return popQue.size();
}

template <typename T>
void BlockingQueue<T>::Blocking()
{
	isRelease = false;
}

template <typename T>
void BlockingQueue<T>::Release()
{
	isRelease = true;
	noEmpty.notify_all();
}

template <typename T>
void BlockingQueue<T>::Clear()
{
    Release();
    std::lock_guard<std::mutex> popLock(popMtx);
    std::lock_guard<std::mutex> pushLock(pushMtx);
    {
        std::queue<T> empty;
        std::swap(popQue, empty);
    }
    {
        std::queue<T> empty;
        std::swap(pushQue, empty);
    }
    Blocking();
}

