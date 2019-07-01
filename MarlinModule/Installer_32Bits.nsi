;-------------------------------------------------------
;
; File:     MarlinModule\Installer_32BIts.nsi
; Author:   W.E. Huisman
;
; Copyright (c) 2019 Edwig Huisman
;
; Date of last change: 01-07-2019
; Version:             6.1
;-------------------------------------------------------
 !define PRODUCT_NAME                         "Marlin IIS Module 32Bits"
 !define PRODUCT_VERSION                      "6.1.0.0"
 !define PRODUCT_BUILDNUMBER                  "10"
 !define PRODUCT_PUBLISHER                    "Edwig Huisman"
 !define PRODUCT_WEB_SITE                     "https://github.com/Edwig/Marlin"
 !define PRODUCT_DIR_REGKEY                   "Software\Microsoft\Windows\CurrentVersion\App Paths\${PRODUCT_NAME}"
 !define PRODUCT_UNINST_KEY                   "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
 !define PRODUCT_UNINST_ROOT_KEY              "HKLM"
 !define MUI_HEADERIMAGE
 !define MUI_HEADERIMAGE_BITMAP               "Marlin.bmp"
 !define MUI_DIRECTORYPAGE_VERIFYONLEAVE
 !define LogFile                              "MarlinIISModule_32bits"
  
 ; Directories containing our files
 !define InputDirectory32                     "C:\Develop\Marlin\BinRelease_Win32"
 !define RedistMap                            "C:\Develop\Marlin\Documentation"

;--------------------------------------------------------------------------------------------------------
 ;variables
 var currentVersion
 
;--------------------------------------------------------------------------------------------------------
 !include "MUI.nsh"
;--------------------------------------------------------------------------------------------------------

; This compressions saves about 40% diskspace
SetCompressor LZMA
SetCompressorDictSize 8
;--------------------------------------------------------------------------------------------------------
 SetPluginUnload alwaysoff
 BrandingText /TRIMRIGHT "Marlin"
 InstallColors 000000 FFFFFF
 XPStyle on
 RequestExecutionLevel admin
;--------------------------------------------------------------------------------------------------------
 OutFile "MarlinModule_${PRODUCT_VERSION}_32_bits.exe"
;--------------------------------------------------------------------------------------------------------
 ; title of the setup
 Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
;--------------------------------------------------------------------------------------------------------
 ; Places it in the standard installation folder
 ; IIS is never in program files, but in it's own directory under MS-Windows
 ; InstallDir  "$PROGRAMFILES\${PRODUCT_PUBLISHER}"
InstallDir "$WINDIR\system32\inetsrv\"
;--------------------------------------------------------------------------------------------------------
 ; MUI Settings
 !define MUI_ABORTWARNING
 !define MUI_ICON   "Marlin.ico"
 !define MUI_UNICON "Marlin.ico"
;--------------------------------------------------------------------------------------------------------
 ; Language Selection Dialog Settings
 !define MUI_LANGDLL_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
 !define MUI_LANGDLL_REGISTRY_KEY  "${PRODUCT_UNINST_KEY}"
 !define MUI_LANGDLL_REGISTRY_VALUENAME "NSIS:Language"
;--------------------------------------------------------------------------------------------------------
 ; Welkom page
 !insertmacro MUI_PAGE_WELCOME
;--------------------------------------------------------------------------------------------------------
 ; Directory page
 ; !insertmacro MUI_PAGE_DIRECTORY
 !insertmacro MUI_PAGE_INSTFILES
;--------------------------------------------------------------------------------------------------------
 !insertmacro MUI_PAGE_FINISH
;--------------------------------------------------------------------------------------------------------
; Uninstaller pages
 !insertmacro MUI_UNPAGE_INSTFILES
;--------------------------------------------------------------------------------------------------------
; Language files
 !insertmacro MUI_LANGUAGE "Dutch"
;--------------------------------------------------------------------------------------------------------
 ShowInstDetails   show
 ShowUnInstDetails show
;--------------------------------------------------------------------------------------------------------
; Really handy functions
!include Logfile.nsh

; Stuur logtext naar window én naar LogFile
;--------------------------------------------------------------------------------------------------------
!macro LogDetail LogLine
  DetailPrint      "${LogLine}"
  !insertmacro Log "${LogLine}"
!macroend

;--------------------------------------------------------------------------------------------------------
Function .onInit
 strcpy $R0 "$TEMP\${LogFile}.log"
 call OpenLogfile
 
 !insertmacro Log "Installing and registering the MarlinModule 32-Bits version: ${PRODUCT_VERSION} buildnummer ${PRODUCT_BUILDNUMBER}"
 !insertmacro Log ""
 ReadEnvStr $1 "USERNAME"
 !insertmacro Log "Processed by $1 on ${__DATE__} ${__TIME__}"
 !insertmacro LogLine
 !insertmacro Log "Initialising the installer."
 !insertmacro LogLine
  
;Check if a newer version of the product is already installed
 Readregstr $currentVersion "${PRODUCT_UNINST_ROOT_KEY}" "${PRODUCT_UNINST_KEY}" "BuildNumber"
 
 IfSilent SilentLogging
 
 InitPluginsDir
 File /oname=$PLUGINSDIR\marlin.bmp "marlin.bmp"
 IfSilent +2
 advsplash::show 1000 1000 500 0xFFFFFF $PLUGINSDIR\Marlin
 
 IntCmp $currentVersion ${PRODUCT_BUILDNUMBER} versionSame SetupNewerVersion CurrentVersionNewer

 versionSame:
  Messagebox MB_YESNO "This version of ${PRODUCT_NAME} is already installed. Do you want to re-install it?. " /SD IDYES IDNO doNotRepair
  !insertmacro Log "Reinstalling product ${PRODUCT_NAME}."
  goto SetupNewerVersion
  
 doNotRepair:
  !insertmacro Log "User has chosen NOT to repair the product."
  !insertmacro Log "Setup is ready."
  abort
  
 CurrentVersionNewer:
  !insertmacro Log "There is already a newer version of  ${PRODUCT_NAME} installed on your system."
  !insertmacro Log "Version: ${PRODUCT_VERSION}.$currentVersion"
  Messagebox MB_YESNO "There is already a newer version of  ${PRODUCT_NAME} installed on your system. \
                       Do you want to overwrite the newer version with this version?" \
                      /SD IDNO IDYES doInstall

  abort
 doInstall:
 !insertmacro Log "User has chosen to install an older version of the product."
 Delete "$INSTDIR\uninstall ${PRODUCT_NAME} $currentVersion.exe"
 
 SetupNewerVersion:
  goto InitEnd

 SilentLogging:
   !insertmacro Log "This is a 'silent' installation, onrelate to a previous installed version."
   !insertmacro Log "The current version WAS: $currentVersion"
   !insertmacro Log "The now installed version IS: ${PRODUCT_BUILDNUMBER}"

 InitEnd:
; !insertmacro MUI_LANGDLL_DISPLAY
functionend

;--------------------------------------------------------------------------------------------------------
Section "MarlinModule"

 ; Expliciet overschrijven van files
 SetOverwrite on

 SetOutPath "$INSTDIR"
 !insertmacro LogDetail "Output directory set to: $INSTDIR"
 !insertmacro LogDetail "Copiing files."

 File /r "${InputDirectory32}\MarlinModule.dll"

 ; Registering the module
 !insertmacro LogDetail "Registering the MarlinModule with IIS"
 ExecWait '"$INSTDIR\appcmd.exe" install module /name:MarlinModule /image:"$INSTDIR\MarlinModule.dll"' $0
 !insertmacro LogDetail "Registering MarlinModule with appcmd returned: $0"

SectionEnd

;--------------------------------------------------------------------------------------------------------
Section -Post
 WriteUninstaller "$SYSDIR\inetsrv\Uninstall ${PRODUCT_NAME} ${PRODUCT_VERSION}.${PRODUCT_BUILDNUMBER}.exe"
 
 WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName"     "$(^Name)"
 WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$WINDIR\SysWOW64\inetsrv\Uninstall ${PRODUCT_NAME} ${PRODUCT_VERSION}.${PRODUCT_BUILDNUMBER}.exe"
 WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon"     "$WINDIR\SysWOW64\inetsrv\Uninstall ${PRODUCT_NAME} ${PRODUCT_VERSION}.${PRODUCT_BUILDNUMBER}.exe"
 WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion"  "${PRODUCT_VERSION}.${PRODUCT_BUILDNUMBER}"
 WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout"    "${PRODUCT_WEB_SITE}"
 WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher"       "${PRODUCT_PUBLISHER}"
 WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "BuildNumber"     "${PRODUCT_BUILDNUMBER}"

 ; deze sectie wordt als laatste aangelopen
 ; nu kunnen we het LogFile afsluiten.
 call CloseLogfile
 MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "The installation of ${PRODUCT_NAME} ${PRODUCT_VERSION} is ready" /SD IDNO IDNO 
SectionEnd

;--------------------------------------------------------------------------------------------------------
Function un.onUninstSuccess
 HideWindow
 MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) has been removed from your computer." /SD IDOK 
FunctionEnd

;--------------------------------------------------------------------------------------------------------
Function un.onInit
 MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure that you want to remove $(^Name) and all its components completely?" /SD IDYES IDYES +2
 Abort
FunctionEnd

;--------------------------------------------------------------------------------------------------------
Section Uninstall
 SetAutoClose true
 strcpy $R0 "$TEMP\Uninstaller ${LogFile}.log"
 Call Un.OpenLogfile
 !insertmacro LogDetail "De-installation and de-registration of ${PRODUCT_NAME} version: ${PRODUCT_VERSION} buildnummer ${PRODUCT_BUILDNUMBER}"

 ; UN-Registering the module
 !insertmacro LogDetail "Deleting the MarlinModule from IIS"
 ExecWait '"$SYSDIR\inetsrv\appcmd.exe" delete module MarlinModule' $0
 !insertmacro LogDetail "Deleting MarlinModule with appcmd returned: $0"

 !insertmacro LogDetail "Uninstall the MarlinModule from IIS"
 ExecWait '"$SYSDIR\inetsrv\appcmd.exe" uninstall module MarlinModule' $0
 !insertmacro LogDetail "Uninstalling MarlinModule with appcmd returned: $0"

 !insertmacro LogDetail "Removing files from : $SYSDIR\inetsrv"
 Delete /REBOOTOK "$SYSDIR\inetsrv\MarlinModule.dll"
 
  ;De-Registreren van het product.
 DetailPrint "De-register of ${PRODUCT_NAME}"
 Delete "$INSTDIR\Uninstall ${PRODUCT_NAME} $currentVersion.exe"
 DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
 
 !insertmacro LogDetail "End of de-registration of the ${PRODUCT_NAME}"
 !insertmacro LogDetail "End of the uninstaller ${PRODUCT_NAME} ${PRODUCT_VERSION}."
 Call Un.CloseLogfile
SectionEnd
;--------------------------------------------------------------------------------------------------------
; Einde script

