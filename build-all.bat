@echo off
echo ========================================
echo React Native Build Automator - Complete Build
echo ========================================
echo.

echo This script will build the application AND create an installer in one go.
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
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DOPTION_USE_SYSTEM_LIBJPG=OFF -DOPTION_USE_SYSTEM_LIBPNG=OFF -DOPTION_USE_SYSTEM_ZLIB=OFF ..
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

cd ..

echo.
echo ========================================
echo [SUCCESS] Application built successfully!
echo ========================================
echo.

REM Now check for NSIS and create installer
echo Checking NSIS availability for installer creation...
where makensis >nul 2>&1
if errorlevel 1 (
    echo.
    echo [INFO] NSIS not found - cannot create installer automatically.
    echo.
    echo To create an installer, please:
    echo 1. Download NSIS from: https://nsis.sourceforge.io/Download
    echo 2. Install it and add to PATH
    echo 3. Run: build-installer.bat
    echo.
    echo For now, you can use the application from the build folder:
    echo - Executable: build\RNBuildAutomator.exe
    echo - Required: bundletool.jar (already in build folder)
    echo.
    pause
    exit /b 0
)

echo [OK] NSIS found - proceeding with installer creation
echo.

REM Create LICENSE.txt if it doesn't exist
if not exist "LICENSE.txt" (
    echo Creating LICENSE.txt...
    echo MIT License > LICENSE.txt
    echo. >> LICENSE.txt
    echo Copyright (c) 2024 React Native Build Automator >> LICENSE.txt
    echo. >> LICENSE.txt
    echo Permission is hereby granted, free of charge, to any person obtaining a copy >> LICENSE.txt
    echo of this software and associated documentation files (the "Software"), to deal >> LICENSE.txt
    echo in the Software without restriction, including without limitation the rights >> LICENSE.txt
    echo to use, copy, modify, merge, publish, distribute, sublicense, and/or sell >> LICENSE.txt
    echo copies of the Software, and to permit persons to whom the Software is >> LICENSE.txt
    echo furnished to do so, subject to the following conditions: >> LICENSE.txt
    echo. >> LICENSE.txt
    echo The above copyright notice and this permission notice shall be included in all >> LICENSE.txt
    echo copies or substantial portions of the Software. >> LICENSE.txt
    echo. >> LICENSE.txt
    echo THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR >> LICENSE.txt
    echo IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, >> LICENSE.txt
    echo FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE >> LICENSE.txt
    echo AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER >> LICENSE.txt
    echo LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, >> LICENSE.txt
    echo OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE >> LICENSE.txt
    echo SOFTWARE. >> LICENSE.txt
)

REM Create icon.ico if it doesn't exist (placeholder)
if not exist "icon.ico" (
    echo [WARNING] icon.ico not found - installer will use default icon
    echo You can add a custom icon.ico file to customize the installer appearance
    echo.
)

REM Build the installer
echo Building installer with NSIS...
makensis installer.nsi

if errorlevel 1 (
    echo [ERROR] Installer build failed!
    echo Check the NSIS output above for errors.
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo [SUCCESS] Everything completed successfully!
echo ========================================
echo.
echo You now have:
echo - Built application in build\ folder
echo - Professional installer: RNBuildAutomator-Setup.exe
echo.
echo To distribute your application:
echo 1. Share RNBuildAutomator-Setup.exe with users
echo 2. Users run the installer to install to their system
echo 3. Application will be available in Start Menu and Desktop
echo.
pause
