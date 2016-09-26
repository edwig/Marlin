@echo off
@echo Cleaning Marlin IIS directories
@echo .

del marlin_3.rar
del /q /s /f *.sdf
del /q /s /f *.VC.db
rmdir /q /s Marlin\Debug
rmdir /q /s Marlin\Release
rmdir /q /s Marlin\Win32
rmdir /q /s Marlin\x64
rmdir /q /s MarlinClient\MarlinClient\ipch
rmdir /q /s MarlinClient\MarlinClient\Debug
rmdir /q /s MarlinClient\MarlinClient\Release
rmdir /q /s MarlinClient\MarlinClient\x64
rmdir /q /s MarlinClient\MarlinIISClient\ipch
rmdir /q /s MarlinClient\MarlinIISClient\Debug
rmdir /q /s MarlinClient\MarlinIISClient\Release
rmdir /q /s MarlinClient\MarlinIISClient\x64
rmdir /q /s MarlinServer\MarlinServer\ipch
rmdir /q /s MarlinServer\MarlinServer\Debug
rmdir /q /s MarlinServer\MarlinServer\Release
rmdir /q /s MarlinServer\MarlinServer\x64
rmdir /q /s MarlinServer\MarlinIISModule\ipch
rmdir /q /s MarlinServer\MarlinIISModule\Debug
rmdir /q /s MarlinServer\MarlinIISModule\Release
rmdir /q /s MarlinServer\MarlinIISModule\x64
rmdir /q /s HTTPManager\Debug
rmdir /q /s HTTPManager\Release
rmdir /q /s HTTPManager\ipch
rmdir /q /s HTTPManager\x64
rmdir /q /s binDebug_Win32
rmdir /q /s binDebug_x64
rmdir /q /s binRelease_Win32
rmdir /q /s binRelease_x64

echo .
echo Ready cleaning
echo .
echo Creating a rar archive for the Marlin server
echo .
"C:\Program Files\Winrar\rar.exe" a Marlin_4.rar Certificates ExtraParts Marlin MarlinClient MarlinIISClient MarlinIISModule MarlinServer ServerTestset TestsetClient HTTPManager Documentation *.cmd
echo .
echo Ready making RAR archive
echo .
robocopy . C:\Users\%USERNAME%\OneDrive\Documents Marlin_4.rar /xo
echo .
echo Ready copying the archive to the OneDrive cloud.
pause
