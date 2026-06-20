SetCompressor lzma
XPStyle on
ManifestDPIAware true

!include "MUI2.nsh"

!define MUI_ICON "..\packaging\icon.ico"
!define VER_MAJOR 2026
!define VER_MINOR 06
!define VER_BUILD 19

!define VERSION "2026.06.19"

!define LOGO_FILE "install_logo.bmp"
!define LOGO_PATH "..\packaging\${LOGO_FILE}"

Name "Principia"
OutFile "principia-setup.exe"

InstallDir "$PROGRAMFILES\Principia"

;get install dir from registry if available
InstallDirRegKey HKCU "Software\Bithack\Principia" ""

!define REG_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Principia"

VIProductVersion ${VER_MAJOR}.${VER_MINOR}.${VER_BUILD}.0
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "FileDescription" "Principia Setup"
VIAddVersionKey "LegalCopyright" "Bithack AB"

BrandingText " "

RequestExecutionLevel admin

Function .onInit
    ; Required because otherwise start menu and desktop shortcuts will be installed for only
    ; the main administrator, even if a regular user escalating to admin installs it.
    SetShellVarContext All

    InitPluginsDir
    File "/oname=$PLUGINSDIR\${LOGO_FILE}" "${LOGO_PATH}"
FunctionEnd

; ---
; Welcome page

Page Custom WelcomePageCreate WelcomePageLeave

Var Dialog
Var Logo
Var VersionLabel
Var BmpHandle
Var DirText
Var DirBrowseBtn
Var DesktopShortcutCheckbox
Var CreateDesktopShortcut

Function OnBrowseDir
    nsDialogs::SelectFolderDialog "Select the folder to install Principia in:" "$INSTDIR"
    Pop $0

    ; user cancelled or failed
    StrCmp $0 "" done
    StrCmp $0 "error" done

    ; concatenate \Principia to the path
    ; if there is no trailing backslash, add one
    StrCpy $1 $0 1 -1
    ${If} $1 != "\"
        StrCpy $0 "$0\"
    ${EndIf}
    StrCpy $0 "$0Principia"

    ; update state
    StrCpy $INSTDIR $0
    SendMessage $DirText ${WM_SETTEXT} 0 "STR:$INSTDIR"

done:
FunctionEnd

Function WelcomePageCreate

    nsDialogs::Create 1044
    Pop $Dialog

    ${If} $Dialog == error
        Abort
    ${EndIf}

    ; Top Logo
    ${NSD_CreateBitmap} 123u 10u 96u 96u ""
    Pop $Logo
    ${NSD_SetImage} $Logo "$PLUGINSDIR\${LOGO_FILE}" $BmpHandle

    ; Version label
    ${NSD_CreateLabel} 0u 100u 100% 12u "Installing Principia ${VERSION}"
    Pop $VersionLabel
    ${NSD_AddStyle} $VersionLabel ${SS_CENTER}
    CreateFont $0 "Segoe UI" 12 700
    SendMessage $VersionLabel ${WM_SETFONT} $0 1

    ; Checkbox for creating desktop shortcut
    ${NSD_CreateCheckbox} 20u 130u 100% 12u "Create desktop shortcut"
    Pop $DesktopShortcutCheckbox
    ${NSD_Check} $DesktopShortcutCheckbox ; check by default

    ; Horizontal line
    ${NSD_CreateHLine} 9u 147u 317u 5u ""
    Pop $0

    ; Destination folder label
    ${NSD_CreateLabel} 20u 155u 80u 12u "Destination folder:"
    Pop $0

    ; Input field
    ${NSD_CreateText} 20u 170u 230u 14u "$INSTDIR"
    Pop $DirText
    EnableWindow $DirText 0

    ; Browse button
    ${NSD_CreateButton} 255u 169u 55u 16u "Browse"
    Pop $DirBrowseBtn
    ${NSD_OnClick} $DirBrowseBtn OnBrowseDir

    nsDialogs::Show

FunctionEnd

Function WelcomePageLeave
    ${NSD_GetState} $DesktopShortcutCheckbox $CreateDesktopShortcut
FunctionEnd

; ---
; Installation
; ---

!insertmacro MUI_PAGE_INSTFILES

AutoCloseWindow true

Section "" SecCore

    SectionIn RO
    SetOverwrite on

    SetOutPath "$INSTDIR"

    File "release\principia.exe"
    File /x "opengl32.dll" "release\*.dll"

    File /r "release\lib"
    File /r "release\share"

    File /r /x android "..\data"

    WriteRegStr HKCR "principia" "" "URL:Principia"
    WriteRegStr HKCR "principia" "URL Protocol" ""
    WriteRegStr HKCR "principia" "DefaultIcon" ""
    WriteRegStr HKCR "principia\shell\open\command" "" '"$INSTDIR\principia.exe" %1'

    WriteRegStr HKCU "Software\Bithack\Principia" "" $INSTDIR

    WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayName" "Principia"
    WriteRegStr HKLM "${REG_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninst-principia.exe"'
    WriteRegStr HKLM "${REG_UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayIcon" '"$INSTDIR\principia.exe"'
    WriteRegStr HKLM "${REG_UNINST_KEY}" "Publisher" "Bithack AB"
    WriteRegStr HKLM "${REG_UNINST_KEY}" "URLInfoAbout" "https://principia-web.se/"
    WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayVersion" "${VERSION}"

    SectionGetSize "${SecCore}" $0
    WriteRegDWORD HKLM "${REG_UNINST_KEY}" "EstimatedSize" "$0"

    WriteUninstaller "$INSTDIR\uninst-principia.exe"

    ; Start menu entry
    CreateDirectory "$SMPROGRAMS\Principia"
    CreateShortCut "$SMPROGRAMS\Principia\Principia.lnk" "$INSTDIR\principia.exe"
    CreateShortCut "$SMPROGRAMS\Principia\Uninstall.lnk" "$INSTDIR\uninst-principia.exe"

    ; Desktop shortcut
    ${If} $CreateDesktopShortcut == ${BST_CHECKED}
        CreateShortCut "$DESKTOP\Principia.lnk" "$INSTDIR\principia.exe"
    ${EndIf}

SectionEnd

; ---
; Finish dialog
; ---

Page Custom FinishPageCreate FinishPageLeave

var LaunchPrincipiaCheckbox
var LaunchPrincipia

Function FinishPageCreate

    nsDialogs::Create 1044
    Pop $Dialog

    SendMessage $mui.Button.Next ${WM_SETTEXT} 0 "STR:Finish"

    ${If} $Dialog == error
        Abort
    ${EndIf}

    ; Top Logo
    ${NSD_CreateBitmap} 123u 10u 96u 96u ""
    Pop $Logo
    ${NSD_SetImage} $Logo "$PLUGINSDIR\${LOGO_FILE}" $BmpHandle

    ; Version label
    ${NSD_CreateLabel} 0u 100u 100% 12u "Principia has been successfully installed"
    Pop $VersionLabel
    ${NSD_AddStyle} $VersionLabel ${SS_CENTER}
    CreateFont $0 "Segoe UI" 12 700
    SendMessage $VersionLabel ${WM_SETFONT} $0 1

    ; Checkbox for whether to launch principia
    ${NSD_CreateCheckbox} 20u 130u 100% 12u "Launch Principia"
    Pop $LaunchPrincipiaCheckbox
    ${NSD_Check} $LaunchPrincipiaCheckbox ; check by default

    nsDialogs::Show
FunctionEnd

Function FinishPageLeave
    ${NSD_GetState} $LaunchPrincipiaCheckbox $LaunchPrincipia

    ${If} $LaunchPrincipia == ${BST_CHECKED}
        Exec '"$INSTDIR\principia.exe"'
    ${EndIf}
FunctionEnd

; ---
; Uninstallation
; ---

SilentUnInstall silent
ShowUninstDetails nevershow

Section "Uninstall"
    MessageBox MB_ICONQUESTION|MB_YESNO \
        "Are you sure you want to uninstall Principia?" \
        IDYES continue

    Abort

continue:

    Delete "$INSTDIR\uninst-principia.exe"

    RMDir /r "$INSTDIR"
    RMDir /r "$SMPROGRAMS\Principia"

    DeleteRegKey /ifempty HKCU "Software\Bithack\Principia"
    DeleteRegKey HKCR "principia"
    DeleteRegKey HKLM "${REG_UNINST_KEY}"

    MessageBox MB_OK|MB_ICONINFORMATION \
        "Principia has been successfully uninstalled."

SectionEnd

!insertmacro MUI_LANGUAGE "English"
