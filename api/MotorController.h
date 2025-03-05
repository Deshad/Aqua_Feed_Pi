#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <gpiod.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

class MotorController {
private:
    const char* GPIO_CHIP;
    int MOTOR_PIN;
    gpiod_chip *chip;
    gpiod_line *line;
    std::atomic<bool> running;
    std::atomic<int> dutyCycle; // 0-100%
    std::thread motorThread;
    std::mutex motorMutex;
    bool debugMode;

public:
    MotorController(const char* gpioChip = "/dev/gpiochip0", int motorPin = 4) 
        : GPIO_CHIP(gpioChip), MOTOR_PIN(motorPin), chip(nullptr), line(nullptr), 
          running(false), dutyCycle(0), debugMode(false) {
    }

    ~MotorController() {
        stop();
    }

    bool initialize() {
        chip = gpiod_chip_open(GPIO_CHIP);
        if (!chip) {
            std::cerr << "Failed to open GPIO chip! Running in debug mode." << std::endl;
            debugMode = true;
            return true; // Still return true to allow program to run
        }

        line = gpiod_chip_get_line(chip, MOTOR_PIN);
        if (!line) {
            std::cerr << "Failed to get GPIO line!" << std::endl;
            gpiod_chip_close(chip);
            chip = nullptr;
            debugMode = true;
            return true;
        }

        if (gpiod_line_request_output(line, "motor_control", 0) < 0) {
            std::cerr << "Failed to request GPIO line as output!" << std::endl;
            gpiod_chip_close(chip);
            chip = nullptr;
            line = nullptr;
            debugMode = true;
            return true;
        }

        return true;
    }

    void start() {
        if (running) return;

        running = true;
        motorThread = std::thread(&MotorController::pwmLoop, this);
    }

    void stop() {
        if (!running) return;

        running = false;
        if (motorThread.joinable()) {
            motorThread.join();
        }

        // Ensure motor is off
        if (line && !debugMode) {
            gpiod_line_set_value(line, 0);
        }
    }

    void setSpeed(int speed) {
        if (speed < 0) speed = 0;
        if (speed > 100) speed = 100;
        
        std::cout << "Motor speed set to: " << speed << "%" 
                  << (debugMode ? " (Debug mode - no hardware)" : "") << std::endl;
        
        dutyCycle = speed;
    }

    int getSpeed() const {
        return dutyCycle;
    }

    void cleanup() {
        stop();
        
        if (line) {
            gpiod_line_release(line);
            line = nullptr;
        }
        
        if (chip) {
            gpiod_chip_close(chip);
            chip = nullptr;
        }
    }

private:
    void pwmLoop() {
        const int period_ms = 10; // PWM period in ms
        
        while (running) {
            int currentDuty = dutyCycle;
            
            if (debugMode) {
                // In debug mode, just sleep
                std::this_thread::sleep_for(std::chrono::milliseconds(period_ms));
                continue;
            }
            
            if (currentDuty > 0) {
                // Turn ON the motor (HIGH)
                gpiod_line_set_value(line, 1);
                std::this_thread::sleep_for(std::chrono::milliseconds(currentDuty * period_ms / 100));
                
                // Turn OFF the motor (LOW) if not at 100%
                if (currentDuty < 100) {
                    gpiod_line_set_value(line, 0);
                    std::this_thread::sleep_for(std::chrono::milliseconds((100 - currentDuty) * period_ms / 100));
                }
            } else {
                // Motor OFF
                gpiod_line_set_value(line, 0);
                std::this_thread::sleep_for(std::chrono::milliseconds(period_ms));
            }
        }
    }
};

#endif // MOTOR_CONTROLLER_H
