cmake_minimum_required(VERSION 3.10)
project(fish_monitoring_system)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenCV REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add source files
set(SOURCES
    src/main.cpp
    src/pir_sensor.cpp
    src/camera.cpp
    src/image_processor.cpp
    src/feeder.cpp
    src/fish_monitoring_system.cpp
)

# Create executable
add_executable(fish_monitor ${SOURCES})

# Link libraries
target_link_libraries(fish_monitor
    ${OpenCV_LIBS}
    gpiod
    pthread
)

# Installation
install(TARGETS fish_monitor DESTINATION bin)
