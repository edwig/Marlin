@echo off
@echo Cleaning Marlin IIS directories
@echo .

call sourceclean.cmd

echo .
echo Ready cleaning
echo .
echo Creating a rar archive for the Marlin server
echo .
"C:\Program Files\Winrar\rar.exe" a Marlin_4.rar Certificates ExtraParts Marlin MarlinClient MarlinServer HTTPManager Documentation *.cmd README .gitignore LICENSE touch.exe
echo .
echo Ready making RAR archive
echo .
robocopy . C:\Users\%USERNAME%\OneDrive\Documents Marlin_4.rar /xo
echo .
echo Ready copying the archive to the OneDrive cloud.
pause
