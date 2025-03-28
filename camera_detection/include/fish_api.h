#ifndef FISH_API_H
#define FISH_API_H

#include "json_fastcgi_web_api.h"
#include "motor.h"
#include "ph_sensor.h"
#include <thread>
#include <atomic>
#include <string>

class FishAPI : public PHSensor::PHSensorCallbackInterface {
public:
    FishAPI(Motor* motor,PHSensor* phSensor);
    ~FishAPI();

    // Start the API in a separate thread
    void start();
    
    // Stop the API
    void stop();
    
    // Update fish detection status
    void setFishDetected(bool detected);
    
    // Update last image path
    void setLastImagePath(const std::string& path);
    
    //Request Ph Sampling
    float requestPHReading();

    // Implement pH sensor callback
    void onPHSample(float pH, float voltage, int16_t adcValue) override;

private:
    // GET and POST callback implementations
    class GETHandler : public JSONCGIHandler::GETCallback {
    public:
        GETHandler(FishAPI* api);
        std::string getJSONString() override;
    private:
        FishAPI* m_api;
    };

    class POSTHandler : public JSONCGIHandler::POSTCallback {
    public:
        POSTHandler(FishAPI* api);
        void postString(std::string postArg) override;
    private:
        FishAPI* m_api;
    };

    // Thread function
    void threadFunction();

    // Member variables
    Motor* m_motor;
    PHSensor* m_phSensor;
    std::atomic<bool> m_running;
    std::thread m_thread;
    JSONCGIHandler m_handler;
    GETHandler m_getHandler;
    POSTHandler m_postHandler;
    
    // Status information
    std::atomic<bool> m_fishDetected;
    std::string m_lastImagePath;
    std::atomic<int> m_feedCount;
    std::time_t m_lastFeedTime;

    // pH sensor data
    std::atomic<float> m_currentPH;
    std::atomic<float> m_currentPHVoltage;
    std::atomic<int16_t> m_currentPHAdcValue;
    std::time_t m_lastPHReadTime;
};

#endif // FISH_API_H
