#include "feeder.h"
#include <chrono>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

Feeder::Feeder(int motorPin) {
    // Create the motor controller
    if (motorPin >= 0) {  // Negative pins are for testing (no hardware init)
        m_motor = std::make_unique<Motor>(motorPin);
        std::cout << "Feeder initialized with motor on pin " << motorPin << std::endl;
    } else {
        std::cout << "Feeder initialized in test mode without hardware" << std::endl;
    }
}

void Feeder::fishDetected(const cv::Mat& image) {
    std::cout << "FISH DETECTED! Activating feeding mechanism..." << std::endl;
    activateFeeder();
    saveImage(image, true);
}

void Feeder::noFishDetected(const cv::Mat& image) {
    std::cout << "No feeding necessary." << std::endl;
    saveImage(image, false);
}

void Feeder::activateFeeder() {
    if (!m_motor || !m_motor->isInitialized()) {
        std::cerr << "Cannot activate feeder: Motor not initialized" << std::endl;
        return;
    }
    
    std::cout << "*** FEEDING MECHANISM ACTIVATED ***" << std::endl;
    
    // Run motor sequence for feeding
    std::cout << "Running feeder motor at full speed..." << std::endl;
    m_motor->run(100, 10, 1000);  // Full speed for 1 second
    
    std::cout << "Slowing down feeder motor..." << std::endl;
    m_motor->run(50, 10, 500);    // Half speed for 0.5 seconds
    
    std::cout << "Stopping feeder motor..." << std::endl;
    m_motor->stop();
}

void Feeder::saveImage(const cv::Mat& image, bool fishDetected) {
    // Create archive directory if it doesn't exist
    if (!fs::exists("../archive")) {
        fs::create_directory("../archive");
    }
    
    // Generate timestamp
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);
    
    char timestamp[100];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", &now_tm);
    
    // Create filename
    std::string prefix = fishDetected ? "fish_" : "no_fish_";
    std::string filename = "../archive/" + prefix + timestamp + ".jpg";
    
    // Save image
    cv::imwrite(filename, image);
    std::cout << "Image saved to: " << filename << std::endl;
}
