#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>
#include <string>
#include "BuildAutomator.h"

class MainWindow : public Fl_Window
{
public:
    MainWindow();
    ~MainWindow();

private:
    static void selectProjectPath_cb(Fl_Widget*, void*);
    static void selectOutputPath_cb(Fl_Widget*, void*);
    static void selectKeystore_cb(Fl_Widget*, void*);
    static void buildAAB_cb(Fl_Widget*, void*);
    static void buildAPK_cb(Fl_Widget*, void*);
    static void clearLog_cb(Fl_Widget*, void*);
    
    void setupUI();
    void updateBuildButtonStates();
    bool validateInputs();
    
    // UI Components
    Fl_Group *m_inputGroup;
    Fl_Input *m_projectPathEdit;
    Fl_Button *m_projectPathButton;
    Fl_Input *m_outputPathEdit;
    Fl_Button *m_outputPathButton;
    Fl_Input *m_keystorePathEdit;
    Fl_Button *m_keystoreButton;
    Fl_Input *m_keystorePasswordEdit;
    Fl_Input *m_keyAliasEdit;
    Fl_Input *m_keyPasswordEdit;
    
    // Build options group
    Fl_Group *m_optionsGroup;
    Fl_Choice *m_buildModeCombo;
    Fl_Check_Button *m_cleanBuildCheck;
    
    // Action group
    Fl_Group *m_actionGroup;
    Fl_Button *m_buildAABButton;
    Fl_Button *m_buildAPKButton;
    Fl_Button *m_clearLogButton;
    
    // Progress and logging
    Fl_Group *m_logGroup;
    Fl_Progress *m_progressBar;
    Fl_Text_Display *m_logTextEdit;
    Fl_Text_Buffer *m_logBuffer;
    
    // Business logic
    BuildAutomator *m_buildAutomator;
    
    // Helper methods
    void logMessage(const std::string& message);
    void onBuildFinished(bool success, const std::string& message);
    void onBuildProgress(const std::string& message);
};

#endif // MAINWINDOW_H
