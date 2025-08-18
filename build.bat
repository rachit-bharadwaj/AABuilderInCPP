@echo off
echo Building React Native Build Automator with FLTK...
echo.

REM Check if bundletool.jar exists
if not exist "bundletool.jar" (
    echo [ERROR] bundletool.jar not found in project root!
    echo Please download bundletool.jar from: https://github.com/google/bundletool/releases
    echo and place it in the same directory as this build script.
    echo.
    pause
    exit /b 1
)

echo [INFO] bundletool.jar found - proceeding with build
echo.

REM Check if CMake is available
cmake --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found!
    echo Please install CMake from: https://cmake.org/download/
    echo Make sure to add it to your system PATH
    echo.
    pause
    exit /b 1
)

REM Check if FLTK is already installed
if exist "C:\fltk" (
    echo [INFO] FLTK already installed at C:\fltk - skipping installation
    echo.
    goto :build_app
)

REM Check if we have git
git --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Git not found!
    echo Please install Git from: https://git-scm.com/download/win
    echo Or manually download FLTK from: https://www.fltk.org/software.php
    echo.
    pause
    exit /b 1
)

echo FLTK not found. Installing FLTK...
echo.

echo Cloning FLTK from GitHub...
git clone https://github.com/fltk/fltk.git
if errorlevel 1 (
    echo [ERROR] Failed to clone FLTK repository
    pause
    exit /b 1
)

echo Building FLTK...
cd fltk
mkdir build
cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DOPTION_USE_SYSTEM_LIBJPEG=OFF -DOPTION_USE_SYSTEM_LIBPNG=OFF -DOPTION_USE_SYSTEM_ZLIB=OFF ..
if errorlevel 1 (
    echo [ERROR] FLTK CMake configuration failed
    cd ..\..
    pause
    exit /b 1
)

cmake --build . --config Release
if errorlevel 1 (
    echo [ERROR] FLTK build failed
    cd ..\..
    pause
    exit /b 1
)

echo Installing FLTK...
cmake --install . --prefix C:\fltk
if errorlevel 1 (
    echo [ERROR] FLTK installation failed
    cd ..\..
    pause
    exit /b 1
)

cd ..\..
echo FLTK installed successfully at C:\fltk
echo.

echo Cleaning up FLTK source files...
if exist fltk (
    rmdir /s /q fltk 2>nul
    if errorlevel 1 (
        echo Note: Could not remove FLTK source directory automatically
        echo You can manually delete the 'fltk' folder if desired
    ) else (
        echo FLTK source files removed from project directory
    )
)
echo.

:build_app
REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
echo Configuring project with CMake...
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:\fltk ..
if errorlevel 1 (
    echo Error: CMake configuration failed!
    echo Make sure FLTK is properly installed at C:\fltk
    echo.
    pause
    exit /b 1
)

REM Build the project
echo Building project...
cmake --build . --config Release
if errorlevel 1 (
    echo Error: Build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo Executable location: build\RNBuildAutomator.exe
echo.

REM Copy bundletool.jar to build directory for development
echo Copying bundletool.jar to build directory...
copy "..\bundletool.jar" "bundletool.jar" >nul
if errorlevel 1 (
    echo [WARNING] Failed to copy bundletool.jar to build directory
) else (
    echo [INFO] bundletool.jar copied to build directory
)

echo.
pause
