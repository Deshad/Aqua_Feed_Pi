#include <iostream>
#include <pigpio.h>
#include <functional>

#include "gpioevent.h"

class PWMController {
private:
    int pin;
    int dutyCycle;
    std::function<void()> callback;  // Callback function

public:
    PWMController(int pwmPin, int initialDuty = 128)
        : pin(pwmPin), dutyCycle(initialDuty), callback(nullptr) {
        
        if (gpioInitialise() < 0) {
            std::cerr << "GPIO initialization failed!\n";
            exit(1);
        }
        
        gpioSetMode(pin, PI_OUTPUT);
        setPWM(dutyCycle);
    }

    ~PWMController() {
        stopPWM();
        gpioTerminate();
    }

    void setPWM(int duty) {
        dutyCycle = duty;
        gpioPWM(pin, dutyCycle);
    }

    void stopPWM() {
        gpioPWM(pin, 0);
    }

    void setCallback(std::function<void()> cb) {
        callback = cb;
    }

    void run() {
        std::cout << "PWM running. Press Enter to stop...\n";
        std::cin.get();

        if (callback) {
            callback();  // Execute callback when stopping
        }

        stopPWM();
    }
};

void myCallback() {
    std::cout << "PWM stopped. Callback executed.\n";
}

int main() {
    PWMController pwm(18, 128);  // Use GPIO18 with 50% duty cycle
    pwm.setCallback(myCallback);
    pwm.run();
    return 0;
}
