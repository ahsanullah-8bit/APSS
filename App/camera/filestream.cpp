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
#include "filestream.h"

#include <opencv2/opencv.hpp>
#include <qdebug.h>

#include "frame.h"

FileStream::FileStream(tbb::concurrent_bounded_queue<Frame> &sharedBuffer
                       , const QString& filePath
                       , QObject *parent)
    : QThread{parent}
    , m_sharedQueue(sharedBuffer)
    , m_filePath(filePath)
    , m_id("localfile")
{}

void FileStream::setFilePath(const QString &path)
{
    m_filePath = path;
}

void FileStream::run()
{
    cv::VideoCapture cap(m_filePath.toStdString());

    qDebug() << "Filestream started with path" << m_filePath;

    if (!cap.isOpened()) {
        qCritical() << "Failed to open up the file" << m_filePath;
        return;
    }

    try {
        cv::Mat frame;
        size_t frame_index = 0;
        while (!QThread::currentThread()->isInterruptionRequested() && cap.read(frame)) {
            FrameId frame_id(m_id, QString::number(frame_index++)); // std::format("filestream_frame_{}", frame_count++);
            Frame full_frame(frame_id, frame.clone());
            m_sharedQueue.emplace(full_frame);
        }
    }
    catch(const tbb::user_abort &e) {}
    catch(...) {
        qCritical() << "Uknown/Uncaught exception occurred.";
    }

    qInfo() << "Aborting on thread" << QThread::currentThread()->objectName();
}

QString FileStream::id() const
{
    return m_id;
}
