#include "motor.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    int motorPin = 4;  // Default GPIO pin
    
    // Allow pin override via command line
    if (argc > 1) {
        try {
            motorPin = std::stoi(argv[1]);
        } catch (const std::exception& e) {
            std::cerr << "Invalid pin number: " << argv[1] << std::endl;
            return 1;
        }
    }
    
    std::cout << "Motor Test Program" << std::endl;
    std::cout << "==================" << std::endl;
    std::cout << "Using GPIO pin: " << motorPin << std::endl;
    
    try {
        // Create motor controller
        Motor motor(motorPin);
        
        if (!motor.isInitialized()) {
            std::cerr << "Failed to initialize motor on pin " << motorPin << std::endl;
            return 1;
        }
        
        // Run motor sequence
        std::cout << "\nMotor Test Sequence:" << std::endl;
        std::cout << "-------------------" << std::endl;
        
        std::cout << "100% duty cycle (Full speed) for 2 seconds" << std::endl;
        motor.run(100, 10, 2000);
        
        std::cout << "50% duty cycle for 2 seconds" << std::endl;
        motor.run(50, 10, 2000);
        
        std::cout << "25% duty cycle for 2 seconds" << std::endl;
        motor.run(25, 10, 2000);
        
        std::cout << "0% duty cycle (OFF) for 2 seconds" << std::endl;
        motor.run(0, 10, 2000);
        
        std::cout << "Ramp up sequence..." << std::endl;
        for (int i = 0; i <= 100; i += 10) {
            std::cout << i << "% duty cycle" << std::endl;
            motor.run(100, 10, 500);
        }
        
        
        
        std::cout << "Stopping motor..." << std::endl;
        motor.stop();
        
        std::cout << "\nTest complete!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
