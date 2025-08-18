#include "MainWindow.h"
#include <QMenuBar>
#include <QAction>
#include <QSizePolicy>
#include <QFont>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_buildAutomator(new BuildAutomator(this))
{
    setWindowTitle("React Native Build Automator");
    setWindowIcon(QIcon(":/icons/app.png")); // We'll add this later
    resize(800, 600);
    
    setupUI();
    setupMenuBar();
    setupStatusBar();
    
    // Connect build automator signals
    connect(m_buildAutomator, &BuildAutomator::buildProgress,
            this, &MainWindow::onBuildProgress);
    connect(m_buildAutomator, &BuildAutomator::buildFinished,
            this, &MainWindow::onBuildFinished);
    
    updateBuildButtonStates();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    m_mainSplitter = new QSplitter(Qt::Vertical, this);
    
    // Create input group
    m_inputGroup = new QGroupBox("Project Configuration", this);
    m_inputGroup->setFixedHeight(180);
    
    QGridLayout *inputLayout = new QGridLayout(m_inputGroup);
    
    // Project path
    inputLayout->addWidget(new QLabel("Project Path:"), 0, 0);
    m_projectPathEdit = new QLineEdit(this);
    m_projectPathEdit->setPlaceholderText("Select your React Native project folder...");
    m_projectPathButton = new QPushButton("Browse", this);
    inputLayout->addWidget(m_projectPathEdit, 0, 1);
    inputLayout->addWidget(m_projectPathButton, 0, 2);
    
    // Output path
    inputLayout->addWidget(new QLabel("Output Path:"), 1, 0);
    m_outputPathEdit = new QLineEdit(this);
    m_outputPathEdit->setPlaceholderText("Select output directory for APK/AAB files...");
    m_outputPathButton = new QPushButton("Browse", this);
    inputLayout->addWidget(m_outputPathEdit, 1, 1);
    inputLayout->addWidget(m_outputPathButton, 1, 2);
    
    // Keystore path
    inputLayout->addWidget(new QLabel("Keystore:"), 2, 0);
    m_keystorePathEdit = new QLineEdit(this);
    m_keystorePathEdit->setPlaceholderText("Select your release keystore file...");
    m_keystoreButton = new QPushButton("Browse", this);
    inputLayout->addWidget(m_keystorePathEdit, 2, 1);
    inputLayout->addWidget(m_keystoreButton, 2, 2);
    
    // Keystore details
    QHBoxLayout *keystoreLayout = new QHBoxLayout();
    keystoreLayout->addWidget(new QLabel("Password:"));
    m_keystorePasswordEdit = new QLineEdit(this);
    m_keystorePasswordEdit->setEchoMode(QLineEdit::Password);
    m_keystorePasswordEdit->setPlaceholderText("Keystore password");
    keystoreLayout->addWidget(m_keystorePasswordEdit);
    
    keystoreLayout->addWidget(new QLabel("Alias:"));
    m_keyAliasEdit = new QLineEdit(this);
    m_keyAliasEdit->setPlaceholderText("Key alias");
    keystoreLayout->addWidget(m_keyAliasEdit);
    
    keystoreLayout->addWidget(new QLabel("Key Password:"));
    m_keyPasswordEdit = new QLineEdit(this);
    m_keyPasswordEdit->setEchoMode(QLineEdit::Password);
    m_keyPasswordEdit->setPlaceholderText("Key password");
    keystoreLayout->addWidget(m_keyPasswordEdit);
    
    inputLayout->addLayout(keystoreLayout, 3, 0, 1, 3);
    
    // Options group
    m_optionsGroup = new QGroupBox("Build Options", this);
    m_optionsGroup->setFixedHeight(80);
    
    QHBoxLayout *optionsLayout = new QHBoxLayout(m_optionsGroup);
    
    optionsLayout->addWidget(new QLabel("Build Mode:"));
    m_buildModeCombo = new QComboBox(this);
    m_buildModeCombo->addItems({"release", "debug"});
    optionsLayout->addWidget(m_buildModeCombo);
    
    m_cleanBuildCheck = new QCheckBox("Clean build", this);
    m_cleanBuildCheck->setChecked(true);
    optionsLayout->addWidget(m_cleanBuildCheck);
    
    optionsLayout->addStretch();
    
    // Action group
    m_actionGroup = new QGroupBox("Build Actions", this);
    m_actionGroup->setFixedHeight(80);
    
    QHBoxLayout *actionLayout = new QHBoxLayout(m_actionGroup);
    
    m_buildAABButton = new QPushButton("Build AAB", this);
    m_buildAABButton->setMinimumHeight(40);
    m_buildAABButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; border-radius: 5px; }");
    
    m_buildAPKButton = new QPushButton("Build APK", this);
    m_buildAPKButton->setMinimumHeight(40);
    m_buildAPKButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; border-radius: 5px; }");
    
    m_clearLogButton = new QPushButton("Clear Log", this);
    m_clearLogButton->setMinimumHeight(40);
    
    actionLayout->addWidget(m_buildAABButton);
    actionLayout->addWidget(m_buildAPKButton);
    actionLayout->addWidget(m_clearLogButton);
    actionLayout->addStretch();
    
    // Log group
    m_logGroup = new QGroupBox("Build Log", this);
    
    QVBoxLayout *logLayout = new QVBoxLayout(m_logGroup);
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    logLayout->addWidget(m_progressBar);
    
    m_logTextEdit = new QTextEdit(this);
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setFont(QFont("Consolas", 9));
    logLayout->addWidget(m_logTextEdit);
    
    // Add groups to splitter
    QWidget *topWidget = new QWidget();
    QVBoxLayout *topLayout = new QVBoxLayout(topWidget);
    topLayout->addWidget(m_inputGroup);
    topLayout->addWidget(m_optionsGroup);
    topLayout->addWidget(m_actionGroup);
    topLayout->setContentsMargins(0, 0, 0, 0);
    
    m_mainSplitter->addWidget(topWidget);
    m_mainSplitter->addWidget(m_logGroup);
    m_mainSplitter->setSizes({350, 250});
    
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->addWidget(m_mainSplitter);
    
    // Connect signals
    connect(m_projectPathButton, &QPushButton::clicked, this, &MainWindow::selectProjectPath);
    connect(m_outputPathButton, &QPushButton::clicked, this, &MainWindow::selectOutputPath);
    connect(m_keystoreButton, &QPushButton::clicked, this, &MainWindow::selectKeystore);
    connect(m_buildAABButton, &QPushButton::clicked, this, &MainWindow::buildAAB);
    connect(m_buildAPKButton, &QPushButton::clicked, this, &MainWindow::buildAPK);
    connect(m_clearLogButton, &QPushButton::clicked, this, &MainWindow::clearLog);
    
    // Connect input validation
    connect(m_projectPathEdit, &QLineEdit::textChanged, this, &MainWindow::updateBuildButtonStates);
    connect(m_outputPathEdit, &QLineEdit::textChanged, this, &MainWindow::updateBuildButtonStates);
    connect(m_keystorePathEdit, &QLineEdit::textChanged, this, &MainWindow::updateBuildButtonStates);
}

void MainWindow::setupMenuBar()
{
    QMenuBar *menuBar = this->menuBar();
    
    // File menu
    QMenu *fileMenu = menuBar->addMenu("&File");
    
    QAction *exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);
    
    // Help menu
    QMenu *helpMenu = menuBar->addMenu("&Help");
    
    QAction *aboutAction = new QAction("&About", this);
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "About", 
            "React Native Build Automator v1.0\n\n"
            "A professional tool to automate React Native Expo build process.\n\n"
            "Built with Qt and C++");
    });
    helpMenu->addAction(aboutAction);
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("Ready", this);
    statusBar()->addWidget(m_statusLabel);
    statusBar()->showMessage("Application started successfully");
}

void MainWindow::selectProjectPath()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, 
        "Select React Native Project Directory",
        QDir::homePath()
    );
    
    if (!dir.isEmpty()) {
        m_projectPathEdit->setText(dir);
        m_logTextEdit->append(QString("Selected project path: %1").arg(dir));
    }
}

void MainWindow::selectOutputPath()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, 
        "Select Output Directory",
        QDir::homePath()
    );
    
    if (!dir.isEmpty()) {
        m_outputPathEdit->setText(dir);
        m_logTextEdit->append(QString("Selected output path: %1").arg(dir));
    }
}

void MainWindow::selectKeystore()
{
    QString file = QFileDialog::getOpenFileName(
        this,
        "Select Keystore File",
        QDir::homePath(),
        "Keystore files (*.keystore *.jks);;All files (*.*)"
    );
    
    if (!file.isEmpty()) {
        m_keystorePathEdit->setText(file);
        m_logTextEdit->append(QString("Selected keystore: %1").arg(file));
    }
}

void MainWindow::buildAAB()
{
    if (!validateInputs()) return;
    
    BuildAutomator::BuildConfig config;
    config.projectPath = m_projectPathEdit->text();
    config.outputPath = m_outputPathEdit->text();
    config.keystorePath = m_keystorePathEdit->text();
    config.keystorePassword = m_keystorePasswordEdit->text();
    config.keyAlias = m_keyAliasEdit->text();
    config.keyPassword = m_keyPasswordEdit->text();
    config.buildMode = m_buildModeCombo->currentText();
    config.cleanBuild = m_cleanBuildCheck->isChecked();
    
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0); // Indeterminate progress
    updateBuildButtonStates();
    
    m_buildAutomator->buildAAB(config);
}

void MainWindow::buildAPK()
{
    if (!validateInputs()) return;
    
    BuildAutomator::BuildConfig config;
    config.projectPath = m_projectPathEdit->text();
    config.outputPath = m_outputPathEdit->text();
    config.keystorePath = m_keystorePathEdit->text();
    config.keystorePassword = m_keystorePasswordEdit->text();
    config.keyAlias = m_keyAliasEdit->text();
    config.keyPassword = m_keyPasswordEdit->text();
    config.buildMode = m_buildModeCombo->currentText();
    config.cleanBuild = m_cleanBuildCheck->isChecked();
    
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0); // Indeterminate progress
    updateBuildButtonStates();
    
    m_buildAutomator->buildAPK(config);
}

void MainWindow::onBuildFinished(bool success, const QString &message)
{
    m_progressBar->setVisible(false);
    updateBuildButtonStates();
    
    if (success) {
        m_logTextEdit->append(QString("\n✅ BUILD SUCCESSFUL: %1\n").arg(message));
        statusBar()->showMessage("Build completed successfully", 3000);
        QMessageBox::information(this, "Build Complete", message);
    } else {
        m_logTextEdit->append(QString("\n❌ BUILD FAILED: %1\n").arg(message));
        statusBar()->showMessage("Build failed", 3000);
        QMessageBox::critical(this, "Build Error", message);
    }
}

void MainWindow::onBuildProgress(const QString &message)
{
    m_logTextEdit->append(message);
    m_logTextEdit->ensureCursorVisible();
    statusBar()->showMessage(message, 1000);
}

void MainWindow::clearLog()
{
    m_logTextEdit->clear();
    m_logTextEdit->append("Log cleared.\n");
}

void MainWindow::updateBuildButtonStates()
{
    bool hasRequiredInputs = !m_projectPathEdit->text().isEmpty() &&
                           !m_outputPathEdit->text().isEmpty() &&
                           !m_keystorePathEdit->text().isEmpty();
    
    bool buildInProgress = m_buildAutomator->isBuildInProgress();
    
    m_buildAABButton->setEnabled(hasRequiredInputs && !buildInProgress);
    m_buildAPKButton->setEnabled(hasRequiredInputs && !buildInProgress);
}

bool MainWindow::validateInputs()
{
    if (m_projectPathEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Missing Input", "Please select a project path.");
        return false;
    }
    
    if (m_outputPathEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Missing Input", "Please select an output path.");
        return false;
    }
    
    if (m_keystorePathEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Missing Input", "Please select a keystore file.");
        return false;
    }
    
    if (m_keystorePasswordEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Missing Input", "Please enter keystore password.");
        return false;
    }
    
    if (m_keyAliasEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Missing Input", "Please enter key alias.");
        return false;
    }
    
    if (m_keyPasswordEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Missing Input", "Please enter key password.");
        return false;
    }
    
    if (!QDir(m_projectPathEdit->text()).exists()) {
        QMessageBox::warning(this, "Invalid Path", "Project path does not exist.");
        return false;
    }
    
    if (!QFileInfo(m_keystorePathEdit->text()).exists()) {
        QMessageBox::warning(this, "Invalid File", "Keystore file does not exist.");
        return false;
    }
    
    return true;
}
