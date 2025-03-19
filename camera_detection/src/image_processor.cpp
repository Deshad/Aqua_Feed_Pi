#include "image_processor.h"
#include <iostream>

ImageProcessor::ImageProcessor() {}

void ImageProcessor::registerCallback(FishDetectionCallbackInterface* callback) {
    m_callbacks.push_back(callback);
}

void ImageProcessor::imageReady(const cv::Mat& image) {
    std::cout << "Processing image for fish detection..." << std::endl;
    cv::Mat processedImage = image.clone();
    bool fishDetected = detectFish(processedImage);
    
    if (fishDetected) {
        std::cout << "Fish detected!" << std::endl;
        for (auto& callback : m_callbacks) {
            callback->fishDetected(processedImage);
        }
    } else {
        std::cout << "No fish detected." << std::endl;
        for (auto& callback : m_callbacks) {
            callback->noFishDetected(processedImage);
        }
    }
}

bool ImageProcessor::detectFish(cv::Mat& image) {
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
