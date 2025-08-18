cmake_minimum_required(VERSION 3.20)
project(RNBuildAutomator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find FLTK
find_package(FLTK REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/BuildAutomator.cpp
)

set(HEADERS
    src/MainWindow.h
    src/BuildAutomator.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link FLTK libraries
target_link_libraries(${PROJECT_NAME} fltk)

# Set target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
