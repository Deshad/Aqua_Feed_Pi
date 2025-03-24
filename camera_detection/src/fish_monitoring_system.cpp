#include "fish_monitoring_system.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

FishMonitoringSystem::FishMonitoringSystem() {
    // Clear archive
    clearArchive();
    
    // Create components
    std::cout << "Initializing PIR sensor..." << std::endl;
    m_pirSensor = std::make_unique<PirSensor>();  
    
    std::cout << "Initializing camera module..." << std::endl;
    m_camera = std::make_unique<Camera>("fish_detection.jpg",640,480);
    
    std::cout << "Initializing image processor..." << std::endl;
    m_imageProcessor = std::make_unique<ImageProcessor>();
    
    std::cout << "Initializing feeding mechanism with motor on GPIO pin 4..." << std::endl;
    m_feeder = std::make_unique<Feeder>(4); // Motor on GPIO pin 4
    
    // Set up callback chain
    std::cout << "Setting up event callback chain..." << std::endl;
    m_pirSensor->registerCallback(this);
    m_camera->registerCallback(m_imageProcessor.get());
    m_imageProcessor->registerCallback(m_feeder.get());
}

FishMonitoringSystem::~FishMonitoringSystem() {
    stop();
}

void FishMonitoringSystem::start() {
    std::cout << "Starting Fish Monitoring System..." << std::endl;
    m_camera->start();
    m_pirSensor->start();
    std::cout << "System started and ready." << std::endl;
}

void FishMonitoringSystem::stop() {
    std::cout << "Stopping Fish Monitoring System..." << std::endl;
    m_pirSensor->stop();
    m_camera->stop();
    std::cout << "System stopped." << std::endl;
}

void FishMonitoringSystem::motionDetected(gpiod_line_event e) {
    std::cout << "Motion event triggered camera capture!" << std::endl;
    m_camera->captureImage();
}

void FishMonitoringSystem::clearArchive() {
    if (fs::exists("../archive")) {
        for (const auto& entry : fs::directory_iterator("../archive")) {
            fs::remove(entry.path());
        }
        std::cout << "Archive cleared." << std::endl;
    } else {
        fs::create_directory("../archive");
        std::cout << "Archive directory created." << std::endl;
    }
}

