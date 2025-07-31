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
    void frameChangedWithEvents(SharedFrame frame, const QList<int> &activeEvents);

    // QThread interface
protected:
    void run() override;

private:
    QString cropAndSaveThumbnail(const QString &name, const SharedFrame frame, const Prediction &pred);

private:
    SharedFrameBoundedQueue &m_frameQueue;
    std::shared_ptr<odb::database> m_db;
};
