#include "MainWindow.h"
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_draw.H>
#include <iostream>
#include <filesystem>
#include <ctime>
#include <cstdio>

MainWindow::MainWindow()
    : Fl_Window(800, 600, "React Native Build Automator")
    , m_buildAutomator(new BuildAutomator())
{
    setupUI();
    // Enable resizing and maximize button
    this->resizable(m_logGroup);
    this->size_range(800, 600, 0, 0);
    this->color(fl_rgb_color(245,246,250));
    // Ensure window closes when the system close button is clicked
    this->callback([](Fl_Widget* w, void*) { w->hide(); }, nullptr);
    
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
    
    // If project path is already set, populate default output name
    if (strlen(m_projectPathEdit->value()) > 0) {
        std::string defaultName = generateDefaultOutputName(m_projectPathEdit->value());
        logMessage(std::string("Startup: Setting default output name to: ") + defaultName);
        m_outputNameEdit->value(defaultName.c_str());
    }
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
    m_lblProjectPath = new Fl_Box(30, 50, 100, 25, "Project Path:");
    m_lblProjectPath->labelfont(FL_HELVETICA);
    m_lblProjectPath->labelsize(12);
    m_projectPathEdit = new Fl_Input(140, 50, 500, 25);
    m_projectPathEdit->callback(inputChanged_cb, this);
    m_projectPathEdit->when(FL_WHEN_CHANGED | FL_WHEN_RELEASE);
    m_projectPathButton = new Fl_Button(650, 50, 100, 25, "Browse");
    m_projectPathButton->callback(selectProjectPath_cb, this);
    
    // Output path
    m_lblOutputPath = new Fl_Box(30, 80, 100, 25, "Output Path:");
    m_lblOutputPath->labelfont(FL_HELVETICA);
    m_lblOutputPath->labelsize(12);
    m_outputPathEdit = new Fl_Input(140, 80, 500, 25);
    m_outputPathEdit->callback(inputChanged_cb, this);
    m_outputPathEdit->when(FL_WHEN_CHANGED);
    m_outputPathButton = new Fl_Button(650, 80, 100, 25, "Browse");
    m_outputPathButton->callback(selectOutputPath_cb, this);
    
    // Keystore path
    m_lblKeystore = new Fl_Box(30, 110, 100, 25, "Keystore:");
    m_lblKeystore->labelfont(FL_HELVETICA);
    m_lblKeystore->labelsize(12);
    m_keystorePathEdit = new Fl_Input(140, 110, 500, 25);
    m_keystorePathEdit->callback(inputChanged_cb, this);
    m_keystorePathEdit->when(FL_WHEN_CHANGED);
    m_keystoreButton = new Fl_Button(650, 110, 100, 25, "Browse");
    m_keystoreButton->callback(selectKeystore_cb, this);
    
    // Keystore details
    m_lblPassword = new Fl_Box(30, 140, 100, 25, "Password:");
    m_lblPassword->labelfont(FL_HELVETICA);
    m_lblPassword->labelsize(12);
    m_keystorePasswordEdit = new Fl_Input(140, 140, 150, 25);
    m_keystorePasswordEdit->type(FL_SECRET_INPUT);
    m_keystorePasswordEdit->callback(inputChanged_cb, this);
    m_keystorePasswordEdit->when(FL_WHEN_CHANGED);
    
    m_lblAlias = new Fl_Box(300, 140, 80, 25, "Alias:");
    m_lblAlias->labelfont(FL_HELVETICA);
    m_lblAlias->labelsize(12);
    m_keyAliasEdit = new Fl_Input(390, 140, 150, 25);
    m_keyAliasEdit->callback(inputChanged_cb, this);
    m_keyAliasEdit->when(FL_WHEN_CHANGED);
    
    m_lblKeyPassword = new Fl_Box(550, 140, 100, 25, "Key Password:");
    m_lblKeyPassword->labelfont(FL_HELVETICA);
    m_lblKeyPassword->labelsize(12);
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
    
    m_lblBuildMode = new Fl_Box(30, 250, 100, 25, "Build Mode:");
    m_lblBuildMode->labelfont(FL_HELVETICA);
    m_lblBuildMode->labelsize(12);
    m_buildModeCombo = new Fl_Choice(140, 250, 150, 25);
    m_buildModeCombo->add("release");
    m_buildModeCombo->add("debug");
    m_buildModeCombo->value(0);
    
    m_cleanBuildCheck = new Fl_Check_Button(300, 250, 120, 25, "Clean build");
    m_cleanBuildCheck->value(0);

    // Output name (optional override)
    m_lblOutputName = new Fl_Box(430, 250, 100, 25, "Output Name:");
    m_lblOutputName->labelfont(FL_HELVETICA);
    m_lblOutputName->labelsize(12);
    m_outputNameEdit = new Fl_Input(530, 250, 250, 25);
    m_outputNameEdit->when(FL_WHEN_CHANGED);
    
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

void MainWindow::resize(int x, int y, int w, int h)
{
    Fl_Window::resize(x, y, w, h);
    updateLayout(w, h);
}

void MainWindow::updateLayout(int w, int h)
{
    // Maintain margins and a two-column layout for form rows
    int margin = 20;
    int contentW = w - margin * 2;

    // Resize groups
    m_inputGroup->resize(margin, 20, contentW, 180);
    m_optionsGroup->resize(margin, 220, contentW, 80);
    m_actionGroup->resize(margin, 320, contentW, 80);
    m_logGroup->resize(margin, 420, contentW, h - 440);

    // Form layout
    int labelW = 110;
    int buttonW = 100;
    int gap = 10;
    int fieldX = margin + labelW + gap;
    int fieldW = contentW - labelW - buttonW - gap * 3;
    if (fieldW < 240) fieldW = 240;

    m_lblProjectPath->resize(margin + 10, 50, labelW, 25);
    m_projectPathEdit->resize(fieldX, 50, fieldW, 25);
    m_projectPathButton->resize(margin + contentW - buttonW - 10, 50, buttonW, 25);

    m_lblOutputPath->resize(margin + 10, 80, labelW, 25);
    m_outputPathEdit->resize(fieldX, 80, fieldW, 25);
    m_outputPathButton->resize(margin + contentW - buttonW - 10, 80, buttonW, 25);

    m_lblKeystore->resize(margin + 10, 110, labelW, 25);
    m_keystorePathEdit->resize(fieldX, 110, fieldW, 25);
    m_keystoreButton->resize(margin + contentW - buttonW - 10, 110, buttonW, 25);

    // Lower row (three columns): Password | Alias | Key Password
    int colW = (contentW - 40) / 3; // divide remaining space into 3
    if (colW < 200) colW = 200;
    int rowY = 140;

    // Password
    m_lblPassword->resize(margin + 10, rowY, labelW, 25);
    m_keystorePasswordEdit->resize(margin + 10 + labelW + gap, rowY, colW - labelW - gap, 25);

    // Alias
    int aliasX = margin + 10 + colW + gap;
    m_lblAlias->resize(aliasX, rowY, labelW, 25);
    m_keyAliasEdit->resize(aliasX + labelW + gap, rowY, colW - labelW - gap, 25);

    // Key Password
    int keyPwdX = margin + 10 + colW * 2 + gap * 2;
    m_lblKeyPassword->resize(keyPwdX, rowY, labelW + 20, 25);
    m_keyPasswordEdit->resize(keyPwdX + labelW + 20 + gap, rowY, colW - labelW - 20 - gap, 25);

    // Action buttons centered left
    m_buildAABButton->resize(margin + 10, 350, 120, 40);
    m_buildAPKButton->resize(margin + 150, 350, 120, 40);
    m_clearLogButton->resize(margin + 290, 350, 120, 40);

    // Log widgets
    m_progressBar->resize(margin + 10, 450, contentW - 20, 20);
    m_logTextEdit->resize(margin + 10, 480, contentW - 20, h - 510);
    m_logGroup->redraw();

    // Build Options row positions (prevent overlap)
    int optionsY = 250;
    int modeLabelX = margin + 30;
    int modeFieldX = modeLabelX + labelW + gap;
    m_lblBuildMode->resize(modeLabelX, optionsY, labelW, 25);
    m_buildModeCombo->resize(modeFieldX, optionsY, 160, 25);
    int cleanX = modeFieldX + 180;
    m_cleanBuildCheck->resize(cleanX, optionsY, 120, 25);
    int outNameLabelX = cleanX + 150;
    m_lblOutputName->resize(outNameLabelX, optionsY, 100, 25);
    int outNameFieldX = outNameLabelX + 100 + gap;
    int outNameW = std::max(200, margin + contentW - outNameFieldX - 10);
    m_outputNameEdit->resize(outNameFieldX, optionsY, outNameW, 25);
}

void MainWindow::selectProjectPath_cb(Fl_Widget*, void* v)
{
    MainWindow* w = static_cast<MainWindow*>(v);
    const char* dir = fl_dir_chooser("Select React Native Project Directory", w->m_projectPathEdit->value());
    if (dir) {
        w->m_projectPathEdit->value(dir);
        w->logMessage(std::string("Selected project path: ") + dir);
        
        // Generate and set default output name
        std::string defaultName = w->generateDefaultOutputName(dir);
        w->logMessage(std::string("Generated default name: ") + defaultName);
        w->m_outputNameEdit->value(defaultName.c_str());
        w->logMessage(std::string("Set output name to: ") + w->m_outputNameEdit->value());
        
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

void MainWindow::inputChanged_cb(Fl_Widget* widget, void* v)
{
    MainWindow* w = static_cast<MainWindow*>(v);
    
    // Check if this is the project path input field
    if (widget == w->m_projectPathEdit) {
        const char* projectPath = w->m_projectPathEdit->value();
        if (strlen(projectPath) > 0) {
            // Generate and set default output name when project path is manually entered
            std::string defaultName = w->generateDefaultOutputName(projectPath);
            w->logMessage(std::string("Auto-generated output name: ") + defaultName);
            w->m_outputNameEdit->value(defaultName.c_str());
        }
    }
    
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
    config.outputFileName = w->m_outputNameEdit->value();
    config.keystorePath = w->m_keystorePathEdit->value();
    config.keystorePassword = w->m_keystorePasswordEdit->value();
    config.keyAlias = w->m_keyAliasEdit->value();
    config.keyPassword = w->m_keyPasswordEdit->value();
    config.buildMode = w->m_buildModeCombo->text();
    config.cleanBuild = w->m_cleanBuildCheck->value();
    
    w->m_progressBar->show();
    // Start busy animation
    if (!w->m_busyAnimating) {
        w->m_busyAnimating = true;
        w->m_progressBar->minimum(0);
        w->m_progressBar->maximum(100);
        w->m_progressBar->value(0);
        Fl::add_timeout(0.05, busyTick_cb, w);
    }
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
    config.outputFileName = w->m_outputNameEdit->value();
    config.keystorePath = w->m_keystorePathEdit->value();
    config.keystorePassword = w->m_keystorePasswordEdit->value();
    config.keyAlias = w->m_keyAliasEdit->value();
    config.keyPassword = w->m_keyPasswordEdit->value();
    config.buildMode = w->m_buildModeCombo->text();
    config.cleanBuild = w->m_cleanBuildCheck->value();
    
    w->m_progressBar->show();
    if (!w->m_busyAnimating) {
        w->m_busyAnimating = true;
        w->m_progressBar->minimum(0);
        w->m_progressBar->maximum(100);
        w->m_progressBar->value(0);
        Fl::add_timeout(0.05, busyTick_cb, w);
    }
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
    // Stop busy animation if running
    if (m_busyAnimating) {
        m_busyAnimating = false;
    }
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

void MainWindow::busyTick_cb(void* userdata)
{
    MainWindow* w = static_cast<MainWindow*>(userdata);
    if (!w->m_busyAnimating) return;
    double v = w->m_progressBar->value();
    v += 2.5;
    if (v > 100) v = 0;
    w->m_progressBar->value(v);
    w->m_progressBar->redraw();
    Fl::repeat_timeout(0.05, busyTick_cb, w);
}

std::string MainWindow::generateDefaultOutputName(const std::string& projectPath)
{
    std::filesystem::path proj(projectPath);
    std::string projName = proj.filename().string();
    if (projName.empty()) projName = proj.parent_path().filename().string();
    if (projName.empty()) projName = "app";

    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char datebuf[16];
    std::snprintf(datebuf, sizeof(datebuf), "%02d%02d%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
    return projName + datebuf;
}
