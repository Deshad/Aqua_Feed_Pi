#ifndef CAMERA_H
#define CAMERA_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <vector>

/**
 * Camera class that captures images when triggered using libcamera-still
 */
class Camera {
public:
    // Interface for image callbacks
    struct ImageCallbackInterface {
        virtual void imageReady(const cv::Mat& image) = 0;
    };
    
    Camera(const std::string& outputPath = "fish_detection.jpg",
           int width = 640, 
           int height = 480);
    ~Camera();
    
    // Start camera thread
    void start();
    
    // Stop camera thread
    void stop();
    
    // Request an image capture
    void captureImage();
    
    // Register callback for image capture
    void registerCallback(ImageCallbackInterface* callback);
    
private:
    // Worker thread for camera
    void worker();
    
    std::string m_outputPath;
    int m_width;
    int m_height;
    std::atomic<bool> m_running;
    std::atomic<bool> m_captureRequested{false};
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_captureCondition;
    std::vector<ImageCallbackInterface*> m_callbacks;
};

#endif // CAMERA_H
