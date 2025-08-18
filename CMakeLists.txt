cmake_minimum_required(VERSION 3.20)
project(RNBuildAutomator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# Set FLTK paths directly
set(FLTK_INCLUDE_DIR "C:/fltk/include")
set(FLTK_LIB_DIR "C:/fltk/lib")

# Set include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${FLTK_INCLUDE_DIR})

# Link FLTK libraries directly
target_link_libraries(${PROJECT_NAME} 
    ${FLTK_LIB_DIR}/libfltk.a
    ${FLTK_LIB_DIR}/libfltk_images.a
)

# Windows system libs needed by FLTK
target_link_libraries(${PROJECT_NAME}
    ws2_32
    gdiplus
    gdi32
    user32
    comctl32
    ole32
    oleaut32
    uuid
    shell32
    comdlg32
)

# Set target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
