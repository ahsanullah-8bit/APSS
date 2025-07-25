#pragma once

#include "publisher.h"
#include "subscriber.h"

class DetectionSubscriber final : public Subscriber {
public:
    explicit DetectionSubscriber()
        : Subscriber("detection/")
    {}
};


class DetectionPublisher final : public Publisher {
public:
    explicit DetectionPublisher()
        : Publisher("detection/")
    {}
};
