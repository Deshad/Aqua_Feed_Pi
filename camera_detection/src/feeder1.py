include "feeder.h"
include <chrono>
include <filesystem>
include <iostream>
include <fstream>  // For file output
include <string>
include <vector>
include <sstream>
    
    namespace fs = std::filesystem;
    
    //  Simple logging function (replace with a proper logging library)
    void logMessage(const std::string& message, const std::string& level = "INFO") {
        std::ofstream logFile("feeder.log", std::ios::app);
        if (logFile.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto now_time_t = std::chrono::system_clock::to_time_t(now);
            std::tm now_tm = *std::localtime(&now_time_t);
            char timestamp[100];
            std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &now_tm);
            logFile << "[" << timestamp << "] [" << level << "] " << message << std::endl;
            logFile.close();
        } else {
            std::cerr << "Error: Could not open log file." << std::endl;
            std::cerr << message << std::endl; //  Fallback to stderr
        }
    }
    
    Feeder::Feeder(int motorPin, const std::string& configFilePath) : m_configFilePath(configFilePath) {
        //  Create the motor controller
        if (motorPin >= 0) {  //  Negative pins are for testing (no hardware init)
            m_motor = std::make_unique<Motor>(motorPin);
            logMessage("Feeder initialized with motor on pin " + std::to_string(motorPin));
        } else {
            logMessage("Feeder initialized in test mode without hardware");
        }
    
        loadConfig(); //  Load motor sequence from config file
    }
    
    void Feeder::loadConfig() {
        std::ifstream configFile(m_configFilePath);
        if (!configFile.is_open()) {
            logMessage("Error: Could not open config file: " + m_configFilePath, "ERROR");
            //  Default motor sequence (or throw exception)
            m_motorSequence = { {100, 1000}, {50, 500} };
            return;
        }
    
        std::string line;
        while (std::getline(configFile, line)) {
            std::istringstream iss(line);
            int speed, duration;
            if (iss >> speed >> duration) {
                m_motorSequence.push_back({ speed, duration });
            } else if (!line.empty()) {
                logMessage("Warning: Invalid line in config file: " + line, "WARN");
            }
        }
    
        if (m_motorSequence.empty()) {
             logMessage("Warning: No motor sequence found in config file: " + m_configFilePath + ". Using default sequence.", "WARN");
            //  Default motor sequence
            m_motorSequence = { {100, 1000}, {50, 500} };
        } else {
            logMessage("Motor sequence loaded from config file.");
        }
        configFile.close();
    }
    
    void Feeder::fishDetected(const cv::Mat& image) {
        logMessage("FISH DETECTED! Activating feeding mechanism...");
        activateFeeder();
        saveImage(image, true);
    }
    
    void Feeder::noFishDetected(const cv::Mat& image) {
        logMessage("No feeding necessary.");
        saveImage(image, false);
    }
    
    void Feeder::activateFeeder() {
        if (!m_motor || !m_motor->isInitialized()) {
            logMessage("Cannot activate feeder: Motor not initialized", "ERROR");
            return;
        }
    
        logMessage("*** FEEDING MECHANISM ACTIVATED ***");
    
        //  Run motor sequence for feeding
        for (const auto& step : m_motorSequence) {
            logMessage("Running feeder motor at speed " + std::to_string(step.speed) + " for " + std::to_string(step.duration) + "ms");
            if (!m_motor->run(step.speed, 10, step.duration)) {
                logMessage("Error: Motor run failed!", "ERROR");
                return; //  Or throw an exception, depending on error handling strategy
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(step.duration)); //  Basic delay - consider non-blocking if needed
        }
    
        if (!m_motor->stop()) {
            logMessage("Error: Motor stop failed!", "ERROR");
        } else {
            logMessage("Feeder motor stopped.");
        }
    }
    
    void Feeder::saveImage(const cv::Mat& image, bool fishDetected) {
        //  Create archive directory if it doesn't exist
        if (!fs::exists("../archive")) {
            if (!fs::create_directory("../archive")) {
                logMessage("Error: Could not create archive directory!", "ERROR");
                return;
            }
            logMessage("Created archive directory: ../archive");
        }
    
        //  Generate timestamp
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&now_time_t);
    
        char timestamp[100];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", &now_tm);
    
        //  Create filename
        std::string prefix = fishDetected ? "fish_" : "no_fish_";
        std::string filename = "../archive/" + prefix + timestamp + ".jpg";
    
        //  Save image
        if (!cv::imwrite(filename, image)) {
            logMessage("Error: Could not save image to: " + filename, "ERROR");
        } else {
            logMessage("Image saved to: " + filename);
        }
    }