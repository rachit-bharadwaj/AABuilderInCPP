@echo off
echo Building React Native Build Automator Installer...
echo.

REM Check if build exists
if not exist "build\RNBuildAutomator.exe" (
    echo [ERROR] Build not found! Please run build.bat first.
    pause
    exit /b 1
)

REM Check if bundletool.jar exists
if not exist "bundletool.jar" (
    echo [ERROR] bundletool.jar not found! Please run download-bundletool.bat first.
    pause
    exit /b 1
)

REM Check if NSIS is available
makensis /VERSION >nul 2>&1
if errorlevel 1 (
    echo [ERROR] NSIS (makensis) not found!
    echo.
    echo Please install NSIS from: https://nsis.sourceforge.io/Download
    echo Make sure to add it to your system PATH
    echo.
    echo After installation, run this script again.
    echo.
    pause
    exit /b 1
)

echo [INFO] NSIS found - proceeding with installer build
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
    pause
    exit /b 1
)

echo.
echo [SUCCESS] Installer built successfully!
echo [INFO] Installer location: RNBuildAutomator-Setup.exe
echo.
echo [INFO] You can now distribute RNBuildAutomator-Setup.exe
echo [INFO] Users can run it to install the application to their system
echo.
pause
