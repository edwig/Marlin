#pragma once

// Selecting the right library to link with automatically
// So we do not need to worry about which library to use in the linker settings
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
