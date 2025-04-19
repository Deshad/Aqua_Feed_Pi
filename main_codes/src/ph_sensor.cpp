#include "ph_sensor.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <chrono>
#include <thread>

// ADS1115 settings
static const char *I2C_DEVICE = "/dev/i2c-1";
static const uint8_t I2C_ADDR = 0x48;
#define CONFIG_REG 0x01
#define CONVERSION_REG 0x00

// Updated calibration constants
static const float V_REF = 2.048; // PGA ±2.048V
static const float SLOPE = -12.5; //  slope
static const float OFFSET = 12.5; //  offset

PHSensor::PHSensor() : m_fd(-1) {
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
    // Making sure the sensor is initialized
    if (!isInitialized()) {
        std::cout << "Sensor not initialized, initializing now..." << std::endl;
        if (!initialize()) {
            std::cerr << "Cannot read pH: Sensor initialization failed" << std::endl;
            return -1.0f;
        }
    }
    
    // Read raw ADC value 
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
    
    // Print the same output 
    std::cout << "Raw ADC: " << adcValue << " | Voltage: " << voltage
              << "V | pH: " << pH << std::endl;
    
    return pH;
}

int16_t PHSensor::readADC() {
    uint16_t config = (1 << 15) | // Start conversion
                      (0 << 12) | // A0 single-ended (MUX = 000)
                      (2 << 9)  | // ±2.048V gain (PGA = 010)
                      (0 << 8)  | // Single-shot
                      (4 << 5)  | // 128 SPS
                      (0 << 4)  | // Comparator off
                      (3);        // Disable comparator
    
    uint8_t config_bytes[3] = {
        CONFIG_REG,
        static_cast<uint8_t>((config >> 8) & 0xFF),
        static_cast<uint8_t>(config & 0xFF)
    };
    
    if (write(m_fd, config_bytes, 3) != 3) {
        std::cerr << "Failed to write config" << std::endl;
        return -1;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Read back config 
    uint8_t reg = CONFIG_REG;
    if (write(m_fd, &reg, 1) != 1) {
        std::cerr << "Failed to set config register for read" << std::endl;
        return -1;
    }
    
    uint8_t config_read[2];
    if (read(m_fd, config_read, 2) != 2) {
        std::cerr << "Failed to read config" << std::endl;
        return -1;
    }
    
    uint16_t config_check = (config_read[0] << 8) | config_read[1];
    std::cout << "Config Register: 0x" << std::hex << config_check << std::dec << std::endl;
    
    // Read conversion register
    reg = CONVERSION_REG;
    if (write(m_fd, &reg, 1) != 1) {
        std::cerr << "Failed to set conversion register" << std::endl;
        return -1;
    }
    
    uint8_t data[2];
    if (read(m_fd, data, 2) != 2) {
        std::cerr << "Failed to read conversion" << std::endl;
        return -1;
    }
    
    // Combine 16-bit result (big-endian)
    return (data[0] << 8) | data[1];
}

float PHSensor::adcToVoltage(int16_t adcValue) {
    // Using the updated calculation with 2.048V reference
    return (adcValue * V_REF) / 32767.0; // 0-32767 maps to 0-2.048V
}

float PHSensor::voltageToPH(float voltage) {
    return SLOPE * voltage + OFFSET;
}

void PHSensor::notifyCallbacks(float pH, float voltage, int16_t adcValue) {
    for (auto& callback : m_callbackInterfaces) {
        callback->onPHSample(pH, voltage, adcValue);
    }
}
