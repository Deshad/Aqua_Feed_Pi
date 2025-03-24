#ifndef FEEDER_H
#define FEEDER_H

#include "image_processor.h"
#include "motor.h"
#include <opencv2/opencv.hpp>
#include <memory>

/**
 * Feeder class that controls the feeding mechanism
 */
class Feeder : public ImageProcessor::FishDetectionCallbackInterface {
public:
    /**
     * Constructor
     * @param motorPin GPIO pin to use for the feeder motor
     */
    Feeder(int motorPin = 4);
    
    /**
     * Fish detected callback
     * @param image The image where fish was detected
     */
    void fishDetected(const cv::Mat& image) override;
    
    /**
     * No fish detected callback
     * @param image The image where no fish was detected
     */
    void noFishDetected(const cv::Mat& image) override;
    
private:
    /**
     * Activate the feeding mechanism
     */
    void activateFeeder();
    
    /**
     * Save the captured image
     * @param image The image to save
     * @param fishDetected Whether fish was detected in the image
     */
    void saveImage(const cv::Mat& image, bool fishDetected);
    
    // Motor control
    std::unique_ptr<Motor> m_motor;
};

#endif // FEEDER_H
