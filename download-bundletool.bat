@echo off
echo Downloading bundletool.jar...
echo.

REM Check if curl is available
curl --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] curl not found!
    echo Please install curl or manually download bundletool.jar from:
    echo https://github.com/google/bundletool/releases
    echo.
    pause
    exit /b 1
)

REM Download the latest bundletool.jar
echo Downloading latest bundletool.jar...
curl -L -o bundletool.jar "https://github.com/google/bundletool/releases/latest/download/bundletool-all-1.17.2.jar"

if errorlevel 1 (
    echo [ERROR] Failed to download bundletool.jar
    echo Please manually download from: https://github.com/google/bundletool/releases
    pause
    exit /b 1
)

echo.
echo [SUCCESS] bundletool.jar downloaded successfully!
echo You can now run build.bat to build the application.
echo.
pause
