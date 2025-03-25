#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <cstdint>
#include <chrono>
#include <thread>

// ADS1115 settings
static const char *I2C_DEVICE = "/dev/i2c-1";
static const uint8_t I2C_ADDR = 0x48;
#define CONFIG_REG 0x01
#define CONVERSION_REG 0x00

const float V_REF = 2.048; // PGA Â±2.048V
const float SLOPE = -12.5; // Temporary, based on 0.68V = pH 4, 0.44V = pH 7
const float OFFSET = 12.5;

int16_t readADC(int fd) {
    uint16_t config = (1 << 15) | // Start conversion
                      (0 << 12) | // A0 single-ended (MUX = 000)
                      (2 << 9)  | // Â±2.048V gain (PGA = 010)
                      (0 << 8)  | // Single-shot
                      (4 << 5)  | // 128 SPS
                      (0 << 4)  | // Comparator off
                      (3);        // Disable comparator
    uint8_t config_bytes[3] = {
        CONFIG_REG,
        static_cast<uint8_t>((config >> 8) & 0xFF),
        static_cast<uint8_t>(config & 0xFF)
    };
    if (write(fd, config_bytes, 3) != 3) {
        std::cerr << "Failed to write config" << std::endl;
        return -1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Read back config
    uint8_t reg = CONFIG_REG;
    if (write(fd, &reg, 1) != 1) {
        std::cerr << "Failed to set config register for read" << std::endl;
        return -1;
    }
    uint8_t config_read[2];
    if (read(fd, config_read, 2) != 2) {
        std::cerr << "Failed to read config" << std::endl;
        return -1;
    }
    uint16_t config_check = (config_read[0] << 8) | config_read[1];
    std::cout << "Config Register: 0x" << std::hex << config_check << std::dec << std::endl;

    // Read conversion
    reg = CONVERSION_REG;
    if (write(fd, &reg, 1) != 1) {
        std::cerr << "Failed to set conversion register" << std::endl;
        return -1;
    }
    uint8_t data[2];
    if (read(fd, data, 2) != 2) {
        std::cerr << "Failed to read conversion" << std::endl;
        return -1;
    }
    return (data[0] << 8) | data[1];
}

float adcToVoltage(int16_t adcValue) {
    return (adcValue * V_REF) / 32767.0; // 0-32767 maps to 0-2.048V
}

float voltageToPH(float voltage) {
    return SLOPE * voltage + OFFSET;
}

int main() {
    int fd = open(I2C_DEVICE, O_RDWR);
    if (fd < 0) { std::cerr << "Failed to open I2C" << std::endl; return 1; }
    if (ioctl(fd, I2C_SLAVE, I2C_ADDR) < 0) { std::cerr << "Failed to set address" << std::endl; close(fd); return 1; }
    std::cout << "pH Sensor Diagnostics Started..." << std::endl;
    while (true) {
        int16_t adcValue = readADC(fd);
        if (adcValue < 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        float voltage = adcToVoltage(adcValue);
        float pH = voltageToPH(voltage);
        std::cout << "Raw ADC: " << adcValue << " | Voltage: " << voltage 
                  << "V | pH: " << pH << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    close(fd);
    return 0;
}
