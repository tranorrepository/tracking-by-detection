#include "MCSORT.h"

#include "NoAssociatedDetector.h"

#include <dlib/optimization.h>

using std::vector;
using std::cout;
using std::endl;

const int MCSORT::maxAge = 1;
const int MCSORT::minHits = 3;

// Constructors

MCSORT::MCSORT() {}

MCSORT::MCSORT(const std::shared_ptr<Detector> &detector)
        : detector(detector) {}

// Methods

std::vector<Tracking> MCSORT::track(const cv::Mat &image) {
    if (!detector) {
        throw NoAssociatedDetector();
    }
    vector<Detection> detections = detector->detect(image);
    return track(detections);
}

vector<Tracking> MCSORT::track(const vector<Detection> &detections) {
    frameCount++;

    cout << "---DETECTIONS---" << endl;
    for (auto it = detections.begin(); it != detections.end(); ++it) {
        cout << *it << endl;
    }

    cout << "---PREDICTIONS---" << endl;
    for (auto it = predictors.begin(); it != predictors.end(); ++it) {
        it->advance();
        cout << "Tracker " << it->getID() << " " << it->getPredictedNextDetection() << endl;
    }

    Association association = associateDetectionsToPredictors(detections, predictors);
    cout << endl << "Number of matches and unmatched detections and predictors:" << endl;
    cout << association.matches.size() << " " <<
         association.unmatchedDetections.size() << " " <<
         association.unmatchedPredictors.size() << endl;

    // Update matched predictors with assigned detections
    for (auto match : association.matches) {
        predictors.at(match.second).update(detections.at(match.first));
    }

    // Create and initialise new predictors for unmatched detections
    for (auto id : association.unmatchedDetections) {
        DetectionPredictor predictor(detections.at(id));
        predictors.push_back(std::move(predictor));
    }

    // Remove predictors that have been inactive for too long
    predictors.erase(std::remove_if(
            predictors.begin(), predictors.end(),
            [](const DetectionPredictor &predictor) {
                return predictor.getTimeSinceUpdate() > maxAge;
            }), predictors.end());

    // Return trackings from active predictors
    vector<Tracking> trackings;
    for (auto it = predictors.begin(); it != predictors.end(); ++it) {
        if (it->getTimeSinceUpdate() < 1 &&
            (it->getHitStreak() >= minHits || frameCount <= minHits)) {
            trackings.push_back(it->getTracking());
        }
    }
    return trackings;
}

MCSORT::Association MCSORT::associateDetectionsToPredictors(const std::vector<Detection> &detections,
                                                            const std::vector<DetectionPredictor> &predictors,
                                                            double iouThreshold) {
    const int DOUBLE_PRECISION = 100;
    std::vector<std::pair<int, int>> matches;
    vector<int> unmatchedDetections;
    vector<int> unmatchedPredictors;

    if (predictors.empty()) {
        cout << "(No predictors)" << endl;
        for (int i = 0; i < detections.size(); ++i)
            unmatchedDetections.push_back(i);
        return MCSORT::Association{matches, unmatchedDetections, unmatchedPredictors};
    }

    dlib::matrix<int> cost(detections.size(), predictors.size());
    for (size_t row = 0; row < detections.size(); ++row) {
        for (size_t col = 0; col < predictors.size(); ++col) {
            cost(row, col) = int(DOUBLE_PRECISION *
                                 Detection::iou(detections.at(row), predictors.at(col).getPredictedNextDetection()));
        }
    }

    // Zero-pad to make it square
    if (cost.nr() > cost.nc()) {
        cost = dlib::join_rows(cost, dlib::zeros_matrix<int>(1, cost.nr() - cost.nc()));
    } else if (cost.nc() > cost.nr()) {
        cost = dlib::join_cols(cost, dlib::zeros_matrix<int>(cost.nc() - cost.nr(), 1));
    }
    cout << "---COST MATRIX---" << endl << cost;

    vector<long> assignment = dlib::max_cost_assignment(cost);

    // Filter out matches with low IoU, including those for indices from padding
    for (int d = 0; d < assignment.size(); ++d) {
        if (cost(d, assignment[d]) < iouThreshold * DOUBLE_PRECISION) {
            if (d < detections.size()) {
                unmatchedDetections.push_back(d);
            }
            if (assignment[d] < predictors.size()) {
                unmatchedPredictors.push_back(int(assignment[d]));
            }
        } else {
            matches.push_back(std::pair<int, int>(d, assignment[d]));
        }
    }
    return MCSORT::Association{matches, unmatchedDetections, unmatchedPredictors};
}