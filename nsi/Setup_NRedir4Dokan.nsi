; example1.nsi
;
; This script is perhaps one of the simplest NSIs you can make. All of the
  ; optional settings are left to their default settings. The installer simply
; prompts the user asking them where to install, and drops a copy of example1.nsi
; there. 

;--------------------------------

!define APP "NRedir4Dokan"
!define NP "NRedir4Dokan"

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
Page instfiles

;--------------------------------

; The stuff to install
Section "" ;No components page, name is not important

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  SetRebootFlag true

SectionEnd ; end the section

Section "driver install"
  ${DisableX64FSRedirection}
  SetOutPath "$SYSDIR\drivers"
  ${If} ${RunningX64}
    DetailPrint "win7 x64 driver"
    File "..\sys\objchk_win7_amd64\amd64\NRedir4Dokan.sys"
  ${Else}
    DetailPrint "win7 x86 driver"
    File "..\sys\objchk_win7_x86\i386\NRedir4Dokan.sys"
  ${EndIf}

  ExecWait 'sc.exe create NRedir4Dokan type= filesys start= auto binPath= "$OUTDIR\NRedir4Dokan.sys" DisplayName= ${APP} ' $0
  DetailPrint "Œ‹‰Ê: $0"
  ${EnableX64FSRedirection}
  
SectionEnd

Section "np install"
  ${DisableX64FSRedirection}

  ${If} ${RunningX64}
    SetOutPath "$PROGRAMFILES64\${APP}"
    File "..\nred_np\objchk_win7_amd64\amd64\nrednp.dll"
  ${EndIf}
  ${If} 1 > 0
    SetOutPath "$PROGRAMFILES32\${APP}"
    File "..\nred_np\objchk_win7_x86\i386\nrednp.dll"
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
