#ifndef FEEDER_H
#define FEEDER_H

#include "image_processor.h"
#include <opencv2/opencv.hpp>

/**
 * Feeder class that controls the feeding mechanism
 */
class Feeder : public ImageProcessor::FishDetectionCallbackInterface {
public:
    Feeder();
    
    // Fish detected callback
    void fishDetected(const cv::Mat& image) override;
    
    // No fish detected callback
    void noFishDetected(const cv::Mat& image) override;
    
private:
    void activateFeeder();
    void saveImage(const cv::Mat& image, bool fishDetected);
};

#endif // FEEDER_H
