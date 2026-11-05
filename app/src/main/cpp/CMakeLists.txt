cmake_minimum_required(VERSION 3.22.1)
project(qivengine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add engine core
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../../engine ${CMAKE_CURRENT_BINARY_DIR}/engine_build)

# Main shared library for Android app
add_library(qivengine SHARED
    bridge/jni_bridge.cpp
)

target_include_directories(qivengine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../engine/core/include
)

target_link_libraries(qivengine PRIVATE
    qivengine_core
    android
    log
)

target_compile_options(qivengine PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -fexceptions
    -frtti
)
