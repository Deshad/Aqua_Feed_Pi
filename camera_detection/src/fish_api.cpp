#include "fish_api.h"
#include <jsoncpp/json/json.h>
#include <iostream>
#include <ctime>

// Constructor
FishAPI::FishAPI(Motor* motor, PHSensor* phSensor) 
    : m_motor(motor),
      m_phSensor(phSensor),
      m_running(false),
      m_getHandler(this),
      m_postHandler(this),
      m_fishDetected(false),
      m_feedCount(0),
      m_lastFeedTime(0),
      m_currentPH(0.0f),
      m_currentPHVoltage(0.0f),
      m_lastPHReadTime(0) {
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
        std::cout << "API thread started" << std::endl;
    }
}

// Stop the API
void FishAPI::stop() {
    if (m_running) {
        m_running = false;
        if (m_thread.joinable()) {
            m_handler.stop();  // Stop the handler first
            m_thread.join();
        }
        std::cout << "API thread stopped" << std::endl;
    }
}

// Update fish detection status
void FishAPI::setFishDetected(bool detected) {
    m_fishDetected = detected;
}

// Update last image path
void FishAPI::setLastImagePath(const std::string& path) {
    m_lastImagePath = path;
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
    
    if (ph < 0) {
        std::cerr << "Failed to read pH value" << std::endl;
        return -1.0f;
    }
    
    std::cout << "Successfully read pH value: " << ph << std::endl;
    return ph;
}

// Thread function
void FishAPI::threadFunction() {
    try {
        // Start the FastCGI handler with callbacks
        m_handler.start(&m_getHandler, &m_postHandler, "/tmp/fish_api.socket");
        
        // Keep thread alive until stopped
        while (m_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& e) {
        std::cerr << "API thread error: " << e.what() << std::endl;
    }
}

// GET Handler implementation
FishAPI::GETHandler::GETHandler(FishAPI* api) : m_api(api) {
}

void FishAPI::onPHSample(float pH, float voltage, int16_t adcValue) {
    // Update pH sensor data
    m_currentPH.store(pH);
    m_currentPHVoltage.store(voltage);
    m_currentPHAdcValue.store(adcValue);
    m_lastPHReadTime = std::time(nullptr);
    
    // Optional: Log or print pH data
    std::cout << "pH Sensor Reading - pH: " << pH 
              << ", Voltage: " << voltage 
              << ", ADC Value: " << adcValue << std::endl;
}

std::string FishAPI::GETHandler::getJSONString() {
    // Create JSON response
    Json::Value root;
    Json::Value data;
    
    // System information
    data["motor_initialized"] = (m_api->m_motor != nullptr && m_api->m_motor->isInitialized());
    data["ph_sensor_initialized"] = (m_api->m_phSensor != nullptr && m_api->m_phSensor->isInitialized());
    data["feed_count"] = m_api->m_feedCount.load();
    
    // Fish detection status
    data["fish_detected"] = m_api->m_fishDetected.load();
    data["last_image"] = m_api->m_lastImagePath;
    
    // Time information
    data["current_time"] = (long)time(NULL);
    
    if (m_api->m_lastFeedTime > 0) {
        char timeBuffer[100];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", 
                    std::localtime(&m_api->m_lastFeedTime));
        data["last_feed_time"] = timeBuffer;
    } else {
        data["last_feed_time"] = "Never";
    }

    // pH sensor information
    data["current_ph"] = m_api->m_currentPH.load();
    data["current_ph_voltage"] = m_api->m_currentPHVoltage.load();
    data["current_ph_adc_value"] = m_api->m_currentPHAdcValue.load();
    
    // pH read time
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
    
    // Convert to string
    Json::StreamWriterBuilder builder;
    const std::string json_response = Json::writeString(builder, root);
    return json_response;
}

// POST Handler implementation
FishAPI::POSTHandler::POSTHandler(FishAPI* api) : m_api(api) {
}

void FishAPI::POSTHandler::postString(std::string postArg) {
    std::cout << "Received POST data: " << postArg << std::endl;
    
    // Parse JSON
    Json::Value root;
    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    JSONCPP_STRING err;
    
    if (!reader->parse(postArg.c_str(), postArg.c_str() + postArg.length(), 
                      &root, &err)) {
        std::cerr << "Error parsing JSON: " << err << std::endl;
        return;
    }
    
    // Process commands
    if (!root.isMember("command")) {
        std::cerr << "No command specified" << std::endl;
        return;
    }
    
    std::string command = root["command"].asString();
    
    if (command == "run_motor") {
        // Extract parameters
        int dutyCycle = 100; // Default
        int duration = 1000; // Default
        int period = 10;     // Default
        
        if (root.isMember("duty_cycle")) dutyCycle = root["duty_cycle"].asInt();
        if (root.isMember("duration")) duration = root["duration"].asInt();
        if (root.isMember("period")) period = root["period"].asInt();
        
        // Apply safety limits
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
        bool override = false;
        if (root.isMember("override")) override = root["override"].asBool();
        
        // Only feed if fish detected or override is true
        if (m_api->m_fishDetected || override) {
            std::cout << "Feeding fish..." << std::endl;
            
            if (m_api->m_motor && m_api->m_motor->isInitialized()) {
                // Run feeding sequence
                m_api->m_motor->run(100, 10, 1000);  // Full speed for 1 second
                m_api->m_motor->run(50, 10, 500);    // Half speed for 0.5 seconds
                m_api->m_motor->stop();
                
                // Update feed count and time
                m_api->m_feedCount++;
                m_api->m_lastFeedTime = std::time(nullptr);
            } else {
                std::cerr << "Motor not initialized" << std::endl;
            }
        } else {
            std::cout << "Feed command ignored - no fish detected and override not set" << std::endl;
        }
    }
    else if (command == "read_ph") {
        // This is the new command to read pH on demand
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
    else {
        std::cerr << "Unknown command: " << command << std::endl;
    }
}
