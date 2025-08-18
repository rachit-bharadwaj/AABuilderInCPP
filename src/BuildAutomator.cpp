#include "BuildAutomator.h"
#include <QDebug>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QDateTime>

BuildAutomator::BuildAutomator(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_currentStep(StepNone)
    , m_isAABBuild(false)
    , m_timeoutTimer(new QTimer(this))
{
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &BuildAutomator::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &BuildAutomator::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &BuildAutomator::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &BuildAutomator::onProcessOutput);
    
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(300000); // 5 minutes timeout
    connect(m_timeoutTimer, &QTimer::timeout, [this]() {
        logMessage("Build process timed out after 5 minutes");
        stopBuild();
        emit buildFinished(false, "Build process timed out");
    });
}

BuildAutomator::~BuildAutomator()
{
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
}

void BuildAutomator::buildAAB(const BuildConfig &config)
{
    if (!validateConfig(config)) {
        emit buildFinished(false, "Invalid configuration");
        return;
    }
    
    m_currentConfig = config;
    m_isAABBuild = true;
    m_currentStep = config.cleanBuild ? StepClean : StepBuildAAB;
    
    logMessage("=== Starting AAB Build Process ===");
    logMessage(QString("Project: %1").arg(config.projectPath));
    logMessage(QString("Output: %1").arg(config.outputPath));
    logMessage(QString("Mode: %1").arg(config.buildMode));
    
    executeNextStep();
}

void BuildAutomator::buildAPK(const BuildConfig &config)
{
    if (!validateConfig(config)) {
        emit buildFinished(false, "Invalid configuration");
        return;
    }
    
    m_currentConfig = config;
    m_isAABBuild = false;
    m_currentStep = config.cleanBuild ? StepClean : StepBuildAAB;
    
    logMessage("=== Starting APK Build Process ===");
    logMessage(QString("Project: %1").arg(config.projectPath));
    logMessage(QString("Output: %1").arg(config.outputPath));
    logMessage(QString("Mode: %1").arg(config.buildMode));
    
    executeNextStep();
}

void BuildAutomator::stopBuild()
{
    if (m_process->state() != QProcess::NotRunning) {
        logMessage("Stopping build process...");
        m_process->kill();
        m_process->waitForFinished(3000);
    }
    m_timeoutTimer->stop();
    m_currentStep = StepNone;
}

bool BuildAutomator::isBuildInProgress() const
{
    return m_currentStep != StepNone && m_currentStep != StepCompleted;
}

void BuildAutomator::executeNextStep()
{
    QString command;
    QStringList arguments;
    QString workingDir = m_currentConfig.projectPath;
    
    switch (m_currentStep) {
        case StepClean:
            logMessage("\n🧹 Cleaning project...");
            command = "expo";
            arguments << "prebuild" << "--clean";
            break;
            
        case StepBuildAAB:
            logMessage(QString("\n🔨 Building %1 %2...")
                      .arg(m_isAABBuild ? "AAB" : "AAB (for APK conversion)")
                      .arg(m_currentConfig.buildMode));
            command = "eas";
            arguments << "build" << "--platform" << "android" 
                     << "--profile" << m_currentConfig.buildMode
                     << "--local" << "--output" << "./build-output";
            break;
            
        case StepConvertToAPK:
            if (m_isAABBuild) {
                // Skip conversion for AAB builds
                m_currentStep = StepMoveToOutput;
                executeNextStep();
                return;
            }
            logMessage("\n🔄 Converting AAB to APK...");
            {
                QString bundletoolPath = findBundletool();
                if (bundletoolPath.isEmpty()) {
                    emit buildFinished(false, "Bundletool not found. Please install Android SDK build-tools.");
                    return;
                }
                command = "java";
                arguments << "-jar" << bundletoolPath << "build-apks"
                         << "--bundle=" + m_tempAABPath
                         << "--output=" + m_tempAPKPath
                         << "--mode=universal"
                         << "--ks=" + m_currentConfig.keystorePath
                         << "--ks-pass=pass:" + m_currentConfig.keystorePassword
                         << "--ks-key-alias=" + m_currentConfig.keyAlias
                         << "--key-pass=pass:" + m_currentConfig.keyPassword;
            }
            break;
            
        case StepSignAPK:
            if (m_isAABBuild) {
                // AAB doesn't need separate signing, skip to move
                m_currentStep = StepMoveToOutput;
                executeNextStep();
                return;
            }
            logMessage("\n✍️ Signing APK...");
            {
                QString apksignerPath = findApksigner();
                if (apksignerPath.isEmpty()) {
                    emit buildFinished(false, "apksigner not found. Please install Android SDK build-tools.");
                    return;
                }
                command = apksignerPath;
                arguments << "sign" << "--ks" << m_currentConfig.keystorePath
                         << "--ks-pass" << ("pass:" + m_currentConfig.keystorePassword)
                         << "--ks-key-alias" << m_currentConfig.keyAlias
                         << "--key-pass" << ("pass:" + m_currentConfig.keyPassword)
                         << "--out" << (m_currentConfig.outputPath + "/app-signed.apk")
                         << m_tempAPKPath;
            }
            break;
            
        case StepMoveToOutput:
            logMessage(QString("\n📁 Moving %1 to output directory...")
                      .arg(m_isAABBuild ? "AAB" : "APK"));
            {
                // Find the built file and move it
                QString sourceFile = findBuiltFile();
                if (sourceFile.isEmpty()) {
                    emit buildFinished(false, "Could not find built file");
                    return;
                }
                
                QString extension = m_isAABBuild ? ".aab" : ".apk";
                QString outputFile = m_currentConfig.outputPath + "/app-" + 
                                   m_currentConfig.buildMode + extension;
                
                if (QFile::exists(outputFile)) {
                    QFile::remove(outputFile);
                }
                
                if (QFile::copy(sourceFile, outputFile)) {
                    m_currentStep = StepCompleted;
                    emit buildFinished(true, QString("Build completed successfully!\nOutput: %1").arg(outputFile));
                    return;
                } else {
                    emit buildFinished(false, "Failed to copy file to output directory");
                    return;
                }
            }
            break;
            
        default:
            emit buildFinished(false, "Unknown build step");
            return;
    }
    
    if (!createOutputDirectory(m_currentConfig.outputPath)) {
        emit buildFinished(false, "Failed to create output directory");
        return;
    }
    
    logMessage(QString("Executing: %1 %2").arg(command, arguments.join(" ")));
    
    m_process->setWorkingDirectory(workingDir);
    m_process->start(command, arguments);
    m_timeoutTimer->start();
    
    if (!m_process->waitForStarted()) {
        emit buildFinished(false, QString("Failed to start command: %1").arg(command));
    }
}

void BuildAutomator::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_timeoutTimer->stop();
    
    if (exitStatus == QProcess::CrashExit) {
        logMessage("Process crashed!");
        emit buildFinished(false, "Build process crashed");
        return;
    }
    
    if (exitCode != 0) {
        QString error = m_process->readAllStandardError();
        logMessage(QString("Process failed with exit code %1").arg(exitCode));
        logMessage(QString("Error: %1").arg(error));
        emit buildFinished(false, QString("Build failed with exit code %1\n%2").arg(exitCode).arg(error));
        return;
    }
    
    // Move to next step
    switch (m_currentStep) {
        case StepClean:
            m_currentStep = StepBuildAAB;
            break;
        case StepBuildAAB:
            if (m_isAABBuild) {
                m_currentStep = StepMoveToOutput;
            } else {
                m_currentStep = StepConvertToAPK;
            }
            break;
        case StepConvertToAPK:
            m_currentStep = StepSignAPK;
            break;
        case StepSignAPK:
            m_currentStep = StepMoveToOutput;
            break;
        case StepMoveToOutput:
            m_currentStep = StepCompleted;
            break;
        default:
            break;
    }
    
    executeNextStep();
}

void BuildAutomator::onProcessError(QProcess::ProcessError error)
{
    m_timeoutTimer->stop();
    QString errorString;
    
    switch (error) {
        case QProcess::FailedToStart:
            errorString = "Failed to start the process. Check if the command exists.";
            break;
        case QProcess::Crashed:
            errorString = "Process crashed during execution.";
            break;
        case QProcess::Timedout:
            errorString = "Process timed out.";
            break;
        case QProcess::WriteError:
            errorString = "Write error occurred.";
            break;
        case QProcess::ReadError:
            errorString = "Read error occurred.";
            break;
        case QProcess::UnknownError:
            errorString = "Unknown error occurred.";
            break;
    }
    
    logMessage(QString("Process error: %1").arg(errorString));
    emit buildFinished(false, errorString);
}

void BuildAutomator::onProcessOutput()
{
    QByteArray data = m_process->readAllStandardOutput();
    QString output = QString::fromUtf8(data);
    
    if (!output.isEmpty()) {
        logMessage(output.trimmed());
    }
    
    // Also read error output
    QByteArray errorData = m_process->readAllStandardError();
    QString errorOutput = QString::fromUtf8(errorData);
    
    if (!errorOutput.isEmpty()) {
        logMessage(QString("STDERR: %1").arg(errorOutput.trimmed()));
    }
}

bool BuildAutomator::validateConfig(const BuildConfig &config)
{
    if (config.projectPath.isEmpty() || !QDir(config.projectPath).exists()) {
        logMessage("Invalid project path");
        return false;
    }
    
    if (config.outputPath.isEmpty()) {
        logMessage("Invalid output path");
        return false;
    }
    
    if (config.keystorePath.isEmpty() || !QFileInfo(config.keystorePath).exists()) {
        logMessage("Invalid keystore path");
        return false;
    }
    
    if (config.keystorePassword.isEmpty() || config.keyAlias.isEmpty() || config.keyPassword.isEmpty()) {
        logMessage("Missing keystore credentials");
        return false;
    }
    
    // Check if it's a valid React Native project
    QString packageJsonPath = config.projectPath + "/package.json";
    if (!QFileInfo(packageJsonPath).exists()) {
        logMessage("package.json not found - not a valid Node.js project");
        return false;
    }
    
    return true;
}

QString BuildAutomator::findBuiltFile()
{
    // Look for AAB/APK files in common build output locations
    QStringList searchPaths = {
        m_currentConfig.projectPath + "/build-output",
        m_currentConfig.projectPath + "/android/app/build/outputs/bundle/release",
        m_currentConfig.projectPath + "/android/app/build/outputs/apk/release"
    };
    
    QString extension = m_isAABBuild ? "*.aab" : "*.apk";
    
    for (const QString &path : searchPaths) {
        QDir dir(path);
        if (dir.exists()) {
            QStringList files = dir.entryList(QStringList() << extension, QDir::Files);
            if (!files.isEmpty()) {
                return dir.absoluteFilePath(files.first());
            }
        }
    }
    
    return QString();
}

QString BuildAutomator::findBundletool()
{
    // Try to find bundletool in common locations
    QStringList paths = {
        QStandardPaths::findExecutable("bundletool"),
        QDir::homePath() + "/Android/Sdk/build-tools/bundletool.jar"
    };
    
    for (const QString &path : paths) {
        if (QFileInfo(path).exists()) {
            return path;
        }
    }
    
    return QString();
}

QString BuildAutomator::findApksigner()
{
    // Try to find apksigner in Android SDK
    QString androidHome = qgetenv("ANDROID_HOME");
    if (androidHome.isEmpty()) {
        androidHome = QDir::homePath() + "/Android/Sdk";
    }
    
    QDir buildToolsDir(androidHome + "/build-tools");
    if (buildToolsDir.exists()) {
        QStringList versions = buildToolsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        if (!versions.isEmpty()) {
            // Use the latest version
            versions.sort();
            QString apksignerPath = buildToolsDir.absoluteFilePath(versions.last() + "/apksigner");
            if (QFileInfo(apksignerPath).exists()) {
                return apksignerPath;
            }
        }
    }
    
    return QStandardPaths::findExecutable("apksigner");
}

bool BuildAutomator::createOutputDirectory(const QString &path)
{
    QDir dir;
    return dir.mkpath(path);
}

void BuildAutomator::logMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    emit buildProgress(QString("[%1] %2").arg(timestamp, message));
}
