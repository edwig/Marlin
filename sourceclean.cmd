@echo off
@echo Cleaning Marlin IIS directories
@echo .

attrib -h MarlinServer\.vs
attrib -h MarlinClient\.vs
attrib -h HTTPManager\.vs

del /q /s /f *.sdf
del /q /s /f *.VC.db
rmdir /q /s .vs
rmdir /q /s lib
rmdir /q /s Marlin\Debug
rmdir /q /s Marlin\DebugUnicode
rmdir /q /s Marlin\Release
rmdir /q /s Marlin\ReleaseUnicode
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
rmdir /q /s MarlinModule\.vs
rmdir /q /s MarlinModule\x64
rmdir /q /s MarlinModule\Debug
rmdir /q /s MarlinModule\DebugUnicode
rmdir /q /s MarlinModule\Release
rmdir /q /s MarlinModule\ReleaseUnicode
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
rmdir /q /s MarlinServer\IIS
rmdir /q /s MarlinServer\.vs
rmdir /q /s MarlinServer\x64
rmdir /q /s MarlinServer\Debug
rmdir /q /s MarlinServer\DebugUnicode
rmdir /q /s MarlinServer\Release
rmdir /q /s MarlinServer\ReleaseUnicode
rmdir /q /s MarlinServer\Win32
rmdir /q /s HTTPManager\Debug
rmdir /q /s HTTPManager\DebugUnicode
rmdir /q /s HTTPManager\Release
rmdir /q /s HTTPManager\ReleaseUnicode
rmdir /q /s HTTPManager\ipch
rmdir /q /s HTTPManager\x64
rmdir /q /s HTTPManager\.vs
rmdir /q /s binDebug_Win32
rmdir /q /s binDebugUnicode_Win32
rmdir /q /s binDebug_x64
rmdir /q /s binDebugUnicode_x64
rmdir /q /s binRelease_Win32
rmdir /q /s binReleaseUnicode_Win32
rmdir /q /s binRelease_x64
rmdir /q /s binReleaseUnicode_x64
rmdir /q /s HTTPSYS\x64
rmdir /q /s HTTPSYS\Debug
rmdir /q /s HTTPSYS\DebugUnicode
rmdir /q /s HTTPSYS\Release
rmdir /q /s HTTPSYS\ReleaseUnicode
rmdir /q /s ServerApplet\Debug
rmdir /q /s ServerApplet\DebugUnicode
rmdir /q /s ServerApplet\Release
rmdir /q /s ServerApplet\ReleaseUnicode
rmdir /q /s ServerApplet\x64
rmdir /q /s BaseLibrary\Win32
rmdir /q /s BaseLibrary\x64
rmdir /q /s x64
rmdir /q /s Debug
rmdir /q /s Release
rmdir /q /s Marlin-cppcheck-build-dir

echo .
echo Ready cleaning
