#pragma once

#include <QThread>
#include <zmq.hpp>

#include "apss.h"

class ZMQProxyThread : public QThread {
public:
    explicit ZMQProxyThread();
    ~ZMQProxyThread();
    void stop();
    static zmq::context_t &context();

    // QThread interface
protected:
    void run() override;

private:
    /*
     * No I/O threads are involved in passing messages using the inproc transport.
     * Therefore, if you are using a ØMQ context for in-process messaging only you
     * can initialise the context with zero I/O threads.
     *
     * Source: http://api.zeromq.org/4-3:zmq-inproc
     * https://github.com/zeromq/cppzmq/blob/3bcbd9dad2f57180aacd4b4aea292a74f0de7ef4/examples/pubsub_multithread_inproc.cpp#L80
     */
    static inline zmq::context_t m_context = zmq::context_t(0);
};


// definitions
inline ZMQProxyThread::ZMQProxyThread()
{}

inline ZMQProxyThread::~ZMQProxyThread()
{
    stop();
}

inline void ZMQProxyThread::stop()
{
    m_context.shutdown();

    if (isRunning()) {
        wait(500); // ms
        terminate();
        wait();
    }
}

inline zmq::context_t &ZMQProxyThread::context()
{
    return m_context;
}

inline void ZMQProxyThread::run()
{
    zmq::socket_t frontend(m_context, zmq::socket_type::sub);
    frontend.bind(SOCKET_SUB);

    zmq::socket_t backend(m_context, zmq::socket_type::pub);
    backend.bind(SOCKET_PUB);

    try {
        zmq::proxy(frontend, backend);
    } catch (const zmq::error_t&) {}
}
