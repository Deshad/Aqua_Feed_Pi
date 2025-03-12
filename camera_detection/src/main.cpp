#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <ctime>
#include <opencv2/opencv.hpp>

bool detectFish(const cv::Mat& image) {
    // Convert to grayscale
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    
    // Apply Gaussian blur to reduce noise
    cv::GaussianBlur(gray, gray, cv::Size(5, 5), 0);
    
    // Use Canny edge detector
    cv::Mat edges;
    cv::Canny(gray, edges, 50, 150);
    
    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Filter contours based on size and shape
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area > 500) {  // Minimum area threshold
            // Calculate aspect ratio of the contour's bounding rect
            cv::Rect boundRect = cv::boundingRect(contour);
            double aspectRatio = (double)boundRect.width / boundRect.height;
            
            // Fish-like objects typically have specific aspect ratios
            if (aspectRatio > 1.5 && aspectRatio < 4.0) {
                return true;  // Potential fish detected
            }
        }
    }
    
    return false;  // No fish detected
}

std::string getTimestamp() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    return std::to_string(1900 + ltm->tm_year) + "-" +
           std::to_string(1 + ltm->tm_mon) + "-" +
           std::to_string(ltm->tm_mday) + "_" +
           std::to_string(ltm->tm_hour) + "-" +
           std::to_string(ltm->tm_min) + "-" +
           std::to_string(ltm->tm_sec);
}

int main() {
    const int CHECK_INTERVAL_SECONDS = 30; // Check every 30 seconds
    
    while (true) {
        // Capture image
        std::string imagePath = "fish_detection.jpg";
        std::string command = "libcamera-still -o " + imagePath;
        std::cout << "Capturing image..." << std::endl;
        int result = system(command.c_str());
        
        if (result != 0) {
            std::cerr << "Failed to capture image. Retrying in " 
                      << CHECK_INTERVAL_SECONDS << " seconds..." << std::endl;
            sleep(CHECK_INTERVAL_SECONDS);
            continue;
        }
        
        // Load and process the image
        cv::Mat image = cv::imread(imagePath);
        if (image.empty()) {
            std::cerr << "Failed to load captured image. Retrying in " 
                      << CHECK_INTERVAL_SECONDS << " seconds..." << std::endl;
            sleep(CHECK_INTERVAL_SECONDS);
            continue;
        }
        
        // Detect fish
        bool fishDetected = detectFish(image);
        
        if (fishDetected) {
            std::cout << "Fish detected! Activating feeding mechanism..." << std::endl;
            // Add code to trigger feeding mechanism
            // For example: system("python3 /path/to/feeding_script.py");
        } else {
            std::cout << "No fish detected." << std::endl;
        }
        
        // Save image with timestamp for record keeping
        std::string timestamp = getTimestamp();
        std::string archivePath = "../archive/fish_" + timestamp + ".jpg";
        std::string archiveCommand = "cp " + imagePath + " " + archivePath;
        system(archiveCommand.c_str());
        
        // Wait for next check
        std::cout << "Waiting " << CHECK_INTERVAL_SECONDS 
                  << " seconds until next check..." << std::endl;
        sleep(CHECK_INTERVAL_SECONDS);
    }
    
    return 0;
}
