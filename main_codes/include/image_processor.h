#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

#include "camera.h"
#include <opencv2/opencv.hpp>
#include <vector>

/**
 * Image processor class that detects fish in images
 */
class ImageProcessor : public Camera::ImageCallbackInterface {
public:
    // Interface for fish detection callbacks
    struct FishDetectionCallbackInterface {
        virtual void fishDetected(const cv::Mat& image) = 0;
        virtual void noFishDetected(const cv::Mat& image) = 0;
    };
    
    ImageProcessor();
    
    // Register callback for fish detection 
    void registerCallback(FishDetectionCallbackInterface* callback);
    
    // Implementation of Camera callback
    void imageReady(const cv::Mat& image) override;
    
private:
    // Fish detection algorithm
    bool detectFish(cv::Mat& image);
    
    std::vector<FishDetectionCallbackInterface*> m_callbacks;
};

#endif 

