@echo off
@echo Cleaning Marlin IIS directories
@echo .

iisreset /stop

attrib -h MarlinServer\.vs
attrib -h MarlinClient\.vs
attrib -h HTTPManager\.vs

del marlin_4.rar
del /q /s /f *.sdf
del /q /s /f *.VC.db
rmdir /q /s Marlin\Debug
rmdir /q /s Marlin\Release
rmdir /q /s Marlin\x64
rmdir /q /s Marlin\Win32
rmdir /q /s MarlinClient\ipch
rmdir /q /s MarlinClient\MarlinClient\Debug
rmdir /q /s MarlinClient\MarlinClient\Release
rmdir /q /s MarlinClient\MarlinClient\x64
rmdir /q /s MarlinClient\MarlinIISClient\Debug
rmdir /q /s MarlinClient\MarlinIISClient\Release
rmdir /q /s MarlinClient\MarlinIISClient\x64
rmdir /q /s MarlinClient\.vs
rmdir /q /s MarlinClient\x64
rmdir /q /s MarlinClient\Debug
rmdir /q /s MarlinClient\Release
rmdir /q /s MarlinServer\ipch
rmdir /q /s MarlinServer\MarlinServer\Debug
rmdir /q /s MarlinServer\MarlinServer\Release
rmdir /q /s MarlinServer\MarlinServer\x64
rmdir /q /s MarlinServer\MarlinIISModule\Debug
rmdir /q /s MarlinServer\MarlinIISModule\Release
rmdir /q /s MarlinServer\MarlinIISModule\x64
rmdir /q /s MarlinServer\HostedWebCore\Debug
rmdir /q /s MarlinServer\HostedWebCore\Release
rmdir /q /s MarlinServer\HostedWebCore\x64
rmdir /q /s MarlinServer\MarlinServerSync\Debug
rmdir /q /s MarlinServer\MarlinServerSync\Release
rmdir /q /s MarlinServer\MarlinServerSync\x64
rmdir /q /s MarlinServer\.vs
rmdir /q /s MarlinServer\x64
rmdir /q /s MarlinServer\Debug
rmdir /q /s MarlinServer\Release
rmdir /q /s HTTPManager\Debug
rmdir /q /s HTTPManager\Release
rmdir /q /s HTTPManager\ipch
rmdir /q /s HTTPManager\x64
rmdir /q /s HTTPManager\.vs
rmdir /q /s binDebug_Win32
rmdir /q /s binDebug_x64
rmdir /q /s binRelease_Win32
rmdir /q /s binRelease_x64
rmdir /q /s HTTPSYS\x64
rmdir /q /s HTTPSYS\Debug
rmdir /q /s HTTPSYS\Release
rmdir /q /s x64
rmdir /q /s Debug
rmdir /q /s Release

echo .
echo Ready cleaning
