#pragma once

#include <algorithm> // Required for std::min
#include <QDateTime>
#include <QList>

class EventsPerSecond {
public:
    explicit EventsPerSecond(int max_events = 1000, int last_n_seconds = 10)
        : m_start(0.0), // Initialize with 0.0 or a sentinel value
        m_maxEvents(max_events),
        m_lastNSeconds(last_n_seconds) {
    }

    void start() {
        m_start = QDateTime::currentDateTime().toSecsSinceEpoch();
    }

    void update() {
        qreal now = QDateTime::currentDateTime().toSecsSinceEpoch();
        if (m_start == 0.0) { // Check if m_start is uninitialized
            m_start = now;
        }
        m_timestamps.append(now);
        // Truncate the list when it goes 100 over the max_size
        if (m_timestamps.length() > m_maxEvents + 100) {
            // This is a more efficient way to remove elements from the beginning
            // than repeatedly calling removeAt(0) or using a loop with del.
            // It creates a new list with the desired elements.
            m_timestamps = m_timestamps.mid(m_timestamps.length() - m_maxEvents);
        }
        expireTimestamps(now);
    }

    qreal eps() {
        qreal now = QDateTime::currentDateTime().toSecsSinceEpoch();
        if (m_start == 0.0) { // Check if m_start is uninitialized
            m_start = now;
        }
        // Compute the (approximate) events in the last n seconds
        expireTimestamps(now);
        qreal seconds = std::min(now - m_start, (qreal)m_lastNSeconds);
        // Avoid divide by zero
        if (seconds == 0.0) {
            seconds = 1.0;
        }

        return m_timestamps.length() / seconds;
    }

private:
    // Remove aged out timestamps
    void expireTimestamps(qreal now) {
        qreal threshold = now - m_lastNSeconds;
        // Using a while loop with removeFirst() is efficient for QList
        while (!m_timestamps.isEmpty() && m_timestamps.first() < threshold) {
            m_timestamps.removeFirst();
        }
    }

    qreal m_start;
    int m_maxEvents;
    int m_lastNSeconds;
    QList<qreal> m_timestamps;
};
