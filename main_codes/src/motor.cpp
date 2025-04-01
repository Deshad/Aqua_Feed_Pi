#include "motor.h"
#include <iostream>
#include <chrono>
#include <thread>

Motor::Motor(int motorPin, const char* chipPath) 
    : m_motorPin(motorPin), 
      m_chip(nullptr), 
      m_motorLine(nullptr), 
      m_gpioInitialized(false) {
    
    // Initialize GPIO
    m_chip = gpiod_chip_open(chipPath);
    if (!m_chip) {
        std::cerr << "Failed to open GPIO chip for motor control!" << std::endl;
        return;
    }
    
    m_motorLine = gpiod_chip_get_line(m_chip, m_motorPin);
    if (!m_motorLine) {
        std::cerr << "Failed to get GPIO line for motor control!" << std::endl;
        gpiod_chip_close(m_chip);
        m_chip = nullptr;
        return;
    }
    
    if (gpiod_line_request_output(m_motorLine, "motor_control", 0) < 0) {
        std::cerr << "Failed to request GPIO line as output for motor control!" << std::endl;
        gpiod_chip_close(m_chip);
        m_chip = nullptr;
        m_motorLine = nullptr;
        return;
    }
    
    m_gpioInitialized = true;
    std::cout << "Motor initialized on GPIO pin " << m_motorPin << std::endl;
}

Motor::~Motor() {
    stop();
    
    if (m_motorLine) {
        gpiod_line_release(m_motorLine);
    }
    
    if (m_chip) {
        gpiod_chip_close(m_chip);
    }
}

bool Motor::run(int dutyCycle, int periodMs, int durationMs) {
    if (!m_gpioInitialized || !m_motorLine) {
        std::cerr << "Cannot run motor: GPIO not initialized" << std::endl;
        return false;
    }
    
    // Clamp duty cycle to 0-100 range
    dutyCycle = std::max(0, std::min(100, dutyCycle));
    
    std::cout << "Running motor at " << dutyCycle << "% duty cycle for " 
              << durationMs << "ms..." << std::endl;
    
    auto startTime = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - startTime < std::chrono::milliseconds(durationMs)) {
        if (dutyCycle > 0) {
            // Turn ON the motor (HIGH)
            gpiod_line_set_value(m_motorLine, 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(dutyCycle * periodMs / 100));
        }
        
        if (dutyCycle < 100) {
            // Turn OFF the motor (LOW)
            gpiod_line_set_value(m_motorLine, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds((100 - dutyCycle) * periodMs / 100));
        }
    }
    
    return true;
}

void Motor::stop() {
    if (m_gpioInitialized && m_motorLine) {
        gpiod_line_set_value(m_motorLine, 0);
        std::cout << "Motor stopped" << std::endl;
    }
}
