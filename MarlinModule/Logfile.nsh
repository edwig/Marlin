;-------------------------------------------------------
;
; File:    Logfile.nsh
; Author:  Edwig Huisman
;
; Copyright (c) 2019 ir. W.E. Huisman
;
;-------------------------------------------------------
var logfile
var log

Function OpenLogfile
  strcpy $logfile $R0
  ClearErrors
  FileOpen $log $logfile "w"
  IfErrors +1 EndOpenLogfile
  abort "Cannot open the logfile $logfile. Check your rights on this directory!"
  EndOpenLogfile:
FunctionEnd

Function CloseLogfile
  FileClose $log
FunctionEnd

Function Un.OpenLogfile
  strcpy $logfile $R0
  ClearErrors
  FileOpen $log $logfile "w"
  IfErrors +1 EndOpenLogfile
  abort "Cannot open the logfile $logfile. Check your rights on this directory!"
  EndOpenLogfile:
FunctionEnd

Function Un.CloseLogfile
  FileCLose $log
FunctionEnd

!macro Log logline
  FileWrite $log "${logline}$\r$\n"
!macroend

!macro LogLine
 !insertmacro Log "-----------------------------------------------------------------------"
!macroend

