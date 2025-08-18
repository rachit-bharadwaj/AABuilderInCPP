#include "MainWindow.h"
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_draw.H>
#include <iostream>

MainWindow::MainWindow()
    : Fl_Window(800, 600, "React Native Build Automator")
    , m_buildAutomator(new BuildAutomator())
{
    setupUI();
    // Enable resizing and maximize button
    this->resizable(m_logGroup);
    this->size_range(800, 600, 0, 0);
    this->color(fl_rgb_color(245,246,250));
    
    // Set up callbacks for build automator
    m_buildAutomator->setProgressCallback([this](const std::string& message) {
        this->onBuildProgress(message);
    });
    
    m_buildAutomator->setFinishedCallback([this](bool success, const std::string& message) {
        this->onBuildFinished(success, message);
    });
    
    updateBuildButtonStates();
    
    // Debug: Log that initialization is complete
    logMessage("MainWindow initialized successfully");
    logMessage("BuildAutomator callbacks set up");
}

MainWindow::~MainWindow()
{
    delete m_buildAutomator;
}

void MainWindow::setupUI()
{
    // Input group
    m_inputGroup = new Fl_Group(20, 20, 760, 180, "Project Configuration");
    m_inputGroup->box(FL_PLASTIC_UP_BOX);
    m_inputGroup->labelfont(FL_HELVETICA_BOLD);
    m_inputGroup->labelsize(14);
    
    // Project path
    new Fl_Box(30, 50, 100, 25, "Project Path:");
    m_projectPathEdit = new Fl_Input(140, 50, 500, 25);
    m_projectPathEdit->callback(inputChanged_cb, this);
    m_projectPathEdit->when(FL_WHEN_CHANGED);
    m_projectPathButton = new Fl_Button(650, 50, 100, 25, "Browse");
    m_projectPathButton->callback(selectProjectPath_cb, this);
    
    // Output path
    new Fl_Box(30, 80, 100, 25, "Output Path:");
    m_outputPathEdit = new Fl_Input(140, 80, 500, 25);
    m_outputPathEdit->callback(inputChanged_cb, this);
    m_outputPathEdit->when(FL_WHEN_CHANGED);
    m_outputPathButton = new Fl_Button(650, 80, 100, 25, "Browse");
    m_outputPathButton->callback(selectOutputPath_cb, this);
    
    // Keystore path
    new Fl_Box(30, 110, 100, 25, "Keystore:");
    m_keystorePathEdit = new Fl_Input(140, 110, 500, 25);
    m_keystorePathEdit->callback(inputChanged_cb, this);
    m_keystorePathEdit->when(FL_WHEN_CHANGED);
    m_keystoreButton = new Fl_Button(650, 110, 100, 25, "Browse");
    m_keystoreButton->callback(selectKeystore_cb, this);
    
    // Keystore details
    new Fl_Box(30, 140, 100, 25, "Password:");
    m_keystorePasswordEdit = new Fl_Input(140, 140, 150, 25);
    m_keystorePasswordEdit->type(FL_SECRET_INPUT);
    m_keystorePasswordEdit->callback(inputChanged_cb, this);
    m_keystorePasswordEdit->when(FL_WHEN_CHANGED);
    
    new Fl_Box(300, 140, 80, 25, "Alias:");
    m_keyAliasEdit = new Fl_Input(390, 140, 150, 25);
    m_keyAliasEdit->callback(inputChanged_cb, this);
    m_keyAliasEdit->when(FL_WHEN_CHANGED);
    
    new Fl_Box(550, 140, 100, 25, "Key Password:");
    m_keyPasswordEdit = new Fl_Input(650, 140, 150, 25);
    m_keyPasswordEdit->type(FL_SECRET_INPUT);
    m_keyPasswordEdit->callback(inputChanged_cb, this);
    m_keyPasswordEdit->when(FL_WHEN_CHANGED);
    
    m_inputGroup->end();
    
    // Options group
    m_optionsGroup = new Fl_Group(20, 220, 760, 80, "Build Options");
    m_optionsGroup->box(FL_PLASTIC_UP_BOX);
    m_optionsGroup->labelfont(FL_HELVETICA_BOLD);
    m_optionsGroup->labelsize(14);
    
    new Fl_Box(30, 250, 100, 25, "Build Mode:");
    m_buildModeCombo = new Fl_Choice(140, 250, 150, 25);
    m_buildModeCombo->add("release");
    m_buildModeCombo->add("debug");
    m_buildModeCombo->value(0);
    
    m_cleanBuildCheck = new Fl_Check_Button(300, 250, 100, 25, "Clean build");
    m_cleanBuildCheck->value(1);
    
    m_optionsGroup->end();
    
    // Action group
    m_actionGroup = new Fl_Group(20, 320, 760, 80, "Build Actions");
    m_actionGroup->box(FL_PLASTIC_UP_BOX);
    m_actionGroup->labelfont(FL_HELVETICA_BOLD);
    m_actionGroup->labelsize(14);
    
    m_buildAABButton = new Fl_Button(30, 350, 120, 40, "Build AAB");
    m_buildAABButton->callback(buildAAB_cb, this);
    m_buildAABButton->color(fl_rgb_color(46, 204, 113));
    m_buildAABButton->selection_color(fl_rgb_color(39, 174, 96));
    m_buildAABButton->labelfont(FL_HELVETICA_BOLD);
    m_buildAABButton->labelsize(14);
    m_buildAABButton->labelcolor(FL_WHITE);
    
    m_buildAPKButton = new Fl_Button(170, 350, 120, 40, "Build APK");
    m_buildAPKButton->callback(buildAPK_cb, this);
    m_buildAPKButton->color(fl_rgb_color(52, 152, 219));
    m_buildAPKButton->selection_color(fl_rgb_color(41, 128, 185));
    m_buildAPKButton->labelfont(FL_HELVETICA_BOLD);
    m_buildAPKButton->labelsize(14);
    m_buildAPKButton->labelcolor(FL_WHITE);
    
    m_clearLogButton = new Fl_Button(310, 350, 120, 40, "Clear Log");
    m_clearLogButton->callback(clearLog_cb, this);
    m_clearLogButton->color(fl_rgb_color(236, 240, 241));
    m_clearLogButton->selection_color(fl_rgb_color(189, 195, 199));
    m_clearLogButton->labelfont(FL_HELVETICA_BOLD);
    m_clearLogButton->labelsize(14);
    
    m_actionGroup->end();
    
    // Log group
    m_logGroup = new Fl_Group(20, 420, 760, 160, "Build Log");
    m_logGroup->box(FL_PLASTIC_UP_BOX);
    m_logGroup->labelfont(FL_HELVETICA_BOLD);
    m_logGroup->labelsize(14);
    
    m_progressBar = new Fl_Progress(30, 450, 740, 20);
    m_progressBar->color(fl_rgb_color(230, 230, 230));
    m_progressBar->selection_color(fl_rgb_color(52, 152, 219));
    m_progressBar->hide();
    
    m_logBuffer = new Fl_Text_Buffer();
    m_logTextEdit = new Fl_Text_Display(30, 480, 740, 90);
    m_logTextEdit->buffer(m_logBuffer);
    m_logTextEdit->textfont(FL_COURIER);
    m_logTextEdit->textsize(11);
    m_logTextEdit->color(FL_WHITE);
    m_logTextEdit->textcolor(FL_BLACK);
    m_logTextEdit->scrollbar_align(FL_ALIGN_RIGHT);

    // Make the log area resizable with the window
    m_logGroup->resizable(m_logTextEdit);
    
    m_logGroup->end();
}

void MainWindow::selectProjectPath_cb(Fl_Widget*, void* v)
{
    MainWindow* w = static_cast<MainWindow*>(v);
    const char* dir = fl_dir_chooser("Select React Native Project Directory", w->m_projectPathEdit->value());
    if (dir) {
        w->m_projectPathEdit->value(dir);
        w->logMessage(std::string("Selected project path: ") + dir);
        w->updateBuildButtonStates();
    }
}

void MainWindow::selectOutputPath_cb(Fl_Widget*, void* v)
{
    MainWindow* w = static_cast<MainWindow*>(v);
    const char* dir = fl_dir_chooser("Select Output Directory", w->m_outputPathEdit->value());
    if (dir) {
        w->m_outputPathEdit->value(dir);
        w->logMessage(std::string("Selected output path: ") + dir);
        w->updateBuildButtonStates();
    }
}

void MainWindow::selectKeystore_cb(Fl_Widget*, void* v)
{
    MainWindow* w = static_cast<MainWindow*>(v);
    const char* file = fl_file_chooser("Select Keystore File", "Keystore files (*.keystore *.jks)\tAll files (*.*)", w->m_keystorePathEdit->value());
    if (file) {
        w->m_keystorePathEdit->value(file);
        w->logMessage(std::string("Selected keystore: ") + file);
        w->updateBuildButtonStates();
    }
}

void MainWindow::inputChanged_cb(Fl_Widget*, void* v)
{
    MainWindow* w = static_cast<MainWindow*>(v);
    w->updateBuildButtonStates();
}

void MainWindow::buildAAB_cb(Fl_Widget*, void* v)
{
    MainWindow* w = static_cast<MainWindow*>(v);
    w->logMessage("Build AAB button clicked!");
    
    if (!w->validateInputs()) {
        w->logMessage("Input validation failed!");
        return;
    }
    
    w->logMessage("Starting AAB build...");
    
    BuildAutomator::BuildConfig config;
    config.projectPath = w->m_projectPathEdit->value();
    config.outputPath = w->m_outputPathEdit->value();
    config.keystorePath = w->m_keystorePathEdit->value();
    config.keystorePassword = w->m_keystorePasswordEdit->value();
    config.keyAlias = w->m_keyAliasEdit->value();
    config.keyPassword = w->m_keyPasswordEdit->value();
    config.buildMode = w->m_buildModeCombo->text();
    config.cleanBuild = w->m_cleanBuildCheck->value();
    
    w->m_progressBar->show();
    w->updateBuildButtonStates();
    
    w->m_buildAutomator->buildAAB(config);
}

void MainWindow::buildAPK_cb(Fl_Widget*, void* v)
{
    MainWindow* w = static_cast<MainWindow*>(v);
    w->logMessage("Build APK button clicked!");
    
    if (!w->validateInputs()) {
        w->logMessage("Input validation failed!");
        return;
    }
    
    w->logMessage("Starting APK build...");
    
    BuildAutomator::BuildConfig config;
    config.projectPath = w->m_projectPathEdit->value();
    config.outputPath = w->m_outputPathEdit->value();
    config.keystorePath = w->m_keystorePathEdit->value();
    config.keystorePassword = w->m_keystorePasswordEdit->value();
    config.keyAlias = w->m_keyAliasEdit->value();
    config.keyPassword = w->m_keyPasswordEdit->value();
    config.buildMode = w->m_buildModeCombo->text();
    config.cleanBuild = w->m_cleanBuildCheck->value();
    
    w->m_progressBar->show();
    w->updateBuildButtonStates();
    
    w->m_buildAutomator->buildAPK(config);
}

void MainWindow::clearLog_cb(Fl_Widget*, void* v)
{
    MainWindow* w = static_cast<MainWindow*>(v);
    w->m_logBuffer->text("");
    w->logMessage("Log cleared.");
}

void MainWindow::updateBuildButtonStates()
{
    bool hasRequiredInputs = strlen(m_projectPathEdit->value()) > 0 &&
                           strlen(m_outputPathEdit->value()) > 0 &&
                           strlen(m_keystorePathEdit->value()) > 0;
    
    bool buildInProgress = m_buildAutomator->isBuildInProgress();
    
    if (hasRequiredInputs && !buildInProgress) {
        m_buildAABButton->activate();
        m_buildAPKButton->activate();
    } else {
        m_buildAABButton->deactivate();
        m_buildAPKButton->deactivate();
    }
}

bool MainWindow::validateInputs()
{
    if (strlen(m_projectPathEdit->value()) == 0) {
        fl_alert("Please select a project path.");
        return false;
    }
    
    if (strlen(m_outputPathEdit->value()) == 0) {
        fl_alert("Please select an output path.");
        return false;
    }
    
    if (strlen(m_keystorePathEdit->value()) == 0) {
        fl_alert("Please select a keystore file.");
        return false;
    }
    
    if (strlen(m_keystorePasswordEdit->value()) == 0) {
        fl_alert("Please enter keystore password.");
        return false;
    }
    
    if (strlen(m_keyAliasEdit->value()) == 0) {
        fl_alert("Please enter key alias.");
        return false;
    }
    
    if (strlen(m_keyPasswordEdit->value()) == 0) {
        fl_alert("Please enter key password.");
        return false;
    }
    
    return true;
}

void MainWindow::logMessage(const std::string& message)
{
    m_logBuffer->append(message.c_str());
    m_logBuffer->append("\n");
    m_logTextEdit->scroll(m_logBuffer->count_lines(0, m_logBuffer->length()), 0);
}

void MainWindow::onBuildFinished(bool success, const std::string& message)
{
    m_progressBar->hide();
    updateBuildButtonStates();
    
    if (success) {
        logMessage("\n✅ BUILD SUCCESSFUL: " + message + "\n");
        // Success: keep feedback in the log only, no popup
    } else {
        logMessage("\n❌ BUILD FAILED: " + message + "\n");
        fl_alert("Build Error: %s", message.c_str());
    }
}

void MainWindow::onBuildProgress(const std::string& message)
{
    logMessage(message);
}
