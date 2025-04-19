#include "fish_api.h"
#include <jsoncpp/json/json.h>
#include <iostream>
#include <ctime>

// Constructor
FishAPI::FishAPI(Motor* motor, PHSensor* phSensor, PirSensor* pirSensor)
    : m_motor(motor),
      m_phSensor(phSensor),
      m_pirSensor(pirSensor), 
      m_camera(), // Initialize Camera 
      m_imageProcessor(),
      m_running(false),
      m_getHandler(this),
      m_postHandler(this),
      m_fishDetected(false),
      m_feedCount(0),
      m_autoFeedCount(0),
      m_lastFeedTime(0),
      m_AutolastFeedTime(0),
      m_autoModeEnabled(true),
      m_currentPH(0.0f),
      m_currentPHVoltage(0.0f),
      m_lastPHReadTime(0) {
    m_pirSensor->registerCallback(this);
    m_camera.registerCallback(&m_imageProcessor); // Camera sends images to ImageProcessor
    m_imageProcessor.registerCallback(this); // ImageProcessor notifies FishAPI
    if (m_phSensor) {
        std::cout << "Initializing pH sensor in FishAPI constructor..." << std::endl;
        if (m_phSensor->initialize()) {
            std::cout << "pH sensor initialized successfully in FishAPI" << std::endl;
        } else {
            std::cerr << "Failed to initialize pH sensor in FishAPI" << std::endl;
        }
    } else {
        std::cerr << "pH sensor is NULL in FishAPI constructor" << std::endl;
    }
}

// Destructor
FishAPI::~FishAPI() {
    stop();
}

// Start the API in a separate thread
void FishAPI::start() {
    if (!m_running) {
        m_running = true;
        m_thread = std::thread(&FishAPI::threadFunction, this);
        if (m_autoModeEnabled) {
            m_pirSensor->start(); 
            m_camera.start();
        }
        std::cout << "API thread started" << std::endl;
    }
}

// Stop the API
void FishAPI::stop() {
    if (m_running) {
        m_running = false;
        m_pirSensor->stop(); 
        m_camera.stop();
        if (m_thread.joinable()) {
            m_handler.stop();
            m_thread.join();
        }
        std::cout << "API thread stopped" << std::endl;
    }
}

// Update fish detection status
void FishAPI::setFishDetected(bool detected) {
    m_fishDetected = detected;
    std::cout << "Fish detected set to: " << (detected ? "true" : "false") << std::endl;
    if (detected && m_autoModeEnabled) {
        feedFish(false); // Trigger automatic feeding only if auto mode is enabled
    }
}

// Update last img path
void FishAPI::setLastImagePath(const std::string& path) {
    m_lastImagePath = path;
}

//  feeding logic
void FishAPI::feedFish(bool override) {
    if ((m_autoModeEnabled && m_fishDetected) || override) {
        std::cout << "Feeding fish..." << std::endl;
        if (m_motor && m_motor->isInitialized()) {
            m_motor->run(100, 10, 3000);
            m_motor->run(50, 10, 500);
            m_motor->stop();
            // m_lastFeedTime = std::time(nullptr);
            if (override) {
                m_feedCount++;
                m_lastFeedTime = std::time(nullptr);
            } else {
                m_autoFeedCount++;
                m_AutolastFeedTime = std::time(nullptr);
            }
        } else {
            std::cerr << "Motor not initialized" << std::endl;
        }
    } else {
        std::cout << "Feed ignored - auto mode disabled or no fish detected and override not set" << std::endl;
    }
}

// Motion detection callback from PIR sensor
void FishAPI::motionDetected(gpiod_line_event event) {
    if (m_autoModeEnabled) {
        std::cout << "Motion detected, triggering camera..." << std::endl;
        m_camera.captureImage(); // Trigger camera to capture an image
    } else {
        std::cout << "Motion detected, but auto mode is off. Ignoring..." << std::endl;
    }
}

// Fish detection callbacks from ImageProcessor
void FishAPI::fishDetected(const cv::Mat& image) {
    std::cout << "FishAPI: Fish detected callback received" << std::endl;
    if (m_autoModeEnabled) {
        setFishDetected(true);
        setLastImagePath("last_detected_image.jpg");
        cv::imwrite("last_detected_image.jpg", image); 
    }
}

void FishAPI::noFishDetected(const cv::Mat& image) {
    std::cout << "FishAPI: No fish detected callback received" << std::endl;
    if (m_autoModeEnabled) {
        setFishDetected(false);
    }
}

// Request a pH reading
float FishAPI::requestPHReading() {
    std::cout << "requestPHReading() called" << std::endl;
    if (!m_phSensor) {
        std::cerr << "ERROR: pH sensor object is NULL" << std::endl;
        return -1.0f;
    }
    std::cout << "pH sensor initialization status: " << (m_phSensor->isInitialized() ? "Initialized" : "Not initialized") << std::endl;
    if (!m_phSensor->isInitialized()) {
        std::cerr << "pH sensor not initialized, attempting to initialize..." << std::endl;
        if (!m_phSensor->initialize()) {
            std::cerr << "pH sensor initialization failed" << std::endl;
            return -1.0f;
        }
        std::cout << "pH sensor initialization successful" << std::endl;
    }
    std::cout << "Reading pH value..." << std::endl;
    float ph = m_phSensor->readPH();
    std::cout << "Raw pH value from sensor: " << ph << std::endl;
    if (ph < 0) {
        std::cerr << "Failed to read pH value" << std::endl;
        return -1.0f;
    }
    std::cout << "Successfully read pH value: " << ph << std::endl;
    return ph;
}

void FishAPI::threadFunction() {
    try {
        m_handler.start(&m_getHandler, &m_postHandler, "/tmp/fish_api.socket");
        while (m_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        m_handler.stop();
    } catch (const std::exception& e) {
        std::cerr << "API thread error: " << e.what() << std::endl;
    }
}

// GET Handler implementation
FishAPI::GETHandler::GETHandler(FishAPI* api) : m_api(api) {
}

void FishAPI::onPHSample(float pH, float voltage, int16_t adcValue) {
    m_currentPH.store(pH);
    m_currentPHVoltage.store(voltage);
    m_currentPHAdcValue.store(adcValue);
    m_lastPHReadTime = std::time(nullptr);
    std::cout << "pH Sensor Reading - pH: " << pH 
              << ", Voltage: " << voltage 
              << ", ADC Value: " << adcValue << std::endl;
}

std::string FishAPI::GETHandler::getJSONString() {
    Json::Value root;
    Json::Value data;
    
    data["motor_initialized"] = (m_api->m_motor != nullptr && m_api->m_motor->isInitialized());
    data["ph_sensor_initialized"] = (m_api->m_phSensor != nullptr && m_api->m_phSensor->isInitialized());
    data["feed_count"] = m_api->m_feedCount.load();
    data["auto_feed_count"] = m_api->m_autoFeedCount.load();
    data["fish_detected"] = m_api->m_fishDetected.load();
    data["last_image"] = m_api->m_lastImagePath;
    data["auto_mode_enabled"] = m_api->m_autoModeEnabled.load();

    data["current_time"] = (long)time(NULL);
    if (m_api->m_lastFeedTime > 0) {
        char timeBuffer[100];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", 
                      std::localtime(&m_api->m_lastFeedTime));
        data["last_feed_time"] = timeBuffer;
    } else {
        data["last_feed_time"] = "Never";
    }
    
    if (m_api->m_AutolastFeedTime > 0) {
        char timeBuffer[100];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", 
                      std::localtime(&m_api->m_AutolastFeedTime));
        data["auto_last_feed_time"] = timeBuffer;
    } else {
        data["auto_last_feed_time"] = "Never";
    }
    
    data["current_ph"] = m_api->m_currentPH.load();
    data["current_ph_voltage"] = m_api->m_currentPHVoltage.load();
    data["current_ph_adc_value"] = m_api->m_currentPHAdcValue.load();
    
    if (m_api->m_lastPHReadTime > 0) {
        char phTimeBuffer[100];
        std::strftime(phTimeBuffer, sizeof(phTimeBuffer), "%Y-%m-%d %H:%M:%S", 
                      std::localtime(&m_api->m_lastPHReadTime));
        data["last_ph_read_time"] = phTimeBuffer;
    } else {
        data["last_ph_read_time"] = "Never";
    }
    
    root["success"] = true;
    root["data"] = data;
    
    Json::StreamWriterBuilder builder;
    return Json::writeString(builder, root);
}

// POST Handler implementation
FishAPI::POSTHandler::POSTHandler(FishAPI* api) : m_api(api) {
}

void FishAPI::POSTHandler::postString(std::string postArg) {
    std::cout << "Received POST data: " << postArg << std::endl;
    
    Json::Value root;
    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    JSONCPP_STRING err;
    
    if (!reader->parse(postArg.c_str(), postArg.c_str() + postArg.length(), &root, &err)) {
        std::cerr << "Error parsing JSON: " << err << std::endl;
        return;
    }
    
    if (!root.isMember("command")) {
        std::cerr << "No command specified" << std::endl;
        return;
    }
    
    std::string command = root["command"].asString();
    
    if (command == "run_motor") {
        int dutyCycle = root.get("duty_cycle", 100).asInt();
        int duration = root.get("duration", 1000).asInt();
        int period = root.get("period", 10).asInt();
        
        dutyCycle = std::max(0, std::min(100, dutyCycle));
        duration = std::max(100, std::min(10000, duration));
        period = std::max(5, std::min(20, period));
        
        std::cout << "Running motor: duty_cycle=" << dutyCycle 
                  << ", duration=" << duration 
                  << ", period=" << period << std::endl;
                  
        if (m_api->m_motor && m_api->m_motor->isInitialized()) {
            m_api->m_motor->run(dutyCycle, period, duration);
        } else {
            std::cerr << "Motor not initialized" << std::endl;
        }
    }
    else if (command == "feed_fish") {
        bool override = root.get("override", false).asBool();
        m_api->feedFish(override); 
    }
    else if (command == "read_ph") {
        std::cout << "On-demand pH reading requested" << std::endl;
        float ph = m_api->requestPHReading();
        if (ph >= 0) {
            std::cout << "pH reading successful: " << ph << std::endl;
        } else {
            std::cerr << "pH reading failed" << std::endl;
        }
    }
    else if (command == "init_ph_sensor") {
        std::cout << "Manual pH sensor initialization requested" << std::endl;
        if (m_api->m_phSensor) {
            bool success = m_api->m_phSensor->initialize();
            std::cout << "pH sensor initialization " << (success ? "successful" : "failed") << std::endl;
        } else {
            std::cerr << "pH sensor is NULL" << std::endl;
        }
    }
    else if (command == "set_auto_mode") {
        if (root.isMember("enabled")) {
            bool enabled = root["enabled"].asBool();
            m_api->m_autoModeEnabled = enabled;
            if (enabled) {
                m_api->m_pirSensor->start();
                m_api->m_camera.start();
            } else {
                m_api->m_pirSensor->stop();
                m_api->m_camera.stop();
            }
            std::cout << "Auto mode set to: " << (enabled ? "enabled" : "disabled") << std::endl;
        } else {
            std::cerr << "Missing 'enabled' parameter for set_auto_mode command" << std::endl;
        }
    }
    else {
        std::cerr << "Unknown command: " << command << std::endl;
    }
}
