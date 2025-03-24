cmake_minimum_required(VERSION 3.10)
project(fish_monitoring_system)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenCV REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add source files (excluding main.cpp)
set(SOURCES
    src/pir_sensor.cpp
    src/camera.cpp
    src/image_processor.cpp
    src/motor.cpp
    src/feeder.cpp
    src/fish_monitoring_system.cpp
)

# Add main executables
add_executable(fish_monitor src/main.cpp ${SOURCES})
add_executable(motor_test_program src/motor_main.cpp src/motor.cpp)

# Link libraries to main executable
target_link_libraries(fish_monitor
    ${OpenCV_LIBS}
    gpiod
    pthread
    -lgpiod  # Ensure explicit linking to gpiod library
)

# Link libraries to motor test program
target_link_libraries(motor_test_program
    gpiod
    pthread
    -lgpiod
)

# Installation
install(TARGETS fish_monitor motor_test_program DESTINATION bin)
