SetCompressor lzma

!include "MUI2.nsh"

!define MUI_ICON "..\packaging\icon.ico"
!define /ifndef VER_MAJOR 1
!define /ifndef VER_MINOR 5
!define /ifndef VER_BUILD 2

!define /ifndef VER_REVISION 1520

!define /ifndef VERSION '1.5.2 Beta'

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

!ifdef VER_MAJOR & VER_MINOR & VER_REVISION & VER_BUILD
VIProductVersion ${VER_MAJOR}.${VER_MINOR}.${VER_REVISION}.${VER_BUILD}
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "FileDescription" "Principia Setup"
VIAddVersionKey "LegalCopyright" "Bithack AB"
!endif

Section "Core Files (required)" SecCore

  SectionIn RO
  SetOverwrite on

  SetOutPath "$INSTDIR"

  File "release\principia.exe"
  File /x "opengl32.dll" "release\*.dll"

  File /r "release\lib"
  File /r "release\share"

  SetOutPath "$INSTDIR\data-pc"
  File /r /x *.sw* "..\data-pc\bg"
  File /r /x *.sw* "..\data-pc\sfx"
  File /r /x *.sw* "..\data-pc\textures"
  SetOutPath "$INSTDIR\data-shared"
  File /r /x *.sw* "..\data-shared\bg"
  File /r /x *.sw* "..\data-shared\fonts"
  File /r /x *.sw* "..\data-shared\icons"
  File /r /x *.sw* "..\data-shared\lvl"
  File /r /x *.sw* "..\data-shared\models"
  File /r /x *.sw* "..\data-shared\pkg"
  File /r /x *.sw* "..\data-shared\shaders"
  File /r /x *.sw* "..\data-shared\textures"
  File /r /x *.sw* "..\data-shared\tiles"

SectionEnd

Section "Start Menu entry" SecSM

  CreateDirectory $SMPROGRAMS\Principia
  CreateShortCut $SMPROGRAMS\Principia\Principia.lnk $INSTDIR\principia.exe

  CreateShortCut $SMPROGRAMS\Principia\Uninstall.lnk $INSTDIR\uninst-principia.exe

SectionEnd

Section "Desktop shortcut" SecDesktop

  CreateShortCut $DESKTOP\Principia.lnk $INSTDIR\principia.exe

SectionEnd

Section -post

  WriteRegStr HKCR "principia" "" "URL:Principia"
  WriteRegStr HKCR "principia" "URL Protocol" ""
  WriteRegStr HKCR "principia" "DefaultIcon" ""
  WriteRegStr HKCR "principia\shell\open\command" "" '"$INSTDIR\principia.exe" %1'

  WriteRegStr HKCU "Software\Bithack\Principia" "" $INSTDIR
  !ifdef VER_MAJOR & VER_MINOR & VER_REVISION & VER_BUILD
    WriteRegDword HKCU "Software\Bithack\Principia" "VersionMajor" "${VER_MAJOR}"
    WriteRegDword HKCU "Software\Bithack\Principia" "VersionMinor" "${VER_MINOR}"
    WriteRegDword HKCU "Software\Bithack\Principia" "VersionRevision" "${VER_REVISION}"
    WriteRegDword HKCU "Software\Bithack\Principia" "VersionBuild" "${VER_BUILD}"
  !endif

  WriteRegExpandStr HKLM "${REG_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninst-principia.exe"'
  WriteRegExpandStr HKLM "${REG_UNINST_KEY}" "InstallLocation" "$INSTDIR"

  WriteUninstaller "$INSTDIR\uninst-principia.exe"
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

  ; TODO: additional option on uninstall, if the ~/Principia-folder should be deleted

  DeleteRegKey /ifempty HKCU "Software\Bithack\Principia"
  DeleteRegKey HKCR "principia"

SectionEnd
