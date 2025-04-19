#ifndef FISH_API_H
#define FISH_API_H

#include "json_fastcgi_web_api.h"
#include "motor.h"
#include "ph_sensor.h"
#include "pir_sensor.h"
#include "image_processor.h"
#include "camera.h"
#include <thread>
#include <atomic>
#include <string>

class FishAPI : public PHSensor::PHSensorCallbackInterface, 
               public PirSensor::MotionCallbackInterface,
               public ImageProcessor::FishDetectionCallbackInterface {
public:
    FishAPI(Motor* motor, PHSensor* phSensor, PirSensor* pirSensor); 
    ~FishAPI();

    void start();
    void stop();
    void setFishDetected(bool detected);
    void setLastImagePath(const std::string& path);
    float requestPHReading();
    void feedFish(bool override);

    void onPHSample(float pH, float voltage, int16_t adcValue) override;
    void motionDetected(gpiod_line_event event) override; // Pass-by-value  PirSensor
    void fishDetected(const cv::Mat& image) override;
    void noFishDetected(const cv::Mat& image) override;

private:
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

    void threadFunction();

    Motor* m_motor;
    PHSensor* m_phSensor;
    PirSensor* m_pirSensor; // Changed to pointer, not owned by FishAPI
    Camera m_camera;
    ImageProcessor m_imageProcessor;
    std::atomic<bool> m_running;
    std::thread m_thread;
    JSONCGIHandler m_handler;
    GETHandler m_getHandler;
    POSTHandler m_postHandler;
    
    std::atomic<bool> m_fishDetected;
    std::string m_lastImagePath;
    std::atomic<int> m_feedCount;
    std::atomic<int> m_autoFeedCount;
    std::time_t m_lastFeedTime;
    std::time_t m_AutolastFeedTime;
    std::atomic<bool> m_autoModeEnabled;

    std::atomic<float> m_currentPH;
    std::atomic<float> m_currentPHVoltage;
    std::atomic<int16_t> m_currentPHAdcValue;
    std::time_t m_lastPHReadTime;
};

#endif 
