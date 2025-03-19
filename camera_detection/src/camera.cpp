#include "camera.h"
#include <iostream>

Camera::Camera(const std::string& outputPath) 
    : m_outputPath(outputPath), m_running(false) {}

Camera::~Camera() {
    stop();
}

void Camera::start() {
    m_running = true;
    m_thread = std::thread(&Camera::worker, this);
}

void Camera::stop() {
    m_running = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_captureRequested = true;
        m_captureCondition.notify_one();
    }
    
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void Camera::captureImage() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_captureRequested = true;
    m_captureCondition.notify_one();
}

void Camera::registerCallback(ImageCallbackInterface* callback) {
    m_callbacks.push_back(callback);
}

void Camera::worker() {
    std::cout << "Camera thread started." << std::endl;
    
    while (m_running) {
        // Wait for capture request (blocks until requested)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_captureCondition.wait(lock, [this]() {
                return m_captureRequested || !m_running;
            });
            
            if (!m_running) break;
            m_captureRequested = false;
        }
        
        // Capture image using libcamera-still
        std::cout << "Capturing image..." << std::endl;
        std::string command = "libcamera-still -o " + m_outputPath;
        int result = system(command.c_str());
        
        if (result != 0) {
            std::cerr << "Failed to capture image with libcamera-still" << std::endl;
            continue;
        }
        
        // Load captured image
        cv::Mat image = cv::imread(m_outputPath);
        if (image.empty()) {
            std::cerr << "Failed to load captured image from " << m_outputPath << std::endl;
            continue;
        }
        
        // Image captured successfully, notify callbacks
        std::cout << "Image captured successfully, processing..." << std::endl;
        for (auto& callback : m_callbacks) {
            callback->imageReady(image);
        }
    }
    
    std::cout << "Camera thread stopped." << std::endl;
}
