; React Native Build Automator Installer
; NSIS Installer Script

!define APP_NAME "React Native Build Automator"
!define APP_VERSION "1.0.0"
!define APP_PUBLISHER "Your Company"
!define APP_EXE "RNBuildAutomator.exe"
!define APP_ID "RNBuildAutomator"

; Include modern UI
!include "MUI2.nsh"
!include "LogicLib.nsh"

; General
Name "${APP_NAME}"
OutFile "RNBuildAutomator-Setup.exe"
InstallDir "$PROGRAMFILES\${APP_NAME}"
InstallDirRegKey HKCU "Software\${APP_NAME}" "Install_Dir"

; Request application privileges
RequestExecutionLevel admin

; Interface Settings
!define MUI_ABORTWARNING

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Language
!insertmacro MUI_LANGUAGE "English"

; Installer Sections
Section "Main Application" SecMain
    SectionIn RO
    
    ; Set output path to the installation directory
    SetOutPath $INSTDIR
    
    ; Copy main executable
    File "build\RNBuildAutomator.exe"
    
    ; Copy bundletool.jar
    File "bundletool.jar"
    
    ; Write the installation path into the registry
    WriteRegStr HKLM "SOFTWARE\${APP_NAME}" "Install_Dir" "$INSTDIR"
    
    ; Write the uninstall information to the registry
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayIcon" "$INSTDIR\${APP_EXE}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher" "${APP_PUBLISHER}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion" "${APP_VERSION}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoRepair" 1
    
    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    
    ; Create start menu shortcuts
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortCut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
    CreateShortCut "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
    
    ; Create desktop shortcut
    CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
SectionEnd

Section "Java Runtime Check" SecJava
    ; Check if Java is installed
    ReadRegStr $R0 HKLM "SOFTWARE\JavaSoft\Java Runtime Environment" "CurrentVersion"
    ${If} $R0 == ""
        ; Java not found, show warning
        MessageBox MB_YESNO|MB_ICONQUESTION "Java Runtime Environment (JRE) was not detected on your system.$\r$\n$\r$\nThis application requires Java to convert AAB files to APK files.$\r$\n$\r$\nWould you like to download and install Java now?" IDYES downloadJava IDNO skipJava
        downloadJava:
            ExecWait '"$WINDIR\System32\rundll32.exe" "$WINDIR\System32\shell32.dll,ShellExec_RunDLL" "https://adoptium.net/temurin/releases/"'
        skipJava:
            MessageBox MB_OK|MB_ICONINFORMATION "You can install Java later from: https://adoptium.net/temurin/releases/"
    ${EndIf}
SectionEnd

Section "Android SDK Check" SecAndroid
    ; Check if ANDROID_HOME is set
    ReadEnvStr $R0 ANDROID_HOME
    ${If} $R0 == ""
        ; ANDROID_HOME not set, show information
        MessageBox MB_OK|MB_ICONINFORMATION "Android SDK environment variable (ANDROID_HOME) was not detected.$\r$\n$\r$\nTo build APK files, you'll need to install Android Studio or Android SDK and set the ANDROID_HOME environment variable.$\r$\n$\r$\nYou can install Android Studio from: https://developer.android.com/studio"
    ${EndIf}
SectionEnd

; Uninstaller Section
Section "Uninstall"
    ; Remove registry keys
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
    DeleteRegKey HKLM "SOFTWARE\${APP_NAME}"
    
    ; Remove files and uninstaller
    Delete "$INSTDIR\${APP_EXE}"
    Delete "$INSTDIR\bundletool.jar"
    Delete "$INSTDIR\Uninstall.exe"
    
    ; Remove shortcuts
    Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk"
    Delete "$DESKTOP\${APP_NAME}.lnk"
    
    ; Remove directories
    RMDir "$SMPROGRAMS\${APP_NAME}"
    RMDir "$INSTDIR"
SectionEnd
