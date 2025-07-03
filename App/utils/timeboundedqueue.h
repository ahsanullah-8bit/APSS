#pragma once

#include "tbb_patched.h"

template<typename T>
class TimeBoundedQueue : public tbb::concurrent_bounded_queue<T> {
public:
    TimeBoundedQueue();
    virtual ~TimeBoundedQueue();
    bool try_push(const T &value, std::chrono::milliseconds timeout, unsigned int sleepInterval = 5);
    bool try_push(T &&value, std::chrono::milliseconds timeout, unsigned int sleepInterval = 5);
    bool try_push(const T &value, unsigned int timeout, unsigned int sleepInterval = 5);
    bool try_push(T &&value, unsigned int timeout, unsigned int sleepInterval = 5);

    bool try_pop(T &result, std::chrono::milliseconds timeout, unsigned int sleepInterval = 5);
    bool try_pop(T &result, unsigned int timeout, unsigned int sleepInterval = 5);
};

// Definitions
template<typename T>
inline TimeBoundedQueue<T>::TimeBoundedQueue()
    : tbb::concurrent_bounded_queue<T>()
{}

template<typename T>
inline TimeBoundedQueue<T>::~TimeBoundedQueue()
{}

template<typename T>
inline bool TimeBoundedQueue<T>::try_push(const T &value, std::chrono::milliseconds timeout, unsigned int sleepInterval)
{
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (tbb::concurrent_bounded_queue<T>::try_push(value))
            return true;

        if (sleepInterval)
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepInterval));
    }
    return false;
}

template<typename T>
inline bool TimeBoundedQueue<T>::try_push(T &&value, std::chrono::milliseconds timeout, unsigned int sleepInterval)
{
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (tbb::concurrent_bounded_queue<T>::try_push(std::move(value)))
            return true;

        if (sleepInterval)
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepInterval));
    }
    return false;
}

template<typename T>
inline bool TimeBoundedQueue<T>::try_push(const T &value, unsigned int timeout, unsigned int sleepInterval)
{
    return try_push(value, std::chrono::milliseconds(timeout), sleepInterval);
}

template<typename T>
inline bool TimeBoundedQueue<T>::try_push(T &&value, unsigned int timeout, unsigned int sleepInterval)
{
    return try_push(std::forward(value), std::chrono::milliseconds(timeout), sleepInterval);
}

template<typename T>
inline bool TimeBoundedQueue<T>::try_pop(T &result, std::chrono::milliseconds timeout, unsigned int sleepInterval)
{
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (tbb::concurrent_bounded_queue<T>::try_pop(result)) {
            return true;
        }

        if (sleepInterval)
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepInterval));
    }
    return false;
}

template<typename T>
inline bool TimeBoundedQueue<T>::try_pop(T &result, unsigned int timeout, unsigned int sleepInterval)
{
    return try_pop(result, std::chrono::milliseconds(timeout), sleepInterval);
}
