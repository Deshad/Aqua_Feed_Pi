#include "feeder.h"
#include <chrono>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

Feeder::Feeder() {}

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
    // Here you would add code to control your feeding mechanism
    // This could involve GPIO pins, servo motors, etc.
    std::cout << "*** FEEDING MECHANISM ACTIVATED ***" << std::endl;
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
