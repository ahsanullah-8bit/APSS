#include "trackedobjectprocessor.h"

TrackedObjectProcessor::TrackedObjectProcessor(SharedFrameBoundedQueue &frameQueue, QObject *parent)
    : QThread{parent}
    , m_frameQueue(frameQueue)
{
    setObjectName("tracked_object_processor");
}

void TrackedObjectProcessor::stop()
{
    try {
        if (isRunning()) {
            requestInterruption();

            qDebug() << "Waiting for" << objectName() << "to exit gracefully...";
            if (!wait(3000)) {
                qDebug() << objectName() << "didn't exit. Applying force killing...";
                terminate();
                wait();
            }
            qDebug() << objectName() << "thread has exited...";
        }
    } catch (const std::exception &e) {
        qDebug() << e.what();
    } catch (...) {
        qCritical() << "Uknown/Uncaught exception occurred!";
    }
}


void TrackedObjectProcessor::run()
{
    qInfo() << "Starting" << objectName() << "thread";

    try {
        while(!isInterruptionRequested()) {
            SharedFrame frame;
            m_frameQueue.pop(frame);
            if (!frame)
                continue;

            emit frameChanged(frame);
        }
    }
    catch(const tbb::user_abort &) {}
    catch(const std::exception &e) {
        qCritical() << e.what();
    }
    catch(...) {
        qCritical() << "Uknown/Uncaught exception occurred.";
    }

    qInfo() << "Stopping" << objectName() << "thread";
}
