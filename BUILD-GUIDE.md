# Build Guide - React Native Build Automator

## 🚀 **Quick Start (Recommended)**

**One command to build everything:**
```cmd
build-all.bat
```

This single command will:
- ✅ Install FLTK (if needed)
- ✅ Build the application
- ✅ Create a professional installer (if NSIS is available)

## 📋 **Build Options**

### Option 1: Complete Build (Recommended)
```cmd
build-all.bat
```
- **What it does**: Everything in one command
- **Output**: Application + Installer (if NSIS available)
- **Best for**: Most users, complete automation

### Option 2: Step by Step
```cmd
build.bat                    # Step 1: Build application
build-installer.bat          # Step 2: Create installer
```
- **What it does**: Builds application first, then installer
- **Output**: Same as Option 1, but in two steps
- **Best for**: Users who want to see each step separately

### Option 3: Development Only
```cmd
build.bat
```
- **What it does**: Only builds the application
- **Output**: Just the executable in `build\` folder
- **Best for**: Developers who want to test the app locally

## 🔧 **Prerequisites**

### Required (automatically handled):
- ✅ **CMake** - Build system
- ✅ **MinGW-w64** - C++ compiler
- ✅ **Git** - For FLTK download
- ✅ **bundletool.jar** - For APK generation

### Optional (for installer):
- ⚠️ **NSIS** - For creating Windows installer
  - Download from: https://nsis.sourceforge.io/Download
  - Add to system PATH

## 📁 **Output Files**

### After `build-all.bat` (with NSIS):
```
build\RNBuildAutomator.exe          # Application executable
RNBuildAutomator-Setup.exe          # Professional installer
```

### After `build-all.bat` (without NSIS):
```
build\RNBuildAutomator.exe          # Application executable
build\bundletool.jar                # Required dependency
```

## 🎯 **What to Use When**

| Use Case | Command | Output |
|----------|---------|---------|
| **First time setup** | `build-all.bat` | Complete application + installer |
| **Regular development** | `build.bat` | Just the application |
| **Create installer later** | `build-installer.bat` | Just the installer |
| **Complete rebuild** | `build-all.bat` | Everything fresh |

## 🚨 **Troubleshooting**

### Build fails:
- Ensure CMake and MinGW are in PATH
- Check that bundletool.jar exists in project root

### Installer creation fails:
- Install NSIS from https://nsis.sourceforge.io/Download
- Add NSIS to system PATH
- Restart command prompt

### FLTK issues:
- Delete `C:\fltk` folder and run build again
- Ensure Git is installed and in PATH

---

**💡 Pro Tip**: Use `build-all.bat` for the best experience - it handles everything automatically!
