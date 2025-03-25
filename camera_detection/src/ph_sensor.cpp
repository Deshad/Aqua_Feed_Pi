#include "ph_sensor.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <chrono>
#include <thread>

// ADS1115 settings - using static constants
static const char *I2C_DEVICE = "/dev/i2c-1"; // I2C bus 1 on Pi 5
static const uint8_t I2C_ADDR = 0x48; // Default ADS1115 address
#define CONFIG_REG 0x01
#define CONVERSION_REG 0x00
#define PH_CHANNEL 0 // A0 input

// Calibration constants
static const float V_REF = 4.096; // Gain setting ±4.096V
static const float SLOPE = -5.70; // Slope value
static const float OFFSET = 21.34; // Offset value

PHSensor::PHSensor() : m_fd(-1) {
    // Simple constructor with no parameters
}

PHSensor::~PHSensor() {
    cleanup();
}

void PHSensor::registerCallback(PHSensorCallbackInterface* callback) {
    m_callbackInterfaces.push_back(callback);
}

bool PHSensor::initialize() {
    // Close previous descriptor if open
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
    
    // Using the exact same code as your working example
    m_fd = open(I2C_DEVICE, O_RDWR);
    if (m_fd < 0) {
        std::cerr << "Failed to open I2C device" << std::endl;
        return false;
    }
    
    if (ioctl(m_fd, I2C_SLAVE, I2C_ADDR) < 0) {
        std::cerr << "Failed to set I2C address" << std::endl;
        close(m_fd);
        m_fd = -1;
        return false;
    }
    
    std::cout << "pH Sensor initialized successfully" << std::endl;
    return true;
}

void PHSensor::cleanup() {
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
        std::cout << "pH Sensor resources cleaned up" << std::endl;
    }
}

float PHSensor::readPH() {
    // Make sure the sensor is initialized
    if (!isInitialized()) {
        std::cout << "Sensor not initialized, initializing now..." << std::endl;
        if (!initialize()) {
            std::cerr << "Cannot read pH: Sensor initialization failed" << std::endl;
            return -1.0f;
        }
    }
    
    // Read raw ADC value - using exact code from your working example
    int16_t adcValue = readADC();
    if (adcValue < 0) {
        std::cerr << "Failed to read ADC" << std::endl;
        return -1.0f;
    }
    
    // Convert to voltage
    float voltage = adcToVoltage(adcValue);
    
    // Convert to pH
    float pH = voltageToPH(voltage);
    
    // Notify callbacks
    notifyCallbacks(pH, voltage, adcValue);
    
    // Print the same output as your working code
    std::cout << "ADC: " << adcValue << " | Voltage: " << voltage
              << "V | pH: " << pH << std::endl;
    
    return pH;
}

int16_t PHSensor::readADC() {
    // Using exact implementation from your working code
    uint16_t config = (1 << 15) | // Start conversion
                      (0 << 12) | // A0 single-ended (MUX[14:12] = 000)
                      (1 << 9)  | // ±4.096V gain (PGA[11:9] = 001)
                      (0 << 8)  | // Single-shot mode
                      (4 << 5)  | // 128 samples per second (DR[7:5] = 100)
                      (0 << 4)  | // Traditional comparator
                      (0 << 3)  | // Polarity
                      (0 << 2)  | // Latching
                      (3);        // Disable comparator (COMP_QUE[1:0] = 11)
    
    // Write config register (big-endian) - using exact same code
    uint8_t config_bytes[3] = {CONFIG_REG, 
                              static_cast<uint8_t>((config >> 8) & 0xFF), 
                              static_cast<uint8_t>(config & 0xFF)};
    if (write(m_fd, config_bytes, 3) != 3) {
        std::cerr << "Failed to write config" << std::endl;
        return -1;
    }
    
    // Wait for conversion - same timing as your working code
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Read conversion register - exact same code
    uint8_t reg = CONVERSION_REG;
    if (write(m_fd, &reg, 1) != 1) {
        std::cerr << "Failed to set conversion register" << std::endl;
        return -1;
    }
    
    uint8_t data[2];
    if (read(m_fd, data, 2) != 2) {
        std::cerr << "Failed to read conversion" << std::endl;
        return -1;
    }
    
    // Combine 16-bit result (big-endian) - same as your working code
    return (data[0] << 8) | data[1];
}

float PHSensor::adcToVoltage(int16_t adcValue) {
    // Using the exact same calculation as your working code
    return (adcValue * V_REF) / 32767.0;
}

float PHSensor::voltageToPH(float voltage) {
    // Using the exact same calculation as your working code
    return SLOPE * voltage + OFFSET;
}

void PHSensor::notifyCallbacks(float pH, float voltage, int16_t adcValue) {
    for (auto& callback : m_callbackInterfaces) {
        callback->onPHSample(pH, voltage, adcValue);
    }
}
