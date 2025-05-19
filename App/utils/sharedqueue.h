/*
    Copyright (c) 2024-2025 Abdalrahman M. Amer
    Copyright (c) 2025 APSS-Official

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#pragma once

#include <tbb_patched.h>

#include <atomic>

#include <QSemaphore>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>

// A thread-safe producer-consumer queue
template <typename T>
class SharedQueue
{
public:
    SharedQueue(size_t bufferSize)
        : m_freeSpace(bufferSize)
        , m_usedSpace(0)
        , m_bufferSize(bufferSize)
    {}

    bool produce(const T& item)
    {
        if (finished())
            return false;

        m_freeSpace.acquire();
        m_buffer.emplace(item);
        m_usedSpace.release();

        return true;
    }

    bool consume(T& item)
    {
        // End of consumption. Otherwise, consume till empty
        if (finished() && m_buffer.empty())
            return false;

        m_usedSpace.acquire();
        if (!m_buffer.try_pop(item)) {
            qCritical() << "Failed to pop after acquiring used space - this should not happen under normal circumstances.";
            m_usedSpace.release();
            return false;
        }

        m_freeSpace.release();
        return true;
    }

    void finish()
    {
        m_finished = true;
        // Producer signals no more items will be produced.
        // Consumers will eventually empty the queue and then `consume()` will return false.
    }

    bool finished() const
    {
        return m_finished.load();
    }

    bool empty()
    {
        return m_buffer.empty();
    }

private:
    size_t m_bufferSize;
    tbb::concurrent_queue<T> m_buffer;
    std::atomic<bool> m_finished = false;

    QSemaphore m_freeSpace;
    QSemaphore m_usedSpace;
};
