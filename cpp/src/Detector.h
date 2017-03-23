#ifndef CPP_DETECTOR_H
#define CPP_DETECTOR_H


#include <opencv2/core/mat.hpp>
#include <vector>
#include "Detection.h"

class Detector {
public:
    Detector();

    virtual ~Detector();

    Detector(Detector &&rhs) = default;

    Detector &operator=(Detector &&rhs) = default;

    virtual std::vector<Detection> detect(const cv::Mat &image) = 0;

    // Prevent copying
    Detector(const Detector &) = delete;

    Detector &operator=(const Detector &) = delete;
};


#endif //CPP_DETECTOR_H