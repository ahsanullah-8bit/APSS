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

#include <QSemaphore>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>
#include <QWaitCondition>
#include <QQueue>

// A thread-safe producer-consumer queue using Wait Conditions
template <typename T>
class SharedQueue
{
public:
    SharedQueue(size_t bufferSize)
        : m_bufferSize(bufferSize)
    {}

    bool produce(const T& item)
    {
        {
            const QMutexLocker lock(&m_mtx);
            while (m_usedSpace == m_bufferSize)
                bufferNotFull.wait(&m_mtx);
        }

        if (finished())
            return false;

        m_buffer.enqueue(item);

        {
            const QMutexLocker lock(&m_mtx);
            m_usedSpace++;
            bufferNotEmpty.wakeAll();
        }

        return true;
    }

    bool consume(T& item)
    {
        // End of consumption. Otherwise, consume till empty
        if (finished() && m_buffer.empty())
            return false;

        {
            const QMutexLocker locker(&m_mtx);
            while (m_usedSpace <= 0)
                bufferNotEmpty.wait(&m_mtx);
        }

        if (m_buffer.empty())
            return false;

        item = std::move(m_buffer.dequeue());
        {
            const QMutexLocker locker(&m_mtx);
            --m_usedSpace;
            bufferNotFull.wakeAll();
        }

        return true;
    }

    void finish()
    {
        m_finished = true;

        bufferNotEmpty.notify_all();
        bufferNotFull.notify_all();
    }

    bool finished() const
    {
        return m_finished;
    }

    bool empty()
    {
        return m_buffer.empty();
    }

private:
    size_t m_bufferSize;
    QQueue<T> m_buffer;
    std::atomic_bool m_finished;

    QMutex m_mtx; // protects the buffer and the counter
    size_t m_usedSpace = 0;

    QWaitCondition bufferNotEmpty;
    QWaitCondition bufferNotFull;
};
