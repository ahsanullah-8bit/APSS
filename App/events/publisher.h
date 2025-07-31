#pragma once

#include <string>

#include <zmq.hpp>

#include "apss.h"
#include "zmqproxy.h"

class Publisher
{
public:
    explicit Publisher(const std::string &topic);
    ~Publisher();
    void publish(const std::string &data, const std::string &subTopic = std::string());
    void stop();
    std::string topic() const;
    bool isOpen();

private:
    std::string m_topic;
    zmq::context_t &m_context;
    zmq::socket_t m_socket;
};

// definitions
inline Publisher::Publisher(const std::string &topic)
    : m_topic(topic)
    , m_context(ZMQProxyThread::context())
    , m_socket(m_context, zmq::socket_type::pub)
{
    m_socket.connect(SOCKET_PUB);
}

inline Publisher::~Publisher()
{
    stop();
}

inline void Publisher::publish(const std::string &data, const std::string &subTopic)
{
    zmq::message_t message(std::format("{}{} {}", m_topic, subTopic, data));
    m_socket.send(message, zmq::send_flags::none);
}

inline void Publisher::stop()
{
    m_socket.close();
}

inline std::string Publisher::topic() const
{
    return m_topic;
}

inline bool Publisher::isOpen()
{
    return m_socket;
}

// apss/detection/all
