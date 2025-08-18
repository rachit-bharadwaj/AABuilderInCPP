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

### Windows

1. Clone or download this repository
2. **Download bundletool.jar**:
   - Option A: Run `download-bundletool.bat` to automatically download it
   - Option B: Manually download from https://github.com/google/bundletool/releases and place it in the project root
3. Open Command Prompt in the project directory
4. Run the build script:
   ```cmd
   build.bat
   ```
   
   **Note:** FLTK will be automatically downloaded and installed during the first build.

### Manual Build (All Platforms)

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

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
