#pragma once

#include <QThread>
#include <odb/sqlite/database.hxx>

#include "frame.h"

class TrackedObjectProcessor : public QThread
{
    Q_OBJECT
public:
    explicit TrackedObjectProcessor(SharedFrameBoundedQueue &frameQueue,
                                    std::shared_ptr<odb::database> db,
                                    QObject *parent = nullptr);
    void stop();

signals:
    void frameChanged(SharedFrame frame);

    // QThread interface
protected:
    void run() override;

private:
    SharedFrameBoundedQueue &m_frameQueue;
    std::shared_ptr<odb::database> m_db;
};
