#include "fish_api.h"
#include <jsoncpp/json/json.h>
#include <iostream>
#include <ctime>

// Constructor
FishAPI::FishAPI(Motor* motor) 
    : m_motor(motor), 
      m_running(false),
      m_getHandler(this),
      m_postHandler(this),
      m_fishDetected(false),
      m_feedCount(0),
      m_lastFeedTime(0) {
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

std::string FishAPI::GETHandler::getJSONString() {
    // Create JSON response
    Json::Value root;
    Json::Value data;
    
    // System information
    data["motor_initialized"] = (m_api->m_motor != nullptr && m_api->m_motor->isInitialized());
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
    else {
        std::cerr << "Unknown command: " << command << std::endl;
    }
}
