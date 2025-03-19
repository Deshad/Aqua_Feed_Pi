#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <filesystem>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <gpiod.h>
#include <condition_variable>
namespace fs = std::filesystem;

// Forward declarations
class PirSensor;
class Camera;
class ImageProcessor;

/**
 * PIR motion sensor interface class with callback
 */
class PirSensor {
public:
    // Interface for motion callbacks
    struct MotionCallbackInterface {
        virtual void motionDetected(gpiod_line_event e) = 0;
    };

    PirSensor(int chipNumber = 0, int pinNumber = 17) 
        : m_chipNumber(chipNumber), 
          m_pinNumber(pinNumber), 
          m_running(false) {
        
        // Initialize GPIO
        m_chip = gpiod_chip_open_by_number(m_chipNumber);
        if (!m_chip) {
            throw std::runtime_error("Failed to open GPIO chip");
        }
        
        m_line = gpiod_chip_get_line(m_chip, m_pinNumber);
        if (!m_line) {
            gpiod_chip_close(m_chip);
            throw std::runtime_error("Failed to get GPIO line");
        }

        // Configure line for both edges (rising/falling detection)
        if (gpiod_line_request_both_edges_events(m_line, "pir_sensor") != 0) {
            gpiod_chip_close(m_chip);
            throw std::runtime_error("Failed to request line events");
        }
    }

    ~PirSensor() {
        stop();
        if (m_line) {
            gpiod_line_release(m_line);
        }
        if (m_chip) {
            gpiod_chip_close(m_chip);
        }
    }

    // Start motion detection thread
    void start() {
        m_running = true;
        m_thread = std::thread(&PirSensor::worker, this);
    }

    // Stop motion detection thread
    void stop() {
        m_running = false;
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    // Register callback for motion events
    void registerCallback(MotionCallbackInterface* callback) {
        m_callbacks.push_back(callback);
    }

private:
    // Worker thread function - blocks until motion detected
    void worker() {
        std::cout << "PIR sensor thread started. Waiting for motion events..." << std::endl;
        
        while (m_running) {
            // Wait for motion event with timeout
            const timespec ts = { 5, 0 }; // 5 second timeout
            // This blocks till an interrupt has happened!
            int r = gpiod_line_event_wait(m_line, &ts);
            
            // Check if it really has been an event
            if (1 == r) {
                gpiod_line_event event;
                gpiod_line_event_read(m_line, &event);
                
                // Call our event handler (this is our userspace ISR!)
                gpioEvent(event);
            }
        }
        
        std::cout << "PIR sensor thread stopped." << std::endl;
    }

    // Process GPIO event
    void gpioEvent(gpiod_line_event& event) {
        // Only process rising edge (motion detected)
        if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
            std::cout << "Motion detected!" << std::endl;
            // Notify all registered callbacks
            for (auto& callback : m_callbacks) {
                callback->motionDetected(event);
            }
        }
    }

    int m_chipNumber;
    int m_pinNumber;
    std::atomic<bool> m_running;
    std::thread m_thread;
    std::vector<MotionCallbackInterface*> m_callbacks;
    struct gpiod_chip* m_chip;
    struct gpiod_line* m_line;
};

/**
 * Camera class that captures images when triggered using libcamera-still
 */
class Camera {
public:
    // Interface for image callbacks
    struct ImageCallbackInterface {
        virtual void imageReady(const cv::Mat& image) = 0;
    };
    
    Camera(const std::string& outputPath = "fish_detection.jpg") 
        : m_outputPath(outputPath), m_running(false) {}
    
    ~Camera() {
        stop();
    }
    
    // Start camera thread
    void start() {
        m_running = true;
        m_thread = std::thread(&Camera::worker, this);
    }
    
    // Stop camera thread
    void stop() {
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
    
    // Request an image capture
    void captureImage() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_captureRequested = true;
        m_captureCondition.notify_one();
    }
    
    // Register callback for image capture
    void registerCallback(ImageCallbackInterface* callback) {
        m_callbacks.push_back(callback);
    }
    
private:
    // Worker thread for camera
    void worker() {
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
    
    std::string m_outputPath;
    std::atomic<bool> m_running;
    std::atomic<bool> m_captureRequested{false};
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_captureCondition;
    std::vector<ImageCallbackInterface*> m_callbacks;
};

/**
 * Image processor class that detects fish in images
 */
class ImageProcessor : public Camera::ImageCallbackInterface {
public:
    // Interface for fish detection callbacks
    struct FishDetectionCallbackInterface {
        virtual void fishDetected(const cv::Mat& image) = 0;
        virtual void noFishDetected(const cv::Mat& image) = 0;
    };
    
    ImageProcessor() {}
    
    // Register callback for fish detection results
    void registerCallback(FishDetectionCallbackInterface* callback) {
        m_callbacks.push_back(callback);
    }
    
    // Implementation of Camera callback
    void imageReady(const cv::Mat& image) override {
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
    
private:
    // Fish detection algorithm
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
    
    std::vector<FishDetectionCallbackInterface*> m_callbacks;
};

/**
 * Feeder class that controls the feeding mechanism
 */
class Feeder : public ImageProcessor::FishDetectionCallbackInterface {
public:
    Feeder() {}
    
    // Fish detected callback
    void fishDetected(const cv::Mat& image) override {
        std::cout << "FISH DETECTED! Activating feeding mechanism..." << std::endl;
        activateFeeder();
        saveImage(image, true);
    }
    
    // No fish detected callback
    void noFishDetected(const cv::Mat& image) override {
        std::cout << "No feeding necessary." << std::endl;
        saveImage(image, false);
    }
    
private:
    void activateFeeder() {
        // Here you would add code to control your feeding mechanism
        // This could involve GPIO pins, servo motors, etc.
        std::cout << "*** FEEDING MECHANISM ACTIVATED ***" << std::endl;
    }
    
    void saveImage(const cv::Mat& image, bool fishDetected) {
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
};

/**
 * Main system class that connects all components
 */
class FishMonitoringSystem : public PirSensor::MotionCallbackInterface {
public:
    FishMonitoringSystem() {
        // Clear archive
        clearArchive();
        
        // Create components
        std::cout << "Initializing PIR sensor..." << std::endl;
        m_pirSensor = std::make_unique<PirSensor>();  
        
        std::cout << "Initializing camera module..." << std::endl;
        m_camera = std::make_unique<Camera>("fish_detection.jpg");
        
        std::cout << "Initializing image processor..." << std::endl;
        m_imageProcessor = std::make_unique<ImageProcessor>();
        
        std::cout << "Initializing feeding mechanism..." << std::endl;
        m_feeder = std::make_unique<Feeder>();
        
        // Set up callback chain
        std::cout << "Setting up event callback chain..." << std::endl;
        m_pirSensor->registerCallback(this);
        m_camera->registerCallback(m_imageProcessor.get());
        m_imageProcessor->registerCallback(m_feeder.get());
    }
    
    ~FishMonitoringSystem() {
        stop();
    }
    
    // Start the system
    void start() {
        std::cout << "Starting Fish Monitoring System..." << std::endl;
        m_camera->start();
        m_pirSensor->start();
        std::cout << "System started and ready." << std::endl;
    }
    
    // Stop the system
    void stop() {
        std::cout << "Stopping Fish Monitoring System..." << std::endl;
        m_pirSensor->stop();
        m_camera->stop();
        std::cout << "System stopped." << std::endl;
    }
    
    // Motion detected callback (implements MotionCallbackInterface)
    void motionDetected(gpiod_line_event e) override {
        std::cout << "Motion event triggered camera capture!" << std::endl;
        m_camera->captureImage();
    }
    
private:
    void clearArchive() {
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
    
    std::unique_ptr<PirSensor> m_pirSensor;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<ImageProcessor> m_imageProcessor;
    std::unique_ptr<Feeder> m_feeder;
};

int main() {
    try {
        // Create and start system
        FishMonitoringSystem system;
        system.start();
        
        std::cout << "System is running. Press Enter to exit." << std::endl;
        std::cin.get();
        
        system.stop();
        std::cout << "System shut down. Goodbye!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
