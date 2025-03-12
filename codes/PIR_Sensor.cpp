#include <iostream>
#include <gpiod.h>
#include <chrono>
#include <thread>

#define GPIO_CHIP "/dev/gpiochip0"  // GPIO chip device file
#define PIR_SENSOR_PIN 17           // GPIO pin number

int main() {
    // Open the GPIO chip
    gpiod_chip *chip = gpiod_chip_open(GPIO_CHIP);
    if (!chip) {
        std::cerr << "Failed to open GPIO chip" << std::endl;
        return 1;
    }

    // Get the line corresponding to the PIR sensor pin
    gpiod_line *line = gpiod_chip_get_line(chip, PIR_SENSOR_PIN);
    if (!line) {
        std::cerr << "Failed to get GPIO line" << std::endl;
        gpiod_chip_close(chip);
        return 1;
    }

    // Request the line as input
    if (gpiod_line_request_input(line, "pir_sensor") < 0) {
        std::cerr << "Failed to request GPIO line as input" << std::endl;
        gpiod_chip_close(chip);
        return 1;
    }

    while (true) {
        // Read the PIR sensor state
        int val = gpiod_line_get_value(line);
        if (val < 0) {
            std::cerr << "Failed to read GPIO value" << std::endl;
            break;
        }

        // Print the sensor value
        std::cout << "Sensor: " << val << std::endl;

        // Sleep for 100ms (10fps)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Clean up
    gpiod_line_release(line);
    gpiod_chip_close(chip);
    return 0;
}
