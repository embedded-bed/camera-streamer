cmake_minimum_required(VERSION 3.20)

project(CameraStreamer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/bin)
set(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# Add packages
find_package(PkgConfig REQUIRED)

find_package(spdlog REQUIRED)

find_package(OpenCV REQUIRED COMPONENTS
    core
    imgproc
)

pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET
    gstreamer-1.0
    gstreamer-app-1.0
    gstreamer-video-1.0
)

# Add executable
add_executable(camera-streamer
    src/main.cpp
)

# Link to to libraries
target_include_directories(camera-streamer
    PRIVATE
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(camera-streamer
    PRIVATE
        spdlog::spdlog
        ${OpenCV_LIBS}
        PkgConfig::GSTREAMER
)