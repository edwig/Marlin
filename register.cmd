@echo off
@echo Installing Marlin to IIS
@C:\Windows\System32\inetsrv\appcmd.exe install module /name:MarlinIISModule /image:c:\develop\marlin\binDebug_x64\MarlinIISModule.dll
rem  @C:\Windows\System32\inetsrv\appcmd.exe add     module /name:MarlinIISModule /app.name:"MarlinTest"