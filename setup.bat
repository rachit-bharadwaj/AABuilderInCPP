@echo off
echo React Native Build Automator - Setup Script (FLTK Version)
echo ========================================================
echo.

echo Checking for required dependencies...
echo.

REM Check for Node.js
node --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Node.js not found!
    echo Please install Node.js from: https://nodejs.org/
    echo.
) else (
    echo [OK] Node.js found
)

REM Check for npm
npm --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] npm not found!
) else (
    echo [OK] npm found
)

REM Check for expo
expo --version >nul 2>&1
if errorlevel 1 (
    echo [WARNING] Expo CLI not found
    echo Installing Expo CLI...
    npm install -g @expo/cli
    if errorlevel 1 (
        echo [ERROR] Failed to install Expo CLI
    ) else (
        echo [OK] Expo CLI installed successfully
    )
) else (
    echo [OK] Expo CLI found
)

REM Check for eas
eas --version >nul 2>&1
if errorlevel 1 (
    echo [WARNING] EAS CLI not found
    echo Installing EAS CLI...
    npm install -g eas-cli
    if errorlevel 1 (
        echo [ERROR] Failed to install EAS CLI
    ) else (
        echo [OK] EAS CLI installed successfully
    )
) else (
    echo [OK] EAS CLI found
)

REM Check for CMake
cmake --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found!
    echo Please install CMake from: https://cmake.org/download/
    echo Make sure to add it to your system PATH
    echo.
) else (
    echo [OK] CMake found
)

REM Check for FLTK
if exist "C:\fltk" (
    echo [OK] FLTK found at C:\fltk
) else (
    echo [INFO] FLTK not found - will be installed automatically during build
    echo.
)

REM Check for Android SDK
if not defined ANDROID_HOME (
    echo [WARNING] ANDROID_HOME environment variable not set
    echo This is required for APK signing
    echo Please install Android SDK and set ANDROID_HOME
    echo.
) else (
    if exist "%ANDROID_HOME%" (
        echo [OK] ANDROID_HOME found: %ANDROID_HOME%
    ) else (
        echo [ERROR] ANDROID_HOME path does not exist: %ANDROID_HOME%
        echo.
    )
)

echo.
echo Setup check completed!
echo.
echo Next steps:
echo 1. Fix any [ERROR] items shown above
echo 2. Run 'build.bat' to compile the application (FLTK will be installed automatically)
echo 3. Launch the application and configure your project
echo.
pause
