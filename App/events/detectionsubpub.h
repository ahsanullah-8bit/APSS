#pragma once

#include "publisher.h"
#include "subscriber.h"

class DetectionSubscriber final : public Subscriber {
public:
    explicit DetectionSubscriber()
        : Subscriber(m_topic)
    {}

private:
    std::string m_topic = "detection/";
};


class DetectionPublisher final : public Publisher {
    explicit DetectionPublisher()
        : Publisher(m_topic)
    {}

private:
    std::string m_topic = "detection/";
};
