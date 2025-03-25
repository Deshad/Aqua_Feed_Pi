#include "ph_sensor.h"
#include <iostream>
#include <thread>
#include <chrono>

PHSensor::PHSensor(const char* i2cDevice, 
                   uint8_t i2cAddr, 
                   float vRef, 
                   float slope, 
                   float offset)
    : I2C_DEVICE(i2cDevice), 
      I2C_ADDR(i2cAddr), 
      V_REF(vRef), 
      SLOPE(slope), 
      OFFSET(offset),
      m_running(false),
      m_fd(-1) {
}

PHSensor::~PHSensor() {
    stop();
    
    // Close file descriptor if open
    if (m_fd >= 0) {
        close(m_fd);
    }
}

void PHSensor::registerCallback(PHSensorCallbackInterface* callback) {
    m_callbackInterfaces.push_back(callback);
}

void PHSensor::start() {
    if (m_running) return;

    // Open I2C device
    m_fd = open(I2C_DEVICE, O_RDWR);
    if (m_fd < 0) {
        std::cerr << "Failed to open I2C device" << std::endl;
        return;
    }

    // Set I2C slave address
    if (ioctl(m_fd, I2C_SLAVE, I2C_ADDR) < 0) {
        std::cerr << "Failed to set I2C address" << std::endl;
        close(m_fd);
        m_fd = -1;
        return;
    }

    // Start the monitoring thread
    m_running = true;
    m_thread = std::thread(&PHSensor::worker, this);
}

void PHSensor::stop() {
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

float PHSensor::readPH() {
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

    return pH;
}

void PHSensor::worker() {
    std::cout << "pH Sensor thread started" << std::endl;

    // Take readings every 5 seconds in thread
    while (m_running) {
        readPH();
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    std::cout << "pH Sensor thread stopped" << std::endl;
}

int16_t PHSensor::readADC() {
    if (m_fd < 0) {
        std::cerr << "I2C device not initialized" << std::endl;
        return -1;
    }

    // Configuration: Single-shot, ±4.096V gain, 128 SPS, A0 single-ended
    uint16_t config = (1 << 15) | // Start conversion
                      (0 << 12) | // A0 single-ended (MUX[14:12] = 000)
                      (1 << 9)  | // ±4.096V gain (PGA[11:9] = 001)
                      (0 << 8)  | // Single-shot mode
                      (4 << 5)  | // 128 samples per second (DR[7:5] = 100)
                      (0 << 4)  | // Traditional comparator
                      (0 << 3)  | // Polarity
                      (0 << 2)  | // Latching
                      (3);        // Disable comparator (COMP_QUE[1:0] = 11)

    // Write config register (big-endian)
    uint8_t config_bytes[3] = {0x01, (config >> 8) & 0xFF, config & 0xFF};
    if (write(m_fd, config_bytes, 3) != 3) {
        std::cerr << "Failed to write config" << std::endl;
        return -1;
    }

    // Wait for conversion
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Read conversion register
    uint8_t reg = 0x00;
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
    // ADS1115: 16-bit signed, ±32768 range for ±V_REF
    return (adcValue * V_REF) / 32767.0;
}

float PHSensor::voltageToPH(float voltage) {
    return SLOPE * voltage + OFFSET;
}

void PHSensor::notifyCallbacks(float pH, float voltage, int16_t adcValue) {
    for (auto& callback : m_callbackInterfaces) {
        callback->onPHSample(pH, voltage, adcValue);
    }
}
