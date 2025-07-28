#include <optional>

#include "tracker.h"

Tracker::Tracker(std::optional<std::set<std::string>> objectsToTrack,
                 float trackThresh,
                 int trackBuffer,
                 float matchThresh,
                 int videoFrameRate)
    : m_tracker(trackThresh, trackBuffer, matchThresh, videoFrameRate)
    , m_trackThresh(trackThresh)
    , m_trackBuffer(trackBuffer)
    , m_matchThresh(matchThresh)
    , m_videoFrameRate(videoFrameRate)
{
    if (objectsToTrack)
        m_objectsToTrack = objectsToTrack.value();
}

std::vector<int> Tracker::track(const PredictionList &results)
{
    std::set<int> filtered_indxs;
    std::vector<const Prediction*> sel_results;
    for (size_t p = 0; p < results.size(); ++p) {
        if (m_objectsToTrack.contains(results[p].className)) {
            sel_results.push_back(&results[p]);
        } else {
            filtered_indxs.insert(p);
        }
    }

    if (sel_results.empty())
        return {};

    // Store results in a Nx5 matrix, row[xywh + conf]
    Eigen::MatrixXf e_predictions(sel_results.size(), 5);
    for (size_t i = 0; i < sel_results.size(); ++i) {
        const cv::Rect &box = sel_results.at(i)->box;
        e_predictions.row(i) <<
            static_cast<float>(box.x),
            static_cast<float>(box.y),
            static_cast<float>(box.width),
            static_cast<float>(box.height), sel_results.at(i)->conf;
    }

    // Boxes in eigen matrix, in top-left bottom-right format
    Eigen::MatrixXf e_tlbr_boxes(sel_results.size(), 4);
    e_tlbr_boxes << e_predictions.col(0), e_predictions.col(1),
        e_predictions.col(0) + e_predictions.col(2),
        e_predictions.col(1) + e_predictions.col(3);

    // Some tracking results
    std::vector<KalmanBBoxTrack> tracks = m_tracker.process_frame_detections(e_predictions);

    // TODO: find a better approah for this.
    // we filter objects, store the filtered indexes, and re-assign track ids in the same sequence as predictions
    std::vector<int> track_ids = match_detections_with_tracks(e_tlbr_boxes.cast<double>(), tracks);
    std::vector<int> track_results(results.size(), -1);

    for (size_t i = 0, j = 0; i < results.size(); ++i) {
        if (filtered_indxs.contains(i))
            continue;

        track_results[i] = track_ids[j++];
    }

    // return track ids, filtered indexes
    return { track_results };
}

float Tracker::trackThresh() const
{
    return m_trackThresh;
}

int Tracker::trackBuffer() const
{
    return m_trackBuffer;
}

float Tracker::matchThresh() const
{
    return m_matchThresh;
}

int Tracker::videoFrameRate() const
{
    return m_videoFrameRate;
}
