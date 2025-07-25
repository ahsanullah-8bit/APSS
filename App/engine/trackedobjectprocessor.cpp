#include <QLoggingCategory>

#include "db/event"
#include "events/detectionsubpub.h"
#include "trackedobjectprocessor.h"
#include "utils/framemanager.h"

Q_STATIC_LOGGING_CATEGORY(object_proc, "apss.engine.object_proc")

TrackedObjectProcessor::TrackedObjectProcessor(SharedFrameBoundedQueue &frameQueue,
                                               std::shared_ptr<odb::database> db,
                                               QObject *parent)
    : QThread{parent}
    , m_frameQueue(frameQueue)
    , m_db(db)
{
    setObjectName("tracked_object_processor");
}

void TrackedObjectProcessor::stop()
{
    try {
        if (isRunning()) {
            requestInterruption();

            qCDebug(object_proc) << "Waiting for" << objectName() << "to exit gracefully...";
            if (!wait(3000)) {
                qCDebug(object_proc) << objectName() << "didn't exit. Applying force killing...";
                terminate();
                wait();
            }
            qCDebug(object_proc) << objectName() << "thread has exited...";
        }
    } catch (const std::exception &e) {
        qCDebug(object_proc) << e.what();
    } catch (...) {
        qCCritical(object_proc) << "Uknown/Uncaught exception occurred!";
    }
}


void TrackedObjectProcessor::run()
{
    qCInfo(object_proc) << "Starting" << objectName() << "thread";

    Publisher publisher("record/");
    FrameManager &frame_manager = FrameManager::instance();

    try {
        while(!isInterruptionRequested()) {
            SharedFrame frame;
            m_frameQueue.pop(frame);
            if (!frame)
                continue;

            frame_manager.write(frame->id(), frame->data());
            publisher.publish(frame->id().toStdString(), "frame_id");

            if (!frame->predictions().empty()) {
                // if frame has predictions, push it to the frame manager/store
                // odb::transaction t(m_db->begin());
                // try {
                //     Event event;
                //     event.setId(frame->id());
                //     event.setLabel(frame->id());
                //     m_db->persist(event);
                //     t.commit();
                // } catch (const odb::exception& e) {
                //     t.rollback();
                // }

            }

            emit frameChanged(frame);
        }
    }
    catch(const tbb::user_abort &) {}
    catch(const std::exception &e) {
        qCCritical(object_proc) << e.what();
    }
    catch(...) {
        qCCritical(object_proc) << "Uknown/Uncaught exception occurred.";
    }

    qCInfo(object_proc) << "Stopping" << objectName() << "thread";
}
