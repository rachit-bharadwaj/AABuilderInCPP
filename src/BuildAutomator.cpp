#include "BuildAutomator.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <ctime>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <linux/limits.h>
#endif

class BuildAutomator::Impl {
public:
    bool buildInProgress = false;
    std::thread buildThread;
};

BuildAutomator::BuildAutomator()
    : m_impl(std::make_unique<Impl>())
{
}

BuildAutomator::~BuildAutomator()
{
    if (m_impl->buildInProgress) {
        stopBuild();
    }
}

void BuildAutomator::buildAAB(const BuildConfig &config)
{
    if (m_impl->buildInProgress) {
        if (m_finishedCallback) {
            m_finishedCallback(false, "Build already in progress");
        }
        return;
    }
    
    m_impl->buildInProgress = true;
    
    // Start build in background thread
    m_impl->buildThread = std::thread([this, config]() {
        if (m_progressCallback) {
            m_progressCallback("Starting AAB build...");
        }

        std::string mode = config.buildMode.empty() ? std::string("release") : config.buildMode;
        std::transform(mode.begin(), mode.end(), mode.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

        if (m_progressCallback) {
            m_progressCallback(std::string("Running: npx react-native build-android --mode=") + mode);
            m_progressCallback(std::string("Working directory: ") + config.projectPath);
        }

        // Validate project directory contains package.json
        std::filesystem::path packageJsonPath = std::filesystem::path(config.projectPath) / "package.json";
        if (!std::filesystem::exists(packageJsonPath)) {
            if (m_finishedCallback) {
                m_finishedCallback(false, "Selected directory does not appear to be a valid React Native project (package.json not found).");
            }
            m_impl->buildInProgress = false;
            return;
        }

        // If clean build requested, run gradle clean
        if (config.cleanBuild) {
            if (m_progressCallback) {
                m_progressCallback("Performing clean build (gradlew clean)...");
            }

            std::vector<std::string> cleanLastLines;
            int cleanRc = 0;
#ifdef _WIN32
            SECURITY_ATTRIBUTES saAttrC{sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
            HANDLE rd = NULL, wr = NULL;
            if (!CreatePipe(&rd, &wr, &saAttrC, 0) || !SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0)) {
                if (m_finishedCallback) m_finishedCallback(false, "Failed to start clean process.");
                m_impl->buildInProgress = false;
                return;
            }
            PROCESS_INFORMATION pi{}; STARTUPINFOA si{}; si.cb = sizeof(si);
            si.hStdError = wr; si.hStdOutput = wr; si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
            std::string cleanCmd = "cmd /c \"cd /d \"" + config.projectPath + "\\android\" && gradlew clean\"";
            BOOL ok = CreateProcessA(NULL, cleanCmd.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
            if (!ok) {
                CloseHandle(rd); CloseHandle(wr);
                if (m_finishedCallback) m_finishedCallback(false, "Failed to start gradle clean. Ensure Gradle wrapper exists.");
                m_impl->buildInProgress = false;
                return;
            }
            CloseHandle(wr);
            CHAR buf[4096]; DWORD n;
            while (ReadFile(rd, buf, sizeof(buf)-1, &n, NULL) && n) {
                buf[n] = '\0';
                std::string chunk(buf);
                if (m_progressCallback) m_progressCallback(chunk);
                cleanLastLines.push_back(chunk);
                if (cleanLastLines.size() > 100) cleanLastLines.erase(cleanLastLines.begin());
            }
            WaitForSingleObject(pi.hProcess, INFINITE);
            DWORD ec; GetExitCodeProcess(pi.hProcess, &ec); cleanRc = (int)ec;
            CloseHandle(pi.hProcess); CloseHandle(pi.hThread); CloseHandle(rd);
#else
            std::string cleanCmd = "cd \"" + config.projectPath + "/android\" && ./gradlew clean 2>&1";
            FILE* cp = popen(cleanCmd.c_str(), "r");
            if (!cp) { if (m_finishedCallback) m_finishedCallback(false, "Failed to start gradle clean."); m_impl->buildInProgress = false; return; }
            char cbuf[4096];
            while (fgets(cbuf, sizeof(cbuf), cp)) { std::string line(cbuf); if (m_progressCallback) m_progressCallback(line); }
            cleanRc = pclose(cp);
#endif
            if (cleanRc != 0) {
                if (m_finishedCallback) {
                    m_finishedCallback(false, "gradlew clean failed. See log for details.");
                }
                m_impl->buildInProgress = false;
                return;
            }
        }

        std::vector<std::string> lastLines;
        lastLines.reserve(100);
        int rc = 0;

#ifdef _WIN32
        // Windows: Use CreateProcess with hidden window and pipe output
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        HANDLE hChildStd_OUT_Rd = NULL;
        HANDLE hChildStd_OUT_Wr = NULL;

        // Create pipe for child process stdout
        if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) {
            if (m_finishedCallback) {
                m_finishedCallback(false, "Failed to create output pipe for build process.");
            }
            m_impl->buildInProgress = false;
            return;
        }

        // Ensure read handle is not inherited
        if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
            CloseHandle(hChildStd_OUT_Rd);
            CloseHandle(hChildStd_OUT_Wr);
            if (m_finishedCallback) {
                m_finishedCallback(false, "Failed to configure output pipe for build process.");
            }
            m_impl->buildInProgress = false;
            return;
        }

        PROCESS_INFORMATION piProcInfo;
        STARTUPINFOA siStartInfo;
        BOOL bSuccess = FALSE;

        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));

        siStartInfo.cb = sizeof(STARTUPINFOA);
        siStartInfo.hStdError = hChildStd_OUT_Wr;
        siStartInfo.hStdOutput = hChildStd_OUT_Wr;
        siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
        siStartInfo.dwFlags |= STARTF_USESHOWWINDOW;
        siStartInfo.wShowWindow = SW_HIDE; // Hide the window

        std::string cmdLine = "cmd /c \"cd /d \"" + config.projectPath + "\" && npx react-native build-android --mode=" + mode + "\"";
        
        bSuccess = CreateProcessA(NULL,
            const_cast<char*>(cmdLine.c_str()),
            NULL,
            NULL,
            TRUE,
            CREATE_NO_WINDOW,  // Don't create a window
            NULL,
            config.projectPath.c_str(),
            &siStartInfo,
            &piProcInfo);

        if (!bSuccess) {
            CloseHandle(hChildStd_OUT_Rd);
            CloseHandle(hChildStd_OUT_Wr);
            if (m_finishedCallback) {
                m_finishedCallback(false, "Failed to start build process. Ensure Node.js and npx are installed and available in PATH.");
            }
            m_impl->buildInProgress = false;
            return;
        }

        CloseHandle(hChildStd_OUT_Wr);

        // Read output from child process
        const DWORD BUFSIZE = 4096;
        CHAR chBuf[BUFSIZE];
        DWORD dwRead;
        std::string currentLine;

        while (true) {
            bSuccess = ReadFile(hChildStd_OUT_Rd, chBuf, BUFSIZE - 1, &dwRead, NULL);
            if (!bSuccess || dwRead == 0) break;

            chBuf[dwRead] = '\0';
            std::string chunk(chBuf);
            
            for (char c : chunk) {
                if (c == '\n' || c == '\r') {
                    if (!currentLine.empty()) {
                        if (m_progressCallback) {
                            m_progressCallback(currentLine);
                        }
                        lastLines.push_back(currentLine);
                        if (lastLines.size() > 100) {
                            lastLines.erase(lastLines.begin());
                        }
                        currentLine.clear();
                    }
                } else {
                    currentLine += c;
                }
            }
        }

        // Don't forget the last line if it doesn't end with newline
        if (!currentLine.empty()) {
            if (m_progressCallback) {
                m_progressCallback(currentLine);
            }
            lastLines.push_back(currentLine);
        }

        // Wait for process to complete
        WaitForSingleObject(piProcInfo.hProcess, INFINITE);
        
        DWORD exitCode;
        GetExitCodeProcess(piProcInfo.hProcess, &exitCode);
        rc = static_cast<int>(exitCode);

        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
        CloseHandle(hChildStd_OUT_Rd);

#else
        // Unix/Linux: Use popen but with better error handling
        std::string command = "cd \"" + config.projectPath + "\" && npx react-native build-android --mode=" + mode + " 2>&1";
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            if (m_finishedCallback) {
                m_finishedCallback(false, "Failed to start build process. Ensure Node.js and npx are installed and available in PATH.");
            }
            m_impl->buildInProgress = false;
            return;
        }

        char buffer[4096];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string line(buffer);
            // Remove trailing newline
            if (!line.empty() && line.back() == '\n') {
                line.pop_back();
            }
            if (m_progressCallback) {
                m_progressCallback(line);
            }
            lastLines.push_back(line);
            if (lastLines.size() > 100) {
                lastLines.erase(lastLines.begin());
            }
        }

        rc = pclose(pipe);
#endif

        if (rc != 0) {
            std::ostringstream oss;
            oss << "Build command failed (exit code " << rc << ").";
            
            // Look for common error patterns in the output
            bool foundCommonError = false;
            for (const auto &line : lastLines) {
                if (line.find("ENOENT") != std::string::npos || line.find("command not found") != std::string::npos) {
                    oss << "\n\n❌ Command not found - Please ensure Node.js and React Native CLI are properly installed.";
                    foundCommonError = true;
                    break;
                } else if (line.find("ANDROID_HOME") != std::string::npos) {
                    oss << "\n\n❌ Android SDK not configured - Please set ANDROID_HOME environment variable.";
                    foundCommonError = true;
                    break;
                } else if (line.find("Gradle") != std::string::npos && line.find("FAILED") != std::string::npos) {
                    oss << "\n\n❌ Gradle build failed - Check Android dependencies and configuration.";
                    foundCommonError = true;
                    break;
                } else if (line.find("No connected devices") != std::string::npos) {
                    oss << "\n\n❌ No Android devices/emulators detected.";
                    foundCommonError = true;
                    break;
                }
            }
            
            if (!foundCommonError && !lastLines.empty()) {
                oss << "\n\n— Recent build output —\n";
                // Show last 20 lines for context
                size_t startIdx = lastLines.size() > 20 ? lastLines.size() - 20 : 0;
                for (size_t i = startIdx; i < lastLines.size(); ++i) {
                    oss << lastLines[i] << "\n";
                }
            }
            
            if (m_finishedCallback) {
                m_finishedCallback(false, oss.str());
            }
            m_impl->buildInProgress = false;
            return;
        }

        // Expected AAB path
        std::filesystem::path aabPath = std::filesystem::path(config.projectPath) / "android" / "app" / "build" / "outputs" / "bundle" / mode / (std::string("app-") + mode + ".aab");

        if (!std::filesystem::exists(aabPath)) {
            if (m_finishedCallback) {
                m_finishedCallback(false, std::string("Build finished but AAB was not found at expected path: ") + aabPath.string());
            }
            m_impl->buildInProgress = false;
            return;
        }

        // Try to copy to output directory with naming rules
        std::filesystem::path finalPath = aabPath;
        if (!config.outputPath.empty()) {
            std::error_code ec;
            std::filesystem::path outDir(config.outputPath);
            std::filesystem::create_directories(outDir, ec);
            if (ec) {
                if (m_progressCallback) {
                    m_progressCallback(std::string("Warning: Could not create output directory: ") + ec.message());
                }
            } else {
                // Determine base name: user-provided or project name + date
                auto trim = [](std::string s){
                    const char* ws = " \t\n\r";
                    s.erase(0, s.find_first_not_of(ws));
                    s.erase(s.find_last_not_of(ws) + 1);
                    return s;
                };
                auto endsWithCaseInsensitive = [](const std::string& s, const std::string& suf){
                    if (s.size() < suf.size()) return false;
                    for (size_t i = 0; i < suf.size(); ++i) {
                        char a = (char)std::tolower((unsigned char)s[s.size() - suf.size() + i]);
                        char b = (char)std::tolower((unsigned char)suf[i]);
                        if (a != b) return false;
                    }
                    return true;
                };

                std::string baseName = trim(config.outputFileName);
                if (endsWithCaseInsensitive(baseName, ".aab")) {
                    baseName = baseName.substr(0, baseName.size() - 4);
                    baseName = trim(baseName);
                }
                if (baseName.empty()) {
                    // derive from project folder name; handle trailing slash
                    std::filesystem::path proj(config.projectPath);
                    std::string projName = proj.filename().string();
                    if (projName.empty()) {
                        projName = proj.parent_path().filename().string();
                    }
                    if (projName.empty()) {
                        projName = "app";
                    }
                    // date as ddmmyyyy
                    std::time_t t = std::time(nullptr);
                    std::tm tm{};
#ifdef _WIN32
                    localtime_s(&tm, &t);
#else
                    localtime_r(&t, &tm);
#endif
                    char datebuf[16];
                    std::snprintf(datebuf, sizeof(datebuf), "%02d%02d%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
                    baseName = projName + datebuf;
                }

                // ensure unique using non-throwing exists
                std::filesystem::path destPath = outDir / (baseName + ".aab");
                std::error_code ecExists;
                int suffix = 1;
                while (std::filesystem::exists(destPath, ecExists)) {
                    destPath = outDir / (baseName + "-" + std::to_string(suffix) + ".aab");
                    ++suffix;
                    ecExists.clear();
                }
                std::filesystem::copy_file(aabPath, destPath, std::filesystem::copy_options::none, ec);
                if (ec) {
                    if (m_progressCallback) {
                        m_progressCallback(std::string("Warning: Could not copy AAB to output path: ") + ec.message());
                    }
                } else {
                    finalPath = destPath;
                    if (m_progressCallback) {
                        m_progressCallback(std::string("Copied AAB to: ") + finalPath.string());
                    }
                }
            }
        }
        
        if (m_finishedCallback) {
            m_finishedCallback(true, std::string("AAB build completed successfully. File: ") + finalPath.string());
        }
        
        m_impl->buildInProgress = false;
    });
    
    m_impl->buildThread.detach();
}

void BuildAutomator::buildAPK(const BuildConfig &config)
{
    if (m_impl->buildInProgress) {
        if (m_finishedCallback) {
            m_finishedCallback(false, "Build already in progress");
        }
        return;
    }
    
    m_impl->buildInProgress = true;
    
    // Start build in background thread
    m_impl->buildThread = std::thread([this, config]() {
        try {
            if (m_progressCallback) {
                m_progressCallback("Starting APK build process...");
                m_progressCallback("Step 1: Building AAB first...");
            }

            std::string mode = config.buildMode.empty() ? std::string("release") : config.buildMode;
            std::transform(mode.begin(), mode.end(), mode.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

            if (m_progressCallback) {
                m_progressCallback(std::string("Running: npx react-native build-android --mode=") + mode);
                m_progressCallback(std::string("Working directory: ") + config.projectPath);
            }

            // Validate project directory contains package.json
            std::filesystem::path packageJsonPath = std::filesystem::path(config.projectPath) / "package.json";
            if (!std::filesystem::exists(packageJsonPath)) {
                if (m_finishedCallback) {
                    m_finishedCallback(false, "Selected directory does not appear to be a valid React Native project (package.json not found).");
                }
                m_impl->buildInProgress = false;
                return;
            }

            std::vector<std::string> lastLines;
            lastLines.reserve(100);
            int rc = 0;

#ifdef _WIN32
            // Windows: Use CreateProcess with hidden window and pipe output
            SECURITY_ATTRIBUTES saAttr;
            saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
            saAttr.bInheritHandle = TRUE;
            saAttr.lpSecurityDescriptor = NULL;

            HANDLE hChildStd_OUT_Rd = NULL;
            HANDLE hChildStd_OUT_Wr = NULL;

            // Create pipe for child process stdout
            if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) {
                if (m_finishedCallback) {
                    m_finishedCallback(false, "Failed to create output pipe for build process.");
                }
                m_impl->buildInProgress = false;
                return;
            }

            // Ensure read handle is not inherited
            if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
                CloseHandle(hChildStd_OUT_Rd);
                CloseHandle(hChildStd_OUT_Wr);
                if (m_finishedCallback) {
                    m_finishedCallback(false, "Failed to configure output pipe for build process.");
                }
                m_impl->buildInProgress = false;
                return;
            }

            PROCESS_INFORMATION piProcInfo;
            STARTUPINFOA siStartInfo;
            BOOL bSuccess = FALSE;

            ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
            ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));

            siStartInfo.cb = sizeof(STARTUPINFOA);
            siStartInfo.hStdError = hChildStd_OUT_Wr;
            siStartInfo.hStdOutput = hChildStd_OUT_Wr;
            siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
            siStartInfo.dwFlags |= STARTF_USESHOWWINDOW;
            siStartInfo.wShowWindow = SW_HIDE; // Hide the window

            std::string cmdLine = "cmd /c \"cd /d \"" + config.projectPath + "\" && npx react-native build-android --mode=" + mode + "\"";
            
            bSuccess = CreateProcessA(NULL,
                const_cast<char*>(cmdLine.c_str()),
                NULL,
                NULL,
                TRUE,
                CREATE_NO_WINDOW,  // Don't create a window
                NULL,
                config.projectPath.c_str(),
                &siStartInfo,
                &piProcInfo);

            if (!bSuccess) {
                CloseHandle(hChildStd_OUT_Rd);
                CloseHandle(hChildStd_OUT_Wr);
                if (m_finishedCallback) {
                    m_finishedCallback(false, "Failed to start build process. Ensure Node.js and npx are installed and available in PATH.");
                }
                m_impl->buildInProgress = false;
                return;
            }

            CloseHandle(hChildStd_OUT_Wr);

            // Read output from child process
            const DWORD BUFSIZE = 4096;
            CHAR chBuf[BUFSIZE];
            DWORD dwRead;
            std::string currentLine;

            while (true) {
                bSuccess = ReadFile(hChildStd_OUT_Rd, chBuf, BUFSIZE - 1, &dwRead, NULL);
                if (!bSuccess || dwRead == 0) break;

                chBuf[dwRead] = '\0';
                std::string chunk(chBuf);
                
                for (char c : chunk) {
                    if (c == '\n' || c == '\r') {
                        if (!currentLine.empty()) {
                            if (m_progressCallback) {
                                m_progressCallback(currentLine);
                            }
                            lastLines.push_back(currentLine);
                            if (lastLines.size() > 100) {
                                lastLines.erase(lastLines.begin());
                            }
                            currentLine.clear();
                        }
                    } else {
                        currentLine += c;
                    }
                }
            }

            // Don't forget the last line if it doesn't end with newline
            if (!currentLine.empty()) {
                if (m_progressCallback) {
                    m_progressCallback(currentLine);
                }
                lastLines.push_back(currentLine);
            }

            // Wait for process to complete
            WaitForSingleObject(piProcInfo.hProcess, INFINITE);
            
            DWORD exitCode;
            GetExitCodeProcess(piProcInfo.hProcess, &exitCode);
            rc = static_cast<int>(exitCode);

            CloseHandle(piProcInfo.hProcess);
            CloseHandle(piProcInfo.hThread);
            CloseHandle(hChildStd_OUT_Rd);

#else
            // Unix/Linux: Use popen but with better error handling
            std::string command = "cd \"" + config.projectPath + "\" && npx react-native build-android --mode=" + mode + " 2>&1";
            FILE* pipe = popen(command.c_str(), "r");
            if (!pipe) {
                if (m_finishedCallback) {
                    m_finishedCallback(false, "Failed to start build process. Ensure Node.js and npx are installed and available in PATH.");
                }
                m_impl->buildInProgress = false;
                return;
            }

            char buffer[4096];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                std::string line(buffer);
                // Remove trailing newline
                if (!line.empty() && line.back() == '\n') {
                    line.pop_back();
                }
                if (m_progressCallback) {
                    m_progressCallback(line);
                }
                lastLines.push_back(line);
                if (lastLines.size() > 100) {
                    lastLines.erase(lastLines.begin());
                }
            }

            rc = pclose(pipe);
#endif

            if (rc != 0) {
                std::ostringstream oss;
                oss << "AAB build failed (exit code " << rc << "). Cannot proceed with APK creation.";
                
                // Look for common error patterns in the output
                bool foundCommonError = false;
                for (const auto &line : lastLines) {
                    if (line.find("ENOENT") != std::string::npos || line.find("command not found") != std::string::npos) {
                        oss << "\n\n❌ Command not found - Please ensure Node.js and React Native CLI are properly installed.";
                        foundCommonError = true;
                        break;
                    } else if (line.find("ANDROID_HOME") != std::string::npos) {
                        oss << "\n\n❌ Android SDK not configured - Please set ANDROID_HOME environment variable.";
                        foundCommonError = true;
                        break;
                    } else if (line.find("Gradle") != std::string::npos && line.find("FAILED") != std::string::npos) {
                        oss << "\n\n❌ Gradle build failed - Check Android dependencies and configuration.";
                        foundCommonError = true;
                        break;
                    } else if (line.find("No connected devices") != std::string::npos) {
                        oss << "\n\n❌ No Android devices/emulators detected.";
                        foundCommonError = true;
                        break;
                    }
                }
                
                if (!foundCommonError && !lastLines.empty()) {
                    oss << "\n\n— Recent build output —\n";
                    // Show last 20 lines for context
                    size_t startIdx = lastLines.size() > 20 ? lastLines.size() - 20 : 0;
                    for (size_t i = startIdx; i < lastLines.size(); ++i) {
                        oss << lastLines[i] << "\n";
                    }
                }
                
                if (m_finishedCallback) {
                    m_finishedCallback(false, oss.str());
                }
                m_impl->buildInProgress = false;
                return;
            }

            // Expected AAB path
            std::filesystem::path aabPath = std::filesystem::path(config.projectPath) / "android" / "app" / "build" / "outputs" / "bundle" / mode / (std::string("app-") + mode + ".aab");

            if (!std::filesystem::exists(aabPath)) {
                if (m_finishedCallback) {
                    m_finishedCallback(false, std::string("AAB build finished but file was not found at expected path: ") + aabPath.string());
                }
                m_impl->buildInProgress = false;
                return;
            }

            if (m_progressCallback) {
                m_progressCallback("✅ AAB built successfully!");
                m_progressCallback("Step 2: Converting AAB to APK using bundletool...");
            }

            // Step 2: Convert AAB to APK using bundletool
            std::filesystem::path bundletoolPath;
            
            // First try to find bundletool.jar in the installed location (Program Files)
            std::filesystem::path installedPath = std::filesystem::path("C:\\Program Files\\React Native Build Automator\\bundletool.jar");
            if (std::filesystem::exists(installedPath)) {
                bundletoolPath = installedPath;
            } else {
                // Fallback: Get the directory where the executable is located
                std::filesystem::path exePath;
#ifdef _WIN32
                char exePathBuf[MAX_PATH];
                GetModuleFileNameA(NULL, exePathBuf, MAX_PATH);
                exePath = std::filesystem::path(exePathBuf);
#else
                // Unix/Linux: read from /proc/self/exe
                char exePathBuf[PATH_MAX];
                ssize_t len = readlink("/proc/self/exe", exePathBuf, sizeof(exePathBuf) - 1);
                if (len != -1) {
                    exePathBuf[len] = '\0';
                    exePath = std::filesystem::path(exePathBuf);
                } else {
                    // Fallback to current working directory
                    exePath = std::filesystem::current_path();
                }
#endif
                
                // Look for bundletool.jar in the executable directory
                bundletoolPath = exePath.parent_path() / "bundletool.jar";
            }
            
            if (!std::filesystem::exists(bundletoolPath)) {
                if (m_finishedCallback) {
                    m_finishedCallback(false, "bundletool.jar not found. Please ensure the application is properly installed or bundletool.jar is available in the same folder as the executable.");
                }
                m_impl->buildInProgress = false;
                return;
            }

            // Determine output APK name
            std::string apkBaseName = config.outputFileName;
            auto trim = [](std::string s){
                const char* ws = " \t\n\r";
                s.erase(0, s.find_first_not_of(ws));
                s.erase(s.find_last_not_of(ws) + 1);
                return s;
            };
            auto endsWithCaseInsensitive = [](const std::string& s, const std::string& suf){
                if (s.size() < suf.size()) return false;
                for (size_t i = 0; i < suf.size(); ++i) {
                    char a = (char)std::tolower((unsigned char)s[s.size() - suf.size() + i]);
                    char b = (char)std::tolower((unsigned char)suf[i]);
                    if (a != b) return false;
                }
                return true;
            };

            if (endsWithCaseInsensitive(apkBaseName, ".aab")) {
                apkBaseName = apkBaseName.substr(0, apkBaseName.size() - 4);
                apkBaseName = trim(apkBaseName);
            }
            if (endsWithCaseInsensitive(apkBaseName, ".apk")) {
                apkBaseName = apkBaseName.substr(0, apkBaseName.size() - 4);
                apkBaseName = trim(apkBaseName);
            }
            if (apkBaseName.empty()) {
                // derive from project folder name
                std::filesystem::path proj(config.projectPath);
                std::string projName = proj.filename().string();
                if (projName.empty()) {
                    projName = proj.parent_path().filename().string();
                }
                if (projName.empty()) {
                    projName = "app";
                }
                // date as ddmmyyyy
                std::time_t t = std::time(nullptr);
                std::tm tm{};
#ifdef _WIN32
                localtime_s(&tm, &t);
#else
                localtime_r(&t, &tm);
#endif
                char datebuf[16];
                std::snprintf(datebuf, sizeof(datebuf), "%02d%02d%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
                apkBaseName = projName + datebuf;
            }

            // Build bundletool command
            std::string bundletoolCmd;
#ifdef _WIN32
            bundletoolCmd = "java -jar \"" + bundletoolPath.string() + "\" build-apks --bundle=\"" + aabPath.string() + "\" --output=\"" + apkBaseName + ".apks\" --ks=\"" + config.keystorePath + "\" --ks-key-alias=" + config.keyAlias + " --ks-pass=pass:" + config.keystorePassword + " --key-pass=pass:" + config.keyPassword + " --mode=universal";
#else
            bundletoolCmd = "java -jar \"" + bundletoolPath.string() + "\" build-apks --bundle=\"" + aabPath.string() + "\" --output=\"" + apkBaseName + ".apks\" --ks=\"" + config.keystorePath + "\" --ks-key-alias=" + config.keyAlias + " --ks-pass=pass:" + config.keystorePassword + " --key-pass=pass:" + config.keyPassword + " --mode=universal";
#endif

            if (m_progressCallback) {
                m_progressCallback("Running bundletool command...");
                m_progressCallback(bundletoolCmd);
            }

            // Execute bundletool command
            std::vector<std::string> bundletoolLines;
            int bundletoolRc = 0;

#ifdef _WIN32
            // Windows: Execute bundletool
            SECURITY_ATTRIBUTES saAttr2{sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
            HANDLE rd2 = NULL, wr2 = NULL;
            if (!CreatePipe(&rd2, &wr2, &saAttr2, 0) || !SetHandleInformation(rd2, HANDLE_FLAG_INHERIT, 0)) {
                if (m_finishedCallback) m_finishedCallback(false, "Failed to start bundletool process.");
                m_impl->buildInProgress = false;
                return;
            }
            PROCESS_INFORMATION pi2{}; STARTUPINFOA si2{}; si2.cb = sizeof(si2);
            si2.hStdError = wr2; si2.hStdOutput = wr2; si2.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            si2.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW; si2.wShowWindow = SW_HIDE;
            std::string cmd2 = "cmd /c \"" + bundletoolCmd + "\"";
            BOOL ok2 = CreateProcessA(NULL, cmd2.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si2, &pi2);
            if (!ok2) {
                CloseHandle(rd2); CloseHandle(wr2);
                if (m_finishedCallback) m_finishedCallback(false, "Failed to start bundletool. Ensure Java is installed.");
                m_impl->buildInProgress = false;
                return;
            }
            CloseHandle(wr2);
            CHAR buf2[4096]; DWORD n2;
            while (ReadFile(rd2, buf2, sizeof(buf2)-1, &n2, NULL) && n2) {
                buf2[n2] = '\0';
                std::string chunk2(buf2);
                if (m_progressCallback) m_progressCallback(chunk2);
                bundletoolLines.push_back(chunk2);
                if (bundletoolLines.size() > 100) bundletoolLines.erase(bundletoolLines.begin());
            }
            WaitForSingleObject(pi2.hProcess, INFINITE);
            DWORD ec2; GetExitCodeProcess(pi2.hProcess, &ec2); bundletoolRc = (int)ec2;
            CloseHandle(pi2.hProcess); CloseHandle(pi2.hThread); CloseHandle(rd2);
#else
            // Unix/Linux: Execute bundletool
            FILE* pipe2 = popen(bundletoolCmd.c_str(), "r");
            if (!pipe2) { 
                if (m_finishedCallback) m_finishedCallback(false, "Failed to start bundletool."); 
                m_impl->buildInProgress = false; 
                return; 
            }
            char buf2[4096];
            while (fgets(buf2, sizeof(buf2), pipe2)) { 
                std::string line2(buf2); 
                if (m_progressCallback) m_progressCallback(line2); 
                bundletoolLines.push_back(line2);
                if (bundletoolLines.size() > 100) bundletoolLines.erase(bundletoolLines.begin());
            }
            bundletoolRc = pclose(pipe2);
#endif

            if (bundletoolRc != 0) {
                if (m_finishedCallback) {
                    m_finishedCallback(false, "Bundletool conversion failed. See log for details.");
                }
                m_impl->buildInProgress = false;
                return;
            }

            // Check if .apks file was created
            std::filesystem::path apksPath = std::filesystem::current_path() / (apkBaseName + ".apks");
            if (!std::filesystem::exists(apksPath)) {
                if (m_finishedCallback) {
                    m_finishedCallback(false, "Bundletool conversion completed but .apks file was not found.");
                }
                m_impl->buildInProgress = false;
                return;
            }

            if (m_progressCallback) {
                m_progressCallback("✅ APK conversion completed successfully!");
                m_progressCallback("Step 3: Extracting APK from .apks bundle...");
            }

            // Step 3: Extract APK from .apks bundle
            // First rename .apks to .zip
            std::filesystem::path zipPath = std::filesystem::current_path() / (apkBaseName + ".zip");
            std::error_code ec;
            std::filesystem::rename(apksPath, zipPath, ec);
            if (ec) {
                if (m_finishedCallback) {
                    m_finishedCallback(false, "Failed to rename .apks to .zip: " + ec.message());
                }
                m_impl->buildInProgress = false;
                return;
            }

            // Create extraction directory
            std::filesystem::path extractDir = std::filesystem::current_path() / (apkBaseName + "_extract");
            std::filesystem::create_directories(extractDir, ec);
            if (ec) {
                if (m_finishedCallback) {
                    m_finishedCallback(false, "Failed to create extraction directory: " + ec.message());
                }
                m_impl->buildInProgress = false;
                return;
            }

            // Extract zip file
            if (m_progressCallback) {
                m_progressCallback("Extracting .zip file...");
            }

#ifdef _WIN32
            // Windows: Use PowerShell to extract zip
            std::string extractCmd = "powershell -command \"Expand-Archive -Path '" + zipPath.string() + "' -DestinationPath '" + extractDir.string() + "' -Force\"";
            SECURITY_ATTRIBUTES saAttr3{sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
            HANDLE rd3 = NULL, wr3 = NULL;
            if (!CreatePipe(&rd3, &wr3, &saAttr3, 0) || !SetHandleInformation(rd3, HANDLE_FLAG_INHERIT, 0)) {
                if (m_finishedCallback) m_finishedCallback(false, "Failed to start extraction process.");
                m_impl->buildInProgress = false;
                return;
            }
            PROCESS_INFORMATION pi3{}; STARTUPINFOA si3{}; si3.cb = sizeof(si3);
            si3.hStdError = wr3; si3.hStdOutput = wr3; si3.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            si3.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW; si3.wShowWindow = SW_HIDE;
            BOOL ok3 = CreateProcessA(NULL, extractCmd.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si3, &pi3);
            if (!ok3) {
                CloseHandle(rd3); CloseHandle(wr3);
                if (m_finishedCallback) m_finishedCallback(false, "Failed to start extraction. Ensure PowerShell is available.");
                m_impl->buildInProgress = false;
                return;
            }
            CloseHandle(wr3);
            WaitForSingleObject(pi3.hProcess, INFINITE);
            DWORD ec3; GetExitCodeProcess(pi3.hProcess, &ec3);
            CloseHandle(pi3.hProcess); CloseHandle(pi3.hThread); CloseHandle(rd3);
            
            if (ec3 != 0) {
                if (m_finishedCallback) {
                    m_finishedCallback(false, "Failed to extract zip file.");
                }
                m_impl->buildInProgress = false;
                return;
            }
#else
            // Unix/Linux: Use unzip command
            std::string extractCmd = "unzip -o \"" + zipPath.string() + "\" -d \"" + extractDir.string() + "\"";
            FILE* pipe3 = popen(extractCmd.c_str(), "r");
            if (!pipe3) {
                if (m_finishedCallback) m_finishedCallback(false, "Failed to start extraction.");
                m_impl->buildInProgress = false;
                return;
            }
            int extractRc = pclose(pipe3);
            if (extractRc != 0) {
                if (m_finishedCallback) {
                    m_finishedCallback(false, "Failed to extract zip file.");
                }
                m_impl->buildInProgress = false;
                return;
            }
#endif

            // Look for universal.apk in the extracted directory
            std::filesystem::path universalApkPath = extractDir / "universal.apk";
            if (!std::filesystem::exists(universalApkPath)) {
                // Try to find any .apk file
                bool foundApk = false;
                for (const auto& entry : std::filesystem::recursive_directory_iterator(extractDir)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".apk") {
                        universalApkPath = entry.path();
                        foundApk = true;
                        break;
                    }
                }
                if (!foundApk) {
                    if (m_finishedCallback) {
                        m_finishedCallback(false, "No APK file found in extracted bundle.");
                    }
                    m_impl->buildInProgress = false;
                    return;
                }
            }

            if (m_progressCallback) {
                m_progressCallback("✅ APK extracted successfully!");
                m_progressCallback("Step 4: Finalizing APK...");
            }

            // Step 4: Copy APK to final location
            std::filesystem::path finalApkPath = std::filesystem::current_path() / (apkBaseName + ".apk");
            std::filesystem::copy_file(universalApkPath, finalApkPath, std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) {
                if (m_finishedCallback) {
                    m_finishedCallback(false, "Failed to copy extracted APK: " + ec.message());
                }
                m_impl->buildInProgress = false;
                return;
            }

            // Copy to output directory if specified
            std::filesystem::path finalPath = finalApkPath;
            if (!config.outputPath.empty()) {
                std::filesystem::path outDir(config.outputPath);
                std::filesystem::create_directories(outDir, ec);
                if (ec) {
                    if (m_progressCallback) {
                        m_progressCallback(std::string("Warning: Could not create output directory: ") + ec.message());
                    }
                } else {
                    std::filesystem::path destPath = outDir / finalApkPath.filename();
                    // Ensure unique filename
                    int suffix = 1;
                    while (std::filesystem::exists(destPath, ec)) {
                        destPath = outDir / (apkBaseName + "-" + std::to_string(suffix) + ".apk");
                        ++suffix;
                        ec.clear();
                    }
                    std::filesystem::copy_file(finalApkPath, destPath, std::filesystem::copy_options::overwrite_existing, ec);
                    if (ec) {
                        if (m_progressCallback) {
                            m_progressCallback(std::string("Warning: Could not copy APK to output path: ") + ec.message());
                        }
                    } else {
                        finalPath = destPath;
                        if (m_progressCallback) {
                            m_progressCallback(std::string("Copied APK to: ") + finalPath.string());
                        }
                    }
                }
            }

            // Clean up temporary files
            if (m_progressCallback) {
                m_progressCallback("Cleaning up temporary files...");
            }
            
            std::filesystem::remove(zipPath, ec);
            if (ec) {
                if (m_progressCallback) {
                    m_progressCallback(std::string("Warning: Could not remove temporary zip file: ") + ec.message());
                }
            }
            
            std::filesystem::remove_all(extractDir, ec);
            if (ec) {
        if (m_progressCallback) {
                    m_progressCallback(std::string("Warning: Could not remove extraction directory: ") + ec.message());
                }
        }
        
        if (m_finishedCallback) {
                m_finishedCallback(true, std::string("APK build completed successfully! File: ") + finalPath.string());
            }
            
        } catch (const std::exception& e) {
            if (m_finishedCallback) {
                m_finishedCallback(false, std::string("Build process crashed with error: ") + e.what());
            }
        } catch (...) {
            if (m_finishedCallback) {
                m_finishedCallback(false, "Build process crashed with unknown error.");
            }
        }
        
        m_impl->buildInProgress = false;
    });
    
    m_impl->buildThread.detach();
}

void BuildAutomator::stopBuild()
{
    if (m_impl->buildInProgress) {
        m_impl->buildInProgress = false;
        if (m_impl->buildThread.joinable()) {
            m_impl->buildThread.join();
        }
    }
}

bool BuildAutomator::isBuildInProgress() const
{
    return m_impl->buildInProgress;
}

void BuildAutomator::setProgressCallback(ProgressCallback callback)
{
    m_progressCallback = callback;
}

void BuildAutomator::setFinishedCallback(FinishedCallback callback)
{
    m_finishedCallback = callback;
}
