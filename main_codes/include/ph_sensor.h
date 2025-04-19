#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include <vector>
#include <cstdint>

class PHSensor {
public:
    // Interface for pH sensor callbacks
    struct PHSensorCallbackInterface {
        virtual void onPHSample(float pH, float voltage, int16_t adcValue) = 0;
    };

    PHSensor();
    ~PHSensor();

    void registerCallback(PHSensorCallbackInterface* callback);
    bool initialize();
    void cleanup();
    float readPH();
    bool isInitialized() const { return m_fd >= 0; }

private:
    int16_t readADC();
    float adcToVoltage(int16_t adcValue);
    float voltageToPH(float voltage);
    void notifyCallbacks(float pH, float voltage, int16_t adcValue);

    std::vector<PHSensorCallbackInterface*> m_callbackInterfaces;
    int m_fd;
};

#endif 
