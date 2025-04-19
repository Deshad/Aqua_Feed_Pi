#ifndef PIR_SENSOR_H
#define PIR_SENSOR_H

#include <atomic>
#include <gpiod.h>
#include <thread>
#include <vector>

/**
 * PIR motion sensor interface class with callback
 */
class PirSensor {
public:
    // Interface for motion callbacks
    struct MotionCallbackInterface {
        virtual void motionDetected(gpiod_line_event e) = 0;
    };

    PirSensor(int chipNumber = 0, int pinNumber = 17);
    ~PirSensor();

    // Start motion detection thread
    void start();

    // Stop motion detection thread
    void stop();

    // Register callback for motion events
    void registerCallback(MotionCallbackInterface* callback);

private:
    // Worker thread function - blocks until motion detected
    void worker();

    // Process GPIO event
    void gpioEvent(gpiod_line_event& event);

    int m_chipNumber;
    int m_pinNumber;
    std::atomic<bool> m_running;
    std::thread m_thread;
    std::vector<MotionCallbackInterface*> m_callbacks;
    struct gpiod_chip* m_chip;
    struct gpiod_line* m_line;
};

#endif 
