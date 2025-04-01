#include "pir_sensor.h"
#include <iostream>
#include <stdexcept>

PirSensor::PirSensor(int chipNumber, int pinNumber) 
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

PirSensor::~PirSensor() {
    stop();
    if (m_line) {
        gpiod_line_release(m_line);
    }
    if (m_chip) {
        gpiod_chip_close(m_chip);
    }
}

void PirSensor::start() {
    m_running = true;
    m_thread = std::thread(&PirSensor::worker, this);
}

void PirSensor::stop() {
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void PirSensor::registerCallback(MotionCallbackInterface* callback) {
    m_callbacks.push_back(callback);
}

void PirSensor::worker() {
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

void PirSensor::gpioEvent(gpiod_line_event& event) {
    // Only process rising edge (motion detected)
    if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
        std::cout << "Motion detected!" << std::endl;
        // Notify all registered callbacks
        for (auto& callback : m_callbacks) {
            callback->motionDetected(event);
        }
    }
}
