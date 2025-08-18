# Quick Start Guide (FLTK Version)

## What You Have

A complete **React Native Build Automator** desktop application built with:
- **C++17** and **FLTK** for lightweight native UI
- **Cross-platform** support (Windows, macOS, Linux)
- **Completely FREE** - no paid licenses required
- **Auto-installing** - FLTK gets installed automatically

## Key Features

✅ **One-click AAB generation**  
✅ **One-click APK generation with signing**  
✅ **Lightweight FLTK-based UI**  
✅ **Real-time build progress logging**  
✅ **Automatic file management**  
✅ **Input validation and error handling**  

## Files Created

```
📁 prod-app-builder/
├── 🔧 CMakeLists.txt      # Build configuration (FLTK)
├── 🏗️ build.bat           # Windows build script (auto-installs FLTK)
├── ⚙️ setup.bat           # Dependency checker
├── 📚 README.md           # Full documentation
├── 🚀 QUICKSTART.md       # This file
├── 🚫 .gitignore          # Git ignore rules
└── 📂 src/
    ├── 🎯 main.cpp        # Application entry point (FLTK)
    ├── 🖥️ MainWindow.h     # UI header (FLTK)
    ├── 🖥️ MainWindow.cpp   # UI implementation (FLTK)
    ├── ⚡ BuildAutomator.h # Build logic header
    └── ⚡ BuildAutomator.cpp# Build logic implementation
```

## What It Does

### Your Current Workflow:
1. 📝 Code your React Native app
2. 💻 Open terminal, navigate to project
3. ⌨️ Type build commands manually
4. 📁 Hunt for output files
5. 🔐 Run signing commands manually
6. 📂 Move files to desired location

### With This App:
1. 📝 Code your React Native app
2. 🖱️ Open the desktop app
3. 📂 Select project folder
4. 🔐 Input keystore details
5. 🚀 Click "Build AAB" or "Build APK"
6. ☕ Wait and get notified when done!

## Next Steps

### 1. Install Prerequisites (Run once)
```cmd
setup.bat
```
This checks for Node.js, npm, and other dependencies.

### 2. Build the Application
```cmd
build.bat
```
This automatically downloads, installs FLTK, and compiles your desktop app.

### 3. Use Your App
1. Run `build\RNBuildAutomator.exe`
2. Configure your React Native project
3. Enjoy automated builds!

## Installation Requirements

| Software | Free? | Required For |
|----------|-------|--------------|
| FLTK | ✅ Yes | GUI Framework (auto-installed) |
| CMake | ✅ Yes | Building |
| MinGW-w64 | ✅ Yes | C++ Compiler |
| Node.js | ✅ Yes | React Native |
| Expo CLI | ✅ Yes | Building AAB |
| EAS CLI | ✅ Yes | Building AAB |
| Android SDK | ✅ Yes | APK Signing |

**Total Cost: $0** 🎉

## Why FLTK Instead of Qt?

- **Lighter**: Much smaller than Qt (fewer MB)
- **Faster**: Quicker compilation and startup
- **Simpler**: Easier to learn and maintain
- **Free**: No licensing restrictions whatsoever
- **Professional**: Still looks modern and clean
- **Auto-install**: Gets installed automatically during build

## Troubleshooting

If builds fail, check:
1. ✅ All prerequisites installed (run `setup.bat`)
2. ✅ CMake and MinGW in PATH
3. ✅ Valid React Native project selected
4. ✅ Correct keystore path and credentials
5. ✅ Internet connection (for FLTK download and EAS builds)

## Success! 

You now have a lightweight, fast desktop automation tool that will save you time on every React Native build. No more repetitive terminal commands!

---
*Built with modern C++ and FLTK - Professional, Fast, Free, Lightweight* 🚀
