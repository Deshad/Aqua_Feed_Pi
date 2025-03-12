#include <iostream>
#include <string>
#include <cstdlib>

int main() {
    std::string command = "libcamera-still -o captured_image.jpg";
    
    std::cout << "Capturing image using libcamera-still..." << std::endl;
    int result = system(command.c_str());
    
    if (result == 0) {
        std::cout << "Image captured successfully: captured_image.jpg" << std::endl;
        return 0;
    } else {
        std::cerr << "Failed to capture image. Error code: " << result << std::endl;
        return 1;
    }
}
