; example1.nsi
;
; This script is perhaps one of the simplest NSIs you can make. All of the
  ; optional settings are left to their default settings. The installer simply
; prompts the user asking them where to install, and drops a copy of example1.nsi
; there. 

;--------------------------------

!define APP "NRedir4Dokan"
!define NP "NRedir4Dokan"

!define sysobj "objfre"
!define npobj "objfre"

; The name of the installer
Name "${APP}"

; The file to write
OutFile "Setup_${APP}.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\${APP}"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

!include "x64.nsh"
!include "LogicLib.nsh"
!include "WordFunc.nsh"

;--------------------------------

; Pages

Page directory
Page components
Page instfiles

InstType "Install"
InstType "Remove"

;--------------------------------

; The stuff to install
Section "" ;No components page, name is not important
  SectionIn 1 2

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  SetRebootFlag true

SectionEnd ; end the section

Section "driver install"
  SectionIn 1
  ${DisableX64FSRedirection}
  SetOutPath "$SYSDIR\drivers"
  ${If} ${RunningX64}
    DetailPrint "win7 x64 driver"
    File "..\sys\${sysobj}_win7_amd64\amd64\NRedir4Dokan.sys"
  ${Else}
    DetailPrint "win7 x86 driver"
    File "..\sys\${sysobj}_win7_x86\i386\NRedir4Dokan.sys"
  ${EndIf}

  ExecWait 'sc.exe create NRedir4Dokan type= filesys start= auto binPath= "$OUTDIR\NRedir4Dokan.sys" DisplayName= ${APP} ' $0
  DetailPrint "Œ‹‰Ê: $0"
  ${EnableX64FSRedirection}
SectionEnd


Section "driver remove"
  SectionIn 2
  ${DisableX64FSRedirection}
  SetOutPath "$SYSDIR\drivers"
  Delete /REBOOTOK "NRedir4Dokan.sys"

  ExecWait 'sc.exe delete NRedir4Dokan ' $0
  DetailPrint "Œ‹‰Ê: $0"
  ${EnableX64FSRedirection}
SectionEnd


Section "np install"
  SectionIn 1
  ${DisableX64FSRedirection}

  ${If} ${RunningX64}
    SetOutPath "$PROGRAMFILES64\${APP}"
    File "..\nrednp\${npobj}_win7_amd64\amd64\nrednp.dll"
  ${EndIf}
  ${If} 1 > 0
    SetOutPath "$PROGRAMFILES32\${APP}"
    File "..\nrednp\${npobj}_win7_x86\i386\nrednp.dll"
  ${EndIf}

  ${EnableX64FSRedirection}

  SetRegView 64
  WriteRegStr       HKLM "SYSTEM\CurrentControlSet\services\${NP}\NetworkProvider" "Name" "${NP}"
  WriteRegExpandStr HKLM "SYSTEM\CurrentControlSet\services\${NP}\NetworkProvider" "ProviderPath" "%ProgramFiles%\${APP}\nrednp.dll"
  SetRegView lastused

  WriteRegStr                  HKLM "SYSTEM\CurrentControlSet\services\${NP}\Map" "" ""
  AccessControl::GrantOnRegKey HKLM "SYSTEM\CurrentControlSet\services\${NP}\Map" "(S-1-1-0)" "QueryValue+SetValue+CreateSubKey+EnumerateSubKeys+Delete+ReadControl+Synchronize"

  ReadRegStr  $1 HKLM "SYSTEM\CurrentControlSet\Control\NetworkProvider\Order" "ProviderOrder"
  ${WordAddS} $1 "," "+${NP}" $2
  WriteRegStr    HKLM "SYSTEM\CurrentControlSet\Control\NetworkProvider\Order" "ProviderOrder" $2
SectionEnd


Section "np delete"
  SectionIn 2
  ${DisableX64FSRedirection}

  ${If} ${RunningX64}
    Delete /REBOOTOK "$PROGRAMFILES64\${APP}\nrednp.dll"
    RMDir  /REBOOTOK "$PROGRAMFILES64\${APP}"
  ${EndIf}
  ${If} 1 > 0
    Delete /REBOOTOK "$PROGRAMFILES32\${APP}\nrednp.dll"
    RMDir  /REBOOTOK "$PROGRAMFILES32\${APP}"
  ${EndIf}

  ${EnableX64FSRedirection}

  ReadRegStr  $1 HKLM "SYSTEM\CurrentControlSet\Control\NetworkProvider\Order" "ProviderOrder"
  ${WordAddS} $1 "," "-${NP}" $2
  WriteRegStr    HKLM "SYSTEM\CurrentControlSet\Control\NetworkProvider\Order" "ProviderOrder" $2
SectionEnd
