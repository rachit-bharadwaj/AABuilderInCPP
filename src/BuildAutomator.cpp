#include "BuildAutomator.h"
#include <iostream>
#include <thread>
#include <chrono>

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
        // Simulate build process
        if (m_progressCallback) {
            m_progressCallback("Starting AAB build...");
            m_progressCallback("Cleaning project...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            m_progressCallback("Building AAB with EAS...");
            std::this_thread::sleep_for(std::chrono::seconds(2));
            m_progressCallback("AAB build completed successfully!");
        }
        
        if (m_finishedCallback) {
            m_finishedCallback(true, "AAB build completed successfully!");
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
