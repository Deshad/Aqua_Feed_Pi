#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <cstdint>

class PHSensor {
public:
    // Interface for pH sensor callbacks
    struct PHSensorCallbackInterface {
        /**
         * Called when a new pH sample is available
         * @param pH Calculated pH value
         * @param voltage Raw voltage reading
         * @param adcValue Raw ADC value
         */
        virtual void onPHSample(float pH, float voltage, int16_t adcValue) = 0;
    };

    /**
     * Constructor
     * @param i2cDevice I2C device path
     * @param i2cAddr I2C address of the ADS1115
     * @param vRef Reference voltage
     * @param slope pH calibration slope
     * @param offset pH calibration offset
     */
    PHSensor(const char* i2cDevice = "/dev/i2c-1", 
             uint8_t i2cAddr = 0x48, 
             float vRef = 4.096, 
             float slope = -5.70, 
             float offset = 21.34);

    /**
     * Destructor
     */
    ~PHSensor();

    /**
     * Register a callback for pH sample events
     * @param callback Pointer to callback interface
     */
    void registerCallback(PHSensorCallbackInterface* callback);

    /**
     * Start continuous pH monitoring
     */
    void start();

    /**
     * Stop pH monitoring
     */
    void stop();

    /**
     * Trigger a single pH reading
     * @return pH value, or -1 if reading failed
     */
    float readPH();

private:
    // Worker thread function
    void worker();

    // Read ADC value from the sensor
    int16_t readADC();

    // Convert ADC value to voltage
    float adcToVoltage(int16_t adcValue);

    // Convert voltage to pH
    float voltageToPH(float voltage);

    // Notify all registered callbacks
    void notifyCallbacks(float pH, float voltage, int16_t adcValue);

    // Callback interfaces
    std::vector<PHSensorCallbackInterface*> m_callbackInterfaces;

    // Configuration constants
    const char* I2C_DEVICE;
    const uint8_t I2C_ADDR;
    const float V_REF;
    const float SLOPE;
    const float OFFSET;

    // Thread and file descriptor management
    std::thread m_thread;
    std::atomic<bool> m_running;
    int m_fd;
};

#endif // PH_SENSOR_H
