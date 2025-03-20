//sudo apt install i2c-tools
//sudo i2cdetect -y 1
//sudo apt update && sudo apt install build-essential g++ libi2c-dev

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
static const char *I2C_DEVICE = "/dev/i2c-1"; // I2C bus 1 on Pi 5
static const uint8_t I2C_ADDR = 0x48; // Default ADS1115 address
#define CONFIG_REG 0x01
#define CONVERSION_REG 0x00
#define PH_CHANNEL 0 // A0 input

// Calibration constants (adjust these based on your sensor)
const float V_REF = 4.096; // Gain setting ±4.096V
const float SLOPE = -5.70; // Example slope (calibrate this)
const float OFFSET = 21.34; // Example offset (calibrate this)

// Function to configure ADS1115 and read ADC value
int16_t readADC(int fd) {
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
    uint8_t config_bytes[3] = {CONFIG_REG, (config >> 8) & 0xFF, config & 0xFF};
    if (write(fd, config_bytes, 3) != 3) {
        std::cerr << "Failed to write config" << std::endl;
        return -1;
    }

    // Wait for conversion (128 SPS ~ 8ms, so 10ms is safe)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Read conversion register
    uint8_t reg = CONVERSION_REG;
    if (write(fd, &reg, 1) != 1) {
        std::cerr << "Failed to set conversion register" << std::endl;
        return -1;
    }
    uint8_t data[2];
    if (read(fd, data, 2) != 2) {
        std::cerr << "Failed to read conversion" << std::endl;
        return -1;
    }

    // Combine 16-bit result (big-endian)
    return (data[0] << 8) | data[1];
}

// Function to convert ADC value to voltage
float adcToVoltage(int16_t adcValue) {
    // ADS1115: 16-bit signed, ±32768 range for ±V_REF
    return (adcValue * V_REF) / 32767.0;
}

// Function to convert voltage to pH
float voltageToPH(float voltage) {
    return SLOPE * voltage + OFFSET;
}

int main() {
    // Open I2C device
    int fd = open(I2C_DEVICE, O_RDWR);
    if (fd < 0) {
        std::cerr << "Failed to open I2C device" << std::endl;
        return 1;
    }

    // Set I2C slave address
    if (ioctl(fd, I2C_SLAVE, I2C_ADDR) < 0) {
        std::cerr << "Failed to set I2C address" << std::endl;
        close(fd);
        return 1;
    }

    std::cout << "pH Sensor Reading Started..." << std::endl;

    while (true) {
        // Read raw ADC value
        int16_t adcValue = readADC(fd);
        if (adcValue < 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        // Convert to voltage
        float voltage = adcToVoltage(adcValue);

        // Convert to pH
        float pH = voltageToPH(voltage);

        // Output result
        std::cout << "ADC: " << adcValue << " | Voltage: " << voltage
                  << "V | pH: " << pH << std::endl;

        // Delay for 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    close(fd);
    return 0;
}
