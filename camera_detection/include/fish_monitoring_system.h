#ifndef FISH_MONITORING_SYSTEM_H
#define FISH_MONITORING_SYSTEM_H

#include "camera.h"
#include "feeder.h"
#include "image_processor.h"
#include "pir_sensor.h"
#include <memory>

/**
 * Main system class that connects all components
 */
class FishMonitoringSystem : public PirSensor::MotionCallbackInterface {
public:
    FishMonitoringSystem();
    ~FishMonitoringSystem();
    
    // Start the system
    void start();
    
    // Stop the system
    void stop();
    
    // Motion detected callback (implements MotionCallbackInterface)
    void motionDetected(gpiod_line_event e) override;
    
private:
    void clearArchive();
    
    std::unique_ptr<PirSensor> m_pirSensor;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<ImageProcessor> m_imageProcessor;
    std::unique_ptr<Feeder> m_feeder;
};

#endif // FISH_MONITORING_SYSTEM_H
