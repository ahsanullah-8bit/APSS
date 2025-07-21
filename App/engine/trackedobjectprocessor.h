#pragma once

#include <QThread>

#include "frame.h"

class TrackedObjectProcessor : public QThread
{
    Q_OBJECT
public:
    explicit TrackedObjectProcessor(SharedFrameBoundedQueue &frameQueue, QObject *parent = nullptr);
    void stop();

signals:
    void frameChanged(SharedFrame frame);

    // QThread interface
protected:
    void run() override;

private:
    SharedFrameBoundedQueue &m_frameQueue;
};
