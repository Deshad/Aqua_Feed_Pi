cmake_minimum_required(VERSION 3.0)
project(camera_detection)

set(CMAKE_CXX_STANDARD 17)

# Find OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Add executable
add_executable(camera_detector src/main.cpp)

# Link libraries
target_link_libraries(camera_detector ${OpenCV_LIBS})

# Find libgpiod
find_library(GPIOD_LIBRARY gpiod REQUIRED)

# Link against the libraries
target_link_libraries(camera_detector 
    ${OpenCV_LIBS}
    ${GPIOD_LIBRARY}
    pthread
)
