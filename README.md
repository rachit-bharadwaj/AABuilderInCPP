# React Native Build Automator (FLTK Version)

A professional desktop application built with C++ and FLTK that automates the React Native Expo build process. This tool streamlines the workflow of creating AAB and APK files with proper signing, all through an intuitive graphical interface.

## Features

- **Professional FLTK-based UI** with modern appearance
- **One-click AAB generation** from React Native Expo projects
- **One-click APK generation** with automatic signing
- **Automatic keystore signing** with release keys
- **Real-time build progress** with detailed logging
- **File browser integration** for easy path selection
- **Cross-platform support** (Windows, macOS, Linux)
- **Completely free** - uses only open-source technologies
- **Lightweight** - small executable size and fast compilation

## Prerequisites

### Required Software

1. **CMake** (3.20 or higher)
   - Download from: https://cmake.org/download/
   - Make sure it's added to your system PATH

2. **MinGW-w64** (for Windows)
   - Download from: https://www.mingw-w64.org/
   - Or install via MSYS2: https://www.msys2.org/

3. **Node.js and npm**
   - Download from: https://nodejs.org/
   - Required for React Native development

4. **Expo CLI**
   ```bash
   npm install -g @expo/cli
   npm install -g eas-cli
   ```

5. **Android SDK** (for APK signing)
   - Install Android Studio or just the command line tools
   - Set ANDROID_HOME environment variable

6. **bundletool.jar** (for APK generation)
   - Download from: https://github.com/google/bundletool/releases
   - Or run `download-bundletool.bat` to automatically download it
   - Place `bundletool.jar` in the project root directory (same folder as `build.bat`)
   - This file is required for converting AAB to APK

### Environment Setup

1. **Add to PATH:**
   - MinGW bin directory: `C:\mingw64\bin` (or your MinGW path)
   - CMake bin directory

## Building the Application

### Windows - Single Command Build

**For the complete build (application + installer) in one command:**
```cmd
build-all.bat
```

This single command will:
1. ✅ Install FLTK (if not already installed)
2. ✅ Build the application
3. ✅ Create a professional installer (if NSIS is available)

### Windows - Step by Step

If you prefer to build step by step:

1. Clone or download this repository
2. **Download bundletool.jar**:
   - Option A: Run `download-bundletool.bat` to automatically download it
   - Option B: Manually download from https://github.com/google/bundletool/releases and place it in the project root
3. Open Command Prompt in the project directory
4. **Option A - Complete build**: Run `build-all.bat` (recommended)
5. **Option B - Step by step**:
   ```cmd
   build.bat                    # Builds the application
   build-installer.bat          # Creates the installer
   ```
    
**Note:** FLTK will be automatically downloaded and installed during the first build.

### Manual Build (All Platforms)

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Distribution

### Creating a Windows Installer

After building the application, you can create a professional Windows installer:

1. **Install NSIS** (Nullsoft Scriptable Install System):
   - Download from: https://nsis.sourceforge.io/Download
   - Add to your system PATH

2. **Build the installer:**
   ```cmd
   build-installer.bat
   ```

3. **The script will create:**
   - `RNBuildAutomator-Setup.exe` - Professional Windows installer

4. **Distribute the installer** - users can run it to install the application to their system

### What the Installer Does

- **Installs to Program Files**: Professional installation location
- **Creates Start Menu shortcuts**: Easy access from Windows Start Menu
- **Creates Desktop shortcut**: Quick access from desktop
- **Registry integration**: Proper Windows integration
- **Uninstaller**: Clean removal from Control Panel
- **Dependency checks**: Warns about missing Java/Android SDK
- **Automatic bundletool.jar**: Includes all required files

### Why This Approach?

- **Professional**: Looks and feels like commercial software
- **System integration**: Proper Windows installation standards
- **Easy distribution**: Single .exe file to share
- **User-friendly**: Familiar installation process
- **Clean uninstall**: Proper cleanup when removed

## Usage

### First Time Setup

1. **Launch the application**: Run `RNBuildAutomator.exe`

2. **Configure your project:**
   - **Project Path**: Select your React Native Expo project folder
   - **Output Path**: Choose where to save the generated AAB/APK files
   - **Keystore**: Select your Android release keystore file
   - **Keystore Password**: Enter your keystore password
   - **Key Alias**: Enter your key alias
   - **Key Password**: Enter your key password

3. **Choose build options:**
   - **Build Mode**: Select "release" for production or "debug" for testing
   - **Clean Build**: Check to clean the project before building

### Building Your App

1. **For AAB files**: Click "Build AAB"
2. **For APK files**: Click "Build APK"

The application will:
- Clean the project (if selected)
- Build the AAB using Expo EAS
- Convert AAB to APK (for APK builds)
- Sign the APK with your release key
- Move the final file to your output directory

### Build Process

The application executes the following steps automatically:

1. **Clean** (optional): `expo prebuild --clean`
2. **Build AAB**: `eas build --platform android --profile release --local`
3. **Convert to APK** (APK builds only): Using bundletool
4. **Sign APK** (APK builds only): Using apksigner
5. **Move to output directory**

## Project Structure

```
prod-app-builder/
├── src/
│   ├── main.cpp              # Application entry point
│   ├── MainWindow.h/.cpp     # Main UI window (FLTK)
│   └── BuildAutomator.h/.cpp # Build logic and process management
├── CMakeLists.txt            # CMake build configuration
├── build.bat                 # Windows build script (auto-installs FLTK)
├── setup.bat                 # Dependency checker
└── README.md                 # This file
```

## Troubleshooting

### Common Issues

1. **"bundletool.jar not found" error**
   - Ensure `bundletool.jar` is in the same directory as the executable
   - Download the latest version from: https://github.com/google/bundletool/releases
   - For development: place it in the project root directory
   - For distribution: ensure it's copied to the same folder as the `.exe` file

2. **Build fails with "command not found"**
   - Ensure Node.js and React Native CLI are installed and in PATH
   - Verify that `npx` command is available

3. **Android SDK errors**
   - Set ANDROID_HOME environment variable to your Android SDK path
   - Ensure Android SDK tools are properly installed

4. **FLTK compilation errors**
   - Delete the `build` folder and run `build.bat` again
   - Ensure MinGW-w64 is properly installed and in PATH

5. **Build script bundletool copy warnings**
   - This is normal during development builds
   - For distribution, use `build-installer.bat` to create a professional installer
   - The installer will place files in the correct system locations

6. **Installer build issues**
   - Ensure NSIS is installed and in PATH
   - Run `makensis /VERSION` to verify NSIS installation
   - Check that all required files exist before building installer

7. **Installation issues**
   - Run installer as Administrator if needed
   - Ensure antivirus doesn't block the installation
   - Check Windows Defender settings

## License

This project is open source and available under the MIT License.

## Contributing

This is a simple utility application. If you find bugs or want to add features:
1. Fork the repository
2. Make your changes
3. Test thoroughly
4. Submit a pull request

## Tech Stack

- **C++17** - Core language
- **FLTK** - Lightweight cross-platform GUI framework
- **CMake** - Build system
- **Expo/EAS** - React Native build tools
- **Android SDK Tools** - APK signing

## Why FLTK?

- **Lightweight**: Much smaller than Qt, faster compilation
- **Simple**: Easy to use and maintain
- **Fast**: Native performance, no overhead
- **Free**: No licensing costs or restrictions
- **Cross-platform**: Works on Windows, macOS, and Linux
- **Mature**: Stable and well-tested framework

## Support

For issues and questions:
1. Check the troubleshooting section above
2. Verify all prerequisites are installed correctly
3. Check that environment variables are set properly

---

**Built with ❤️ using free and open-source technologies**
