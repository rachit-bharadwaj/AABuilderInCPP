#ifndef BUILDAUTOMATOR_H
#define BUILDAUTOMATOR_H

#include <string>
#include <functional>
#include <memory>

class BuildAutomator
{
public:
    explicit BuildAutomator();
    ~BuildAutomator();

    struct BuildConfig {
        std::string projectPath;
        std::string outputPath;
        std::string outputFileName; // desired artifact base name (without extension optional)
        std::string keystorePath;
        std::string keystorePassword;
        std::string keyAlias;
        std::string keyPassword;
        std::string buildMode; // "debug" or "release"
        bool cleanBuild;
    };

    void buildAAB(const BuildConfig &config);
    void buildAPK(const BuildConfig &config);
    void stopBuild();
    bool isBuildInProgress() const;

    // Callback function types
    using ProgressCallback = std::function<void(const std::string&)>;
    using FinishedCallback = std::function<void(bool, const std::string&)>;
    
    void setProgressCallback(ProgressCallback callback);
    void setFinishedCallback(FinishedCallback callback);

private:
    void startBuild(const BuildConfig &config, bool isAAB);
    bool validateConfig(const BuildConfig &config);
    std::string findExpoCommand();
    std::string findJavaKeytool();
    bool createOutputDirectory(const std::string &path);
    void logMessage(const std::string &message);
    void executeNextStep();
    
    enum BuildStep {
        StepNone,
        StepClean,
        StepBuildAAB,
        StepConvertToAPK,
        StepSignAPK,
        StepMoveToOutput,
        StepCompleted
    };

    class Impl;
    std::unique_ptr<Impl> m_impl;
    
    ProgressCallback m_progressCallback;
    FinishedCallback m_finishedCallback;
};

#endif // BUILDAUTOMATOR_H
