#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <ctime>
#include <filesystem>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

void clearArchive() {
    std::string archiveDir = "../archive/";
    for (const auto& entry : fs::directory_iterator(archiveDir)) {
        fs::remove(entry.path());
    }
    std::cout << "Archive cleared.\n";
}

bool detectFish(cv::Mat& image) {
    // Convert to grayscale
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // Apply adaptive thresholding for better edge detection
    cv::Mat binary;
    cv::adaptiveThreshold(gray, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 2);

    // Use Canny edge detector
    cv::Mat edges;
    cv::Canny(binary, edges, 50, 150);

    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    int fishCount = 0;

    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        
        if (area > 500) {  // Minimum area threshold
            cv::Rect boundRect = cv::boundingRect(contour);
            double aspectRatio = (double)boundRect.width / boundRect.height;

            double perimeter = cv::arcLength(contour, true);
            double circularity = (4 * CV_PI * area) / (perimeter * perimeter + 1e-5); // Avoid division by zero
            
            // Fish typically have elongated shapes but not perfect circles
            if (aspectRatio > 1.5 && aspectRatio < 4.0 && circularity < 0.8) {
                fishCount++;
                cv::rectangle(image, boundRect, cv::Scalar(0, 255, 0), 2); // Draw bounding box
                cv::drawContours(image, std::vector<std::vector<cv::Point>>{contour}, -1, cv::Scalar(0, 0, 255), 2);
                if (fishCount >= 2) return true; 
            }
        }
    }

    return fishCount >= 2;
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
    
    // Clear archive before starting
    clearArchive();

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
        
        // Detect fish and draw contours
        bool fishDetected = detectFish(image);
        
        if (fishDetected) {
            std::cout << "Fish detected! Activating feeding mechanism..." << std::endl;
            // Add code to trigger feeding mechanism
            // For example: system("python3 /path/to/feeding_script.py");
        } else {
            std::cout << "No fish detected." << std::endl;
        }
        
        // Save image with detected contours
        std::string timestamp = getTimestamp();
        std::string archivePath = "../archive/fish_" + timestamp + ".jpg";
        cv::imwrite(archivePath, image);
        
        // Wait for next check
        std::cout << "Waiting " << CHECK_INTERVAL_SECONDS 
                  << " seconds until next check..." << std::endl;
        sleep(CHECK_INTERVAL_SECONDS);
    }
    
    return 0;
}
