cmake_minimum_required(VERSION 3.14)
project(gesture_id)

set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC /usr/include/libcamera)
target_link_directories(${PROJECT_NAME} PUBLIC /usr/lib/x86_64-linux-gnu)
target_link_libraries(${PROJECT_NAME}
    libcamera.so
    libcamera-base.so
)