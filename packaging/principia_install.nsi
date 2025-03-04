SetCompressor lzma

!include "MUI2.nsh"

!define MUI_ICON "..\packaging\icon.ico"
!define VER_MAJOR 2024
!define VER_MINOR 07
!define VER_BUILD 12

!define VERSION "2024.07.12"

Name "Principia"
OutFile "principia-setup.exe"

InstallDir "$PROGRAMFILES\Principia"

;get install dir from registry if available
InstallDirRegKey HKCU "Software\Bithack\Principia" ""

!define REG_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Principia"

BrandingText "Principia ${VERSION}"

RequestExecutionLevel admin

; Required because otherwise start menu and desktop shortcuts will be installed for only
; the main administrator, even if a regular user escalating to admin installs it.
Function .onInit
SetShellVarContext All
FunctionEnd

!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING

!define MUI_WELCOMEFINISHPAGE_BITMAP "..\packaging\installer\welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "..\packaging\installer\unwelcome.bmp"

!define MUI_FINISHPAGE_RUN "$INSTDIR/principia.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Run Principia"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE.md"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

VIProductVersion ${VER_MAJOR}.${VER_MINOR}.${VER_BUILD}.0
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "FileDescription" "Principia Setup"
VIAddVersionKey "LegalCopyright" "Bithack AB"

Section "Core Files (required)" SecCore

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

SectionEnd

Section "Start Menu entry" SecSM

    CreateDirectory "$SMPROGRAMS\Principia"
    CreateShortCut "$SMPROGRAMS\Principia\Principia.lnk" "$INSTDIR\principia.exe"

    CreateShortCut "$SMPROGRAMS\Principia\Uninstall.lnk" "$INSTDIR\uninst-principia.exe"

SectionEnd

Section "Desktop shortcut" SecDesktop

    CreateShortCut "$DESKTOP\Principia.lnk" "$INSTDIR\principia.exe"

SectionEnd

LangString DESC_SecCore ${LANG_ENGLISH} "Contains the core files required to run Principia."
LangString DESC_SecSM ${LANG_ENGLISH} "Create a Start Menu entry for Principia."
LangString DESC_SecDesktop ${LANG_ENGLISH} "Create a shortcut to Principia on your desktop."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SecCore} $(DESC_SecCore)
!insertmacro MUI_DESCRIPTION_TEXT ${SecSM} $(DESC_SecSM)
!insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} $(DESC_SecDesktop)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section "Uninstall"

    Delete "$INSTDIR\uninst-principia.exe"

    RMDir /r "$INSTDIR"
    RMDir /r "$SMPROGRAMS\Principia"

    DeleteRegKey /ifempty HKCU "Software\Bithack\Principia"
    DeleteRegKey HKCR "principia"
    DeleteRegKey HKLM "${REG_UNINST_KEY}"

SectionEnd
