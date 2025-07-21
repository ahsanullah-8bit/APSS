#include "videorecorder.h"

#include <QMediaRecorder>

VideoRecorder::VideoRecorder()
{
    QMediaRecorder recorder;

    recorder.setQuality(QMediaRecorder::HighQuality);
    recorder.setOutputLocation(QUrl::fromLocalFile("test.mp3"));
    recorder.record();
}
