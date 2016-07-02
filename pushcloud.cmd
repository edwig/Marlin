@echo off
@echo Cleaning Marlin directories
@echo .

rem Pruning the GIT repository before posting to the cloud
git prune -v --expire now

del marlin_3.rar
del /q /s /f *.sdf
del /q /s /f *.VC.db
rmdir /q /s Marlin\Debug
rmdir /q /s Marlin\Release
rmdir /q /s Marlin\x64
rmdir /q /s MarlinClient\ipch
rmdir /q /s MarlinClient\Debug
rmdir /q /s MarlinClient\Release
rmdir /q /s MarlinClient\x64
rmdir /q /s MarlinServer\ipch
rmdir /q /s MarlinServer\Debug
rmdir /q /s MarlinServer\Release
rmdir /q /s MarlinServer\x64
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
"C:\Program Files\Winrar\rar.exe" a Marlin_3.rar .git MarlinClient MarlinServer Marlin HTTPManager Certificates Documentation ExtraParts *.cmd
echo .
echo Ready making RAR archive
echo .
robocopy . C:\Users\%USERNAME%\OneDrive\Documents Marlin_3.rar /xo
echo .
echo Ready copying the archive to the OneDrive cloud.
pause
