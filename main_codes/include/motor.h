#ifndef MOTOR_H
#define MOTOR_H

#include <gpiod.h>

/**
 * Motor control class using software PWM on GPIO
 */
class Motor {
public:
    /**
     * @param motorPin The GPIO pin number connected to the motor controller
     * @param  Path to the GPIO chip 
     */
    Motor(int motorPin = 4, const char* chipPath = "/dev/gpiochip0");
    
    ~Motor();
    
    /**
     * Run the motor at specified duty cycle
     * @param dutyCycle Percentage (0-100) 
     * @param periodMs PWM period in milliseconds
     * @param durationMs How long to run in milliseconds
     * @return true if successful, false if GPIO not initialized
     */
    bool run(int dutyCycle, int periodMs, int durationMs);
    
    /**
     * Stop the motor immediately
     */
    void stop();
    
    /**
     * Check if motor GPIO is properly init
     * @return true if init, false otherwise
     */
    bool isInitialized() const { return m_gpioInitialized; }

private:
    int m_motorPin;
    gpiod_chip* m_chip;
    gpiod_line* m_motorLine;
    bool m_gpioInitialized;
};

#endif 
