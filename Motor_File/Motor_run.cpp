#include <gpiod.h>
#include <iostream>
#include <chrono>
#include <thread>

#define GPIO_CHIP "/dev/gpiochip0"  // Default GPIO chip on Raspberry Pi
#define MOTOR_PIN 4                // GPIO pin number (not BCM)

// Function to generate software PWM
void software_pwm(gpiod_line *line, int duty_cycle, int period_ms, int duration_ms) {
    auto start_time = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start_time < std::chrono::milliseconds(duration_ms)) {
        // Turn ON the motor (HIGH)
        gpiod_line_set_value(line, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(duty_cycle * period_ms / 100));

        // Turn OFF the motor (LOW)
        gpiod_line_set_value(line, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds((100 - duty_cycle) * period_ms / 100));
    }
}

int main() {
    gpiod_chip *chip = gpiod_chip_open(GPIO_CHIP);
    if (!chip) {
        std::cerr << "Failed to open GPIO chip!" << std::endl;
        return -1;
    }

    gpiod_line *line = gpiod_chip_get_line(chip, MOTOR_PIN);
    if (!line) {
        std::cerr << "Failed to get GPIO line!" << std::endl;
        gpiod_chip_close(chip);
        return -1;
    }

    if (gpiod_line_request_output(line, "motor_control", 0) < 0) {
        std::cerr << "Failed to request GPIO line as output!" << std::endl;
        gpiod_chip_close(chip);
        return -1;
    }

    std::cout << "Motor is running with software PWM!" << std::endl;

    while (true) {
        std::cout << "Setting PWM to 100% duty cycle (Full speed)" << std::endl;
        software_pwm(line, 100, 10, 2000);  // Full speed for 2 seconds

        std::cout << "Setting PWM to 50% duty cycle" << std::endl;
        software_pwm(line, 50, 10, 2000);   // 50% duty cycle for 2 seconds

        std::cout << "Motor OFF" << std::endl;
        software_pwm(line, 0, 10, 2000);    // Off for 2 seconds
    }

    gpiod_line_release(line);
    gpiod_chip_close(chip);
    return 0;
}

