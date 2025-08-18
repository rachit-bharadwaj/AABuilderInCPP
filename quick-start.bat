@echo off
echo ========================================
echo React Native Build Automator - Quick Start
echo ========================================
echo.

echo This script will guide you through building and creating an installer.
echo.

:check_dependencies
echo [STEP 1] Checking dependencies...
echo.

REM Check if bundletool.jar exists
if not exist "bundletool.jar" (
    echo [INFO] bundletool.jar not found. Downloading...
    call download-bundletool.bat
    if errorlevel 1 (
        echo [ERROR] Failed to download bundletool.jar
        pause
        exit /b 1
    )
) else (
    echo [OK] bundletool.jar found
)

echo.

:build_app
echo [STEP 2] Building the application...
echo.
call build.bat
if errorlevel 1 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.

:check_nsis
echo [STEP 3] Checking NSIS availability...
echo.
makensis /VERSION >nul 2>&1
if errorlevel 1 (
    echo [INFO] NSIS not found. Please install NSIS to create an installer.
    echo.
    echo Download NSIS from: https://nsis.sourceforge.io/Download
    echo After installation, run this script again.
    echo.
    echo For now, you can use the application from the build folder:
    echo - Executable: build\RNBuildAutomator.exe
    echo - Required: bundletool.jar (already in build folder)
    echo.
    pause
    exit /b 0
)

echo [OK] NSIS found
echo.

:build_installer
echo [STEP 4] Building installer...
echo.
call build-installer.bat
if errorlevel 1 (
    echo [ERROR] Installer build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo [SUCCESS] All steps completed!
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
