#pragma once

#define VERSION_HTTPAPI  "2.0"      // What we implement here
#define VERSION_HTTPSYS  "1.0.4"    // Version number of our own driver

// Selecting the right library to link with automatically
// So we do not need to worry about which library to use in the linker settings

// Currently not implemented:
// We want to be able to choose at link time between the 'real' kernel httpapi.dll for http.sys
// and our own user-space implementation "httpsys.dll"

// #if defined _M_IX86
// #define HTTPSYS_PLATFORM "x86"
// #else
// #define HTTPSYS_PLATFORM "x64"
// #endif
// 
// #if defined _DEBUG
// #define HTTPSYS_CONFIGURATION "D"
// #else
// #define HTTPSYS_CONFIGURATION "R"
// #endif 
// 
// #ifndef HTTPSYS_NOAUTOLINK
// #pragma comment(lib,"HTTPSYS_"                       HTTPSYS_PLATFORM HTTPSYS_CONFIGURATION ".lib")
// #pragma message("Automatically linking with Marlin_" HTTPSYS_PLATFORM HTTPSYS_CONFIGURATION ".lib")
// #endif 
// 
