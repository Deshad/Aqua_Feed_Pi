// fish_monitoring_system.cpp
#include "fish_monitoring_system.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

// Define a dedicated callback class for updating the API
class FishAPICallback : public ImageProcessor::FishDetectionCallbackInterface {
private:
    FishAPI* m_api;
    
public:
    FishAPICallback(FishAPI* api) : m_api(api) {}
    
    void fishDetected(const cv::Mat& image) override {
        m_api->setFishDetected(true);
        m_api->setLastImagePath("fish_detected.jpg");
    }
    
    void noFishDetected(const cv::Mat& image) override {
        m_api->setFishDetected(false);
        m_api->setLastImagePath("no_fish.jpg");
    }
};

FishMonitoringSystem::FishMonitoringSystem() {
    // Clear archive
    clearArchive();
    
    // Create components
    std::cout << "Initializing PIR sensor..." << std::endl;
    m_pirSensor = std::make_unique<PirSensor>();  
    
    std::cout << "Initializing camera module..." << std::endl;
    m_camera = std::make_unique<Camera>("fish_detection.jpg", 640, 480);
    
    std::cout << "Initializing image processor..." << std::endl;
    m_imageProcessor = std::make_unique<ImageProcessor>();
    
    std::cout << "Initializing feeding mechanism with motor on GPIO pin 4..." << std::endl;
    m_feeder = std::make_unique<Feeder>(4); // Motor on GPIO pin 4
    
    std::cout << "Initializing pH sensor..." << std::endl;
    m_phSensor = std::make_unique<PHSensor>();
    if (m_phSensor->initialize()) {
	 std::cout << "pH sensor initialized successfully" << std::endl;
    } else {
    	std::cerr << "Failed to initialize pH sensor" << std::endl;
	}
    // Create API with pointer to the same motor used by the feeder
    std::cout << "Initializing API..." << std::endl;
    m_api = std::make_unique<FishAPI>(m_feeder->getMotor(),m_phSensor.get());
;
    
    // Set up callback chain
    std::cout << "Setting up event callback chain..." << std::endl;
    m_pirSensor->registerCallback(this);
    m_camera->registerCallback(m_imageProcessor.get());
    m_imageProcessor->registerCallback(m_feeder.get());
    
    // Also set up a callback to update the API when fish is detected
    m_imageProcessor->registerCallback(new FishAPICallback(m_api.get()));
    
    // Register pH sensor callback to API
    m_phSensor->registerCallback(m_api.get());
    m_phSensor->registerCallback(this);

}

FishMonitoringSystem::~FishMonitoringSystem() {
    stop();
}

void FishMonitoringSystem::start() {
    std::cout << "Starting Fish Monitoring System..." << std::endl;
    m_camera->start();
    m_pirSensor->start();
   // m_phSensor->start();  // Start pH sensor monitoring
    m_api->start();  // Start the API
    std::cout << "System started and ready." << std::endl;
}

void FishMonitoringSystem::stop() {
    std::cout << "Stopping Fish Monitoring System..." << std::endl;
    m_api->stop();  // Stop the API
    m_pirSensor->stop();
    m_phSensor->cleanup();  // Stop pH sensor
    m_camera->stop();
    std::cout << "System stopped." << std::endl;
}

void FishMonitoringSystem::motionDetected(gpiod_line_event e) {
    std::cout << "Motion event triggered camera capture!" << std::endl;
    m_camera->captureImage();
}

void FishMonitoringSystem::onPHSample(float pH, float voltage, int16_t adcValue) {
    // Optional: Add any additional system-level pH handling
    std::cout << "System-level pH reading - pH: " << pH 
              << ", Voltage: " << voltage 
              << ", ADC Value: " << adcValue << std::endl;
    
    
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
