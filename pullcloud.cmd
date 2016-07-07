@echo off
@echo Cleaning the Marlin IIS directories
@echo .
pause

del /q /s /f *.sdf
del /q /s /f *.VC.db
rmdir /q /s Certificates
rmdir /q /s Documentation
rmdir /q /s ExtraParts
rmdir /q /s HTTPManager
rmdir /q /s Marlin
rmdir /q /s MarlinClient
rmdir /q /s MarlinIISClient
rmdir /q /s MarlinIISModule
rmdir /q /s MarlinServer
rmdir /q /s ServerTestset
rmdir /q /s TestsetClient
echo .
echo Ready cleaning
echo .
echo Getting the file from the cloud
robocopy C:\Users\%USERNAME%\OneDrive\Documents . Marlin_4.rar /xo
echo .
echo Ready copying the RAR archive
echo .
"C:\Program Files\Winrar\rar.exe" x -o+ marlin_4.rar
echo .
echo Ready getting the archive from the cloud
echo .
pause
