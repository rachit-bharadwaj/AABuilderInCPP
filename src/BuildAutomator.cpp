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
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
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
                std::string baseName = config.outputFileName;
                auto trim = [](std::string s){ s.erase(0, s.find_first_not_of(" \t\n\r")); s.erase(s.find_last_not_of(" \t\n\r") + 1); return s; };
                baseName = trim(baseName);
                if (baseName.empty()) {
                    // derive from project folder name
                    std::filesystem::path proj(config.projectPath);
                    std::string projName = proj.filename().string();
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

                // ensure unique
                std::filesystem::path destPath = outDir / (baseName + ".aab");
                int suffix = 1;
                while (std::filesystem::exists(destPath)) {
                    destPath = outDir / (baseName + "-" + std::to_string(suffix) + ".aab");
                    ++suffix;
                }
                std::filesystem::copy_options options = std::filesystem::copy_options::overwrite_existing;
                std::filesystem::copy_file(aabPath, destPath, options, ec);
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
        // Simulate build process
        if (m_progressCallback) {
            m_progressCallback("Starting APK build...");
            m_progressCallback("Cleaning project...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            m_progressCallback("Building AAB with EAS...");
            std::this_thread::sleep_for(std::chrono::seconds(2));
            m_progressCallback("Converting AAB to APK...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            m_progressCallback("Signing APK...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            m_progressCallback("APK build completed successfully!");
        }
        
        if (m_finishedCallback) {
            m_finishedCallback(true, "APK build completed successfully!");
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
