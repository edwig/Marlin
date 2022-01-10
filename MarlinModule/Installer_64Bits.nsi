;-------------------------------------------------------
;
; File:     MarlinModule\Installer_64BIts.nsi
; Author:   W.E. Huisman
;
; Copyright (c) 2019 Edwig Huisman
;
; Date of last change: 10-01-2022
; Version:             7.3
;-------------------------------------------------------
 !define PRODUCT_NAME                         "Marlin IIS Module 64Bits"
 !define PRODUCT_VERSION                      "7.3.0"
 !define PRODUCT_EXT                          "730"
 !define PRODUCT_PUBLISHER                    "Edwig Huisman"
 !define PRODUCT_WEB_SITE                     "https://github.com/Edwig/Marlin"
 !define PRODUCT_DIR_REGKEY                   "Software\Microsoft\Windows\CurrentVersion\App Paths\${PRODUCT_NAME}"
 !define PRODUCT_UNINST_KEY                   "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
 !define PRODUCT_UNINST_ROOT_KEY              "HKLM"
 !define MUI_HEADERIMAGE
 !define MUI_HEADERIMAGE_BITMAP               "Marlin.bmp"
 !define MUI_DIRECTORYPAGE_VERIFYONLEAVE
 !define LogFile                              "MarlinIISModule_64bits"
  
 ; Directories containing our files
 !define InputDirectory64                     "C:\Develop\Marlin\BinRelease_x64"
 !define ExtraDirectory                       "C:\Develop\Marlin\ExtraParts"
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
 OutFile "MarlinModule_${PRODUCT_VERSION}_64_bits.exe"
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
 !define MUI_WELCOMEFINISHPAGE_BITMAP "jumping.bmp"
 !define MUI_UN1WELCOMEFINISHPAGE_BITMAP "jumping.bmp"
;--------------------------------------------------------------------------------------------------------
 ; Language Selection Dialog Settings
 !define MUI_LANGDLL_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
 !define MUI_LANGDLL_REGISTRY_KEY  "${PRODUCT_UNINST_KEY}"
 !define MUI_LANGDLL_REGISTRY_VALUENAME "NSIS:Language"
;--------------------------------------------------------------------------------------------------------
 ; Welcome page
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

; Send logtext to window AND to the logfile
;--------------------------------------------------------------------------------------------------------
!macro LogDetail LogLine
  DetailPrint      "${LogLine}"
  !insertmacro Log "${LogLine}"
!macroend

;--------------------------------------------------------------------------------------------------------
Function .onInit
 strcpy $R0 "$TEMP\${LogFile}.log"
 call OpenLogfile
 
 !insertmacro Log "Installing and registering the MarlinModule 64-Bits version: ${PRODUCT_VERSION}"
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
 
 StrCmp $currentVersion ${PRODUCT_VERSION} versionSame SetupNewerVersion

 versionSame:
  Messagebox MB_YESNO "This version of ${PRODUCT_NAME} is already installed. Do you want to re-install it?. " /SD IDYES IDNO doNotInstall
  !insertmacro Log "Reinstalling product ${PRODUCT_NAME}."
  goto SetupNewerVersion
  
 doNotInstall:
  !insertmacro Log "User has chosen NOT to install the product."
  !insertmacro Log "Setup is ready."
  abort
  
 SetupNewerVersion:
  goto InitEnd

 SilentLogging:
   !insertmacro Log "This is a 'silent' installation, onrelate to a previous installed version."
   !insertmacro Log "The current version WAS: $currentVersion"
   !insertmacro Log "The now installed version IS: ${PRODUCT_VERSION}"

 InitEnd:
; !insertmacro MUI_LANGDLL_DISPLAY
functionend

;--------------------------------------------------------------------------------------------------------
Section "MarlinModule"

 ; Explicitly overwriting files
 SetOverwrite on

 SetOutPath "$INSTDIR"
 !insertmacro LogDetail "Output directory set to: $INSTDIR"
 !insertmacro LogDetail "Copiing files."

 File /r "${InputDirectory64}\MarlinModule${PRODUCT_EXT}.dll"
 File /r "${InputDirectory64}\MarlinModule${PRODUCT_EXT}.pdb"
 File /r "${ExtraDirectory}\dbghelp64.dll"

 ; Registering the module
 !insertmacro LogDetail "Registering the MarlinModule with IIS"
 ExecWait '"$INSTDIR\appcmd.exe" install module /name:MarlinModule${PRODUCT_EXT} /image:"$INSTDIR\MarlinModule${PRODUCT_EXT}.dll"' $0
 !insertmacro LogDetail "Registering MarlinModule${PRODUCT_EXT} with appcmd returned: $0"

SectionEnd

;--------------------------------------------------------------------------------------------------------
Section -Post
 WriteUninstaller "$INSTDIR\uninstall ${PRODUCT_NAME} ${PRODUCT_VERSION}.exe"
 
 WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName"     "$(^Name)"
 WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninstall ${PRODUCT_NAME} ${PRODUCT_VERSION}.exe"
 WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon"     "$INSTDIR\uninstall ${PRODUCT_NAME} ${PRODUCT_VERSION}.exe"
 WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion"  "${PRODUCT_VERSION}"
 WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout"    "${PRODUCT_WEB_SITE}"
 WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher"       "${PRODUCT_PUBLISHER}"
 WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "BuildNumber"     "${PRODUCT_VERSION}"

 ; This was the last section
 ; We can now close the logfile
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
 !insertmacro LogDetail "De-installation and de-registration of ${PRODUCT_NAME} version: ${PRODUCT_VERSION}"

 ; UN-Registering the module
 !insertmacro LogDetail "Deleting the MarlinModule${PRODUCT_EXT} from IIS"
 ExecWait '"$SYSDIR\inetsrv\appcmd.exe" delete module MarlinModule${PRODUCT_EXT}' $0
 !insertmacro LogDetail "Deleting MarlinModule with appcmd returned: $0"

 !insertmacro LogDetail "Removing files from : $SYSDIR\inetsrv"
 Delete /REBOOTOK "$SYSDIR\inetsrv\MarlinModule${PRODUCT_EXT}.dll"
 Delete /REBOOTOK "$SYSDIR\inetsrv\MarlinModule${PRODUCT_EXT}.pdb"
 
  ;De-Registration of the product.
 DetailPrint "De-register of ${PRODUCT_NAME}"
 Delete "$INSTDIR\Uninstall ${PRODUCT_NAME} $currentVersion.exe"
 DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
 
 !insertmacro LogDetail "End of de-registration of the ${PRODUCT_NAME}"
 !insertmacro LogDetail "End of the uninstaller ${PRODUCT_NAME} ${PRODUCT_VERSION}."
 Call Un.CloseLogfile
SectionEnd
;--------------------------------------------------------------------------------------------------------
; End of the script

