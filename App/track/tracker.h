#pragma once

#include <BYTETracker.h>

#include "utils/prediction.h"
#include "config/objectconfig.h"

class Tracker
{
public:
    explicit Tracker(std::optional<std::set<std::string>> objectsToTrack,
                     float trackThresh = 0.25,
                     int trackBuffer = 30,
                     float matchThresh = 0.8,
                     int videoFrameRate = 30);

    // returns track ids
    std::vector<int> track(const PredictionList &results);
    float trackThresh() const;
    int trackBuffer() const;
    float matchThresh() const;
    int videoFrameRate() const;

private:
    BYTETracker m_tracker;
    const float m_trackThresh = 0.25;
    const int m_trackBuffer = 30;
    const float m_matchThresh = 0.8;
    const int m_videoFrameRate = 30;
    std::set<std::string> m_objectsToTrack = DEFAULT_TRACKED_OBJECTS;
};
