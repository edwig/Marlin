#pragma once
#include "Version.h"


// Selecting the right library to link with automatically
// So we do not need to worry about which library to use in the linker settings
#if defined _M_IX86
#define MARLIN_PLATFORM "x86"
#else
#define MARLIN_PLATFORM "x64"
#endif

#if defined _DEBUG
#define MARLIN_CONFIGURATION "D"
#else
#define MARLIN_CONFIGURATION "R"
#endif 

#ifndef MARLIN_NOAUTOLINK
#pragma comment(lib,"Marlin_"                        MARLIN_PLATFORM MARLIN_CONFIGURATION ".lib")
#pragma message("Automatically linking with Marlin_" MARLIN_PLATFORM MARLIN_CONFIGURATION ".lib")
#endif 
