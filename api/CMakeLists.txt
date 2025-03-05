cmake_minimum_required(VERSION 3.10)
project(MotorControlAPI)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
find_package(Boost REQUIRED COMPONENTS system)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${JSONCPP_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    /path/to/fastcgi_json_cpp_api  # Update this with the correct path
)

# Add the executable
add_executable(motor_api main.cpp)

# Link libraries
target_link_libraries(motor_api
    ${JSONCPP_LIBRARIES}
    ${Boost_LIBRARIES}
    fcgi
    jsoncpp
    pthread
    gpiod
)

# Set install destination
install(TARGETS motor_api DESTINATION bin)
