#include "fish_monitoring_system.h"
#include <iostream>

int main() {
    try {
        // Create and start system
        FishMonitoringSystem system;
        system.start();
        
        std::cout << "System is running. Press Enter to exit." << std::endl;
        std::cin.get();
        
        system.stop();
        std::cout << "System shut down. Goodbye!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
