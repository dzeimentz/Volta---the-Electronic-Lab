; Windows-Installer fuer die Testversion von Volta 1 (NSIS 3, Unicode). Aufruf ueber tools/build_testpaket.py:
;   makensis /DVERSION=0.9.0 /DSRC=<Programmordner> /DOUT=<Setup.exe> /DICON=<volta.ico> volta1.nsi
; Installation nur fuer den angemeldeten Benutzer (ohne Administratorrechte) nach %LOCALAPPDATA%\Programs\Volta 1,
; Verknuepfung im Startmenue und optional auf dem Desktop, Deinstallation ueber »Apps« in den Windows-Einstellungen.
; Fortschritt und Einstellungen liegen im privaten App-Speicher und bleiben bei der Deinstallation erhalten.
; Nicht signiert (Testversion): Windows SmartScreen fragt beim ersten Start nach.

Unicode true
!include "MUI2.nsh"

!define APPNAME "Volta 1"
!define PUBLISHER "DC Software Engineering"
!define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Volta1"

Name "${APPNAME} – Das Elektronik-Labor"
OutFile "${OUT}"
InstallDir "$LOCALAPPDATA\Programs\${APPNAME}"
InstallDirRegKey HKCU "${UNINSTKEY}" "InstallLocation"
RequestExecutionLevel user
SetCompressor /SOLID lzma
BrandingText "${PUBLISHER} · Testversion ${VERSION}"
VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "${APPNAME} – Das Elektronik-Labor"
VIAddVersionKey "CompanyName" "${PUBLISHER}"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "FileDescription" "Installation ${APPNAME} (Testversion)"
VIAddVersionKey "LegalCopyright" "© 2026 ${PUBLISHER}"

!define MUI_ICON "${ICON}"
!define MUI_UNICON "${ICON}"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\volta1.exe"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "French"

LangString SecProgram ${LANG_GERMAN} "Programm"
LangString SecProgram ${LANG_ENGLISH} "Program"
LangString SecProgram ${LANG_FRENCH} "Programme"
LangString SecDesktop ${LANG_GERMAN} "Verknüpfung auf dem Desktop"
LangString SecDesktop ${LANG_ENGLISH} "Desktop shortcut"
LangString SecDesktop ${LANG_FRENCH} "Raccourci sur le bureau"

Function .onInit
  !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

Section "!$(SecProgram)" SecMain
  SectionIn RO
  SetOutPath "$INSTDIR"
  ; alte Programmdateien entfernen (Daten des Kindes liegen nicht hier)
  RMDir /r "$INSTDIR\qml"
  RMDir /r "$INSTDIR\content"
  File /r "${SRC}\*.*"
  File "/oname=volta.ico" "${ICON}"
  WriteUninstaller "$INSTDIR\Deinstallieren.exe"
  CreateShortcut "$SMPROGRAMS\${APPNAME}.lnk" "$INSTDIR\volta1.exe" "" "$INSTDIR\volta.ico"
  WriteRegStr HKCU "${UNINSTKEY}" "DisplayName" "${APPNAME} – Das Elektronik-Labor (Testversion)"
  WriteRegStr HKCU "${UNINSTKEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr HKCU "${UNINSTKEY}" "Publisher" "${PUBLISHER}"
  WriteRegStr HKCU "${UNINSTKEY}" "DisplayIcon" "$INSTDIR\volta.ico"
  WriteRegStr HKCU "${UNINSTKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "${UNINSTKEY}" "UninstallString" '"$INSTDIR\Deinstallieren.exe"'
  WriteRegStr HKCU "${UNINSTKEY}" "URLInfoAbout" "https://dc-software-engineering.com"
  WriteRegDWORD HKCU "${UNINSTKEY}" "NoModify" 1
  WriteRegDWORD HKCU "${UNINSTKEY}" "NoRepair" 1
SectionEnd

Section "$(SecDesktop)" SecDesktopLink
  CreateShortcut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\volta1.exe" "" "$INSTDIR\volta.ico"
SectionEnd

Section "Uninstall"
  Delete "$SMPROGRAMS\${APPNAME}.lnk"
  Delete "$DESKTOP\${APPNAME}.lnk"
  RMDir /r "$INSTDIR"
  DeleteRegKey HKCU "${UNINSTKEY}"
SectionEnd
