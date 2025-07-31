#pragma once

#include <string>

#include <QVariant>

#include <zmq.hpp>

#include "apss.h"
#include "zmqproxy.h"

class Subscriber
{
public:
    explicit Subscriber(const std::string &topic);
    std::optional<std::tuple<std::string, std::string>> checkForUpdates(int timeout_ms = 1000);
    void stop();
    std::string topic() const;
    bool isOpen();

private:
    std::tuple<std::string, std::string> separateSubTopicAndMessage(const std::string &msg);

private:
    std::string m_topic;
    zmq::context_t &m_context;
    zmq::socket_t m_socket;
};


// definitions
inline Subscriber::Subscriber(const std::string &topic)
    : m_topic(topic)
    , m_context(ZMQProxyThread::context())
    , m_socket(m_context, zmq::socket_type::sub)
{
    m_socket.set(zmq::sockopt::subscribe, m_topic);
    m_socket.connect(SOCKET_SUB);
}

inline std::optional<std::tuple<std::string, std::string> > Subscriber::checkForUpdates(int timeout_ms)
{
    zmq::pollitem_t items[] = { static_cast<void*>(m_socket), 0, ZMQ_POLLIN, 0 };

    zmq::poll(items, 1, std::chrono::milliseconds(timeout_ms));

    if (items[0].revents & ZMQ_POLLIN) {
        std::string msg;
        auto res = m_socket.recv(zmq::buffer(msg), zmq::recv_flags::none);

        if (res.has_value())
            return separateSubTopicAndMessage(msg);
    }

    return {};
}

inline void Subscriber::stop()
{
    m_socket.close();
}

inline std::string Subscriber::topic() const
{
    return m_topic;
}

inline bool Subscriber::isOpen()
{
    return m_socket;
}

inline std::tuple<std::string, std::string> Subscriber::separateSubTopicAndMessage(const std::string &msg) {
    auto first_space = msg.find_first_of(' ');
    if (first_space == std::string::npos)
        return { "", msg};

    std::string topic = msg.substr(0, first_space);
    std::string content = msg.substr(first_space + 1);
    return { topic, content };
}

