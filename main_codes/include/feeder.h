#ifndef FEEDER_H
#define FEEDER_H

#include "image_processor.h"
#include "motor.h"
#include <opencv2/opencv.hpp>
#include <memory>

/**
 *  class that controls the feeding 
 */
class Feeder : public ImageProcessor::FishDetectionCallbackInterface {
public:
    
    Feeder(int motorPin = 4);
    void fishDetected(const cv::Mat& image) override;
    void noFishDetected(const cv::Mat& image) override;
    Motor* getMotor() {return m_motor.get();}
private:
    /**
     * Activate the feeding mechanism
     */
    void activateFeeder();
    
    /**
     * Save the captured image
     */
    void saveImage(const cv::Mat& image, bool fishDetected);
    
    // Motor control
    std::unique_ptr<Motor> m_motor;
};

#endif 
