#include <iostream>
#include <string>
#include <map>
#include <signal.h>
#include <unistd.h>
#include <jsoncpp/json/json.h>
#include "json_fastcgi_web_api.h" // Adjust path if needed
#include "MotorController.h"

// Global flag to indicate if the program is running
bool mainRunning = true;
MotorController* motorPtr = nullptr;

// Signal handler for graceful termination
void signalHandler(int sig) {
    if ((sig == SIGHUP) || (sig == SIGINT)) {
        std::cout << "Received signal to terminate." << std::endl;
        mainRunning = false;
    }
}

// Set up signal handlers
void setSignalHandlers() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signalHandler;
    
    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        perror("sigaction SIGHUP");
        exit(-1);
    }
    
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction SIGINT");
        exit(-1);
    }
}

/**
 * Custom GET callback handler for motor status
 */
class MotorStatusGETHandler : public JSONCGIHandler::GETCallback {
private:
    MotorController* motor;

public:
    MotorStatusGETHandler(MotorController* motor) : motor(motor) {}

    /**
     * Returns the JSON data with motor information
     */
    std::string getJSONString() override {
        Json::Value root;
        
        // Current time (epoch)
        root["epoch"] = (long)time(NULL);
        
        // Motor speed
        root["motor_speed"] = motor->getSpeed();
        
        // Convert to JSON string
        Json::StreamWriterBuilder writer;
        return Json::writeString(writer, root);
    }
};

/**
 * Custom POST callback handler for motor control
 */
class MotorControlPOSTHandler : public JSONCGIHandler::POSTCallback {
private:
    MotorController* motor;

public:
    MotorControlPOSTHandler(MotorController* motor) : motor(motor) {}

    /**
     * Process POST data to control the motor
     */
    void postString(std::string postArg) override {
        std::cout << "Received POST data: " << postArg << std::endl;
        
        // Parse the JSON data
        Json::Value data;
        Json::CharReaderBuilder builder;
        Json::CharReader* reader = builder.newCharReader();
        std::string errors;
        
        bool parsingSuccessful = reader->parse(
            postArg.c_str(),
            postArg.c_str() + postArg.size(),
            &data,
            &errors
        );
        delete reader;
        
        if (parsingSuccessful) {
            // Process motor speed command
            if (data.isMember("motor_speed")) {
                int speed = data["motor_speed"].asInt();
                std::cout << "Setting motor speed to: " << speed << "%" << std::endl;
                motor->setSpeed(speed);
            }
        } else {
            std::cout << "Failed to parse JSON: " << errors << std::endl;
        }
    }
};

int main(int argc, char *argv[]) {
    // Check if we have the right number of parameters
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }
    
    try {
        // Get the port from the arguments
        int port = std::stoi(argv[1]);
        
        // Create socket path
        std::string socketPath = "/tmp/fastcgisocket" + std::to_string(port);
        
        // Create and initialize motor controller
        MotorController motor;
        motorPtr = &motor;
        
        if (!motor.initialize()) {
            std::cerr << "Failed to initialize motor controller!" << std::endl;
            return 1;
        }
        
        // Start the motor PWM thread
        motor.start();
        
        // Create callback handlers
        MotorStatusGETHandler getHandler(&motor);
        MotorControlPOSTHandler postHandler(&motor);
        
        // Create a JSON CGI handler
        JSONCGIHandler jsonHandler;
        
        // Set up signal handlers for graceful termination
        setSignalHandlers();
        
        // Start the server with our handlers
        std::cout << "Starting server on socket: " << socketPath << std::endl;
        jsonHandler.start(&getHandler, &postHandler, socketPath.c_str());
        
        std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;
        
        // Main loop - keep running until signaled to stop
        while (mainRunning) {
            sleep(1);
        }
        
        // Clean shutdown
        std::cout << "Shutting down..." << std::endl;
        jsonHandler.stop();
        motor.cleanup();
        
        return 0;
    } catch (const char* e) {
        std::cerr << "Error: " << e << std::endl;
        return 1;
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
