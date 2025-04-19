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


// bool ImageProcessor::detectFish(cv::Mat& image) {
//     // Convert to grayscale
//     cv::Mat gray;
//     cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

//     // Apply adaptive thresholding for better edge detection
//     cv::Mat binary;
//     cv::adaptiveThreshold(gray, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 2);

//     // Use Canny edge detector
//     cv::Mat edges;
//     cv::Canny(binary, edges, 50, 150);

//     // Find contours
//     std::vector<std::vector<cv::Point>> contours;
//     cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

//     int fishCount = 0;

//     for (const auto& contour : contours) {
//         double area = cv::contourArea(contour);
        
//         if (area > 500) {  // Minimum area threshold
//             cv::Rect boundRect = cv::boundingRect(contour);
//             double aspectRatio = (double)boundRect.width / boundRect.height;

//             double perimeter = cv::arcLength(contour, true);
//             double circularity = (4 * CV_PI * area) / (perimeter * perimeter + 1e-5); // Avoid division by zero
            
//             // Fish typically have elongated shapes but not perfect circles
//             if (aspectRatio > 1.5 && aspectRatio < 4.0 && circularity < 0.8) {
//                 fishCount++;
//                 cv::rectangle(image, boundRect, cv::Scalar(0, 255, 0), 2); // Draw bounding box
//                 cv::drawContours(image, std::vector<std::vector<cv::Point>>{contour}, -1, cv::Scalar(0, 0, 255), 2);
//                 if (fishCount >= 2) return true; 
//             }
//         }
//     }

//     return fishCount >= 2;
// }

bool ImageProcessor::detectFish(cv::Mat& image) {
    // Keep a copy of the original 
    cv::Mat originalImage = image.clone();
    
    cv::Mat hsvImage;
    cv::cvtColor(originalImage, hsvImage, cv::COLOR_BGR2HSV);
    
    // mask for red color
    // Red color wraps around in HSV
    cv::Mat redMask1, redMask2, redMask;
    // Lower red range (0-10)
    cv::inRange(hsvImage, cv::Scalar(0, 100, 100), cv::Scalar(10, 255, 255), redMask1);
    // Upper red range (160-180)
    cv::inRange(hsvImage, cv::Scalar(160, 100, 100), cv::Scalar(180, 255, 255), redMask2);
    // Combining masks
    cv::addWeighted(redMask1, 1.0, redMask2, 1.0, 0.0, redMask);
    
    // Applying the red mask to the original image
    cv::Mat redFiltered;
    cv::bitwise_and(originalImage, originalImage, redFiltered, redMask);
    
    // Converting to grayscale for shape analysis
    cv::Mat gray;
    cv::cvtColor(redFiltered, gray, cv::COLOR_BGR2GRAY);
    
    cv::Mat binary;
    cv::threshold(gray, binary, 1, 255, cv::THRESH_BINARY);
    
    cv::Mat morphElement = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, morphElement);
    
    // Find contours in the binary image
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Debug output
    std::cout << "Found " << contours.size() << " red contours" << std::endl;
    
    int fishCount = 0;
    
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        
        // Filter by minimum area to remove noise
        if (area > 100) {  
            cv::Rect boundRect = cv::boundingRect(contour);
            double aspectRatio = (double)boundRect.width / boundRect.height;
            
            double perimeter = cv::arcLength(contour, true);
            double circularity = (4 * CV_PI * area) / (perimeter * perimeter + 1e-5);
            
            // Calculate the percentage of red pixels in the bounding box
            cv::Mat roiMask = redMask(boundRect);
            double redPixelRatio = cv::countNonZero(roiMask) / (double)(boundRect.width * boundRect.height);
            
            std::cout << "Contour analysis - Area: " << area 
                      << ", Aspect ratio: " << aspectRatio 
                      << ", Circularity: " << circularity 
                      << ", Red pixel ratio: " << redPixelRatio << std::endl;
            
           
            if (aspectRatio > 1.0 && aspectRatio < 5.0 && 
                circularity < 0.9 && 
                redPixelRatio > 0.3) {
                
                fishCount++;
                // Draw bounding box (green)
                cv::rectangle(image, boundRect, cv::Scalar(0, 255, 0), 2);
                // Draw contour (red)
                cv::drawContours(image, std::vector<std::vector<cv::Point>>{contour}, -1, cv::Scalar(0, 0, 255), 2);
                
                // Add text label
                std::string label = "Fish " + std::to_string(fishCount);
                cv::putText(image, label, cv::Point(boundRect.x, boundRect.y - 5),
                            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
                
                if (fishCount >= 1) return true;
            }
        }
    }
    
    if (contours.size() > 0 && fishCount == 0) {
        // Saving debug images to check what's happening when no fish is detected
        cv::imwrite("../archive/debug_red_mask.jpg", redMask);
        cv::imwrite("../archive/debug_red_filtered.jpg", redFiltered);
        cv::imwrite("../archive/debug_binary.jpg", binary);
    }
    
    return fishCount >= 1;  // Changed to 1 
}
