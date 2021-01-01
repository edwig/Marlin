/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Marlin.h
// Marlin: Internet server/client C++ Framework
// 
//  ______    _          _         _    _       _                           
// |  ____|  | |        (_)       | |  | |     (_)                          
// | |__   __| |_      ___  __ _  | |__| |_   _ _ ___ _ __ ___   __ _ _ __  
// |  __| / _` \ \ /\ / / |/ _` | |  __  | | | | / __| '_ ` _ \ / _` | '_ \ 
// | |___| (_| |\ V  V /| | (_| | | |  | | |_| | \__ \ | | | | | (_| | | | |
// |______\__,_| \_/\_/ |_|\__, | |_|  |_|\__,_|_|___/_| |_| |_|\__,_|_| |_|
//                          __/ |                                           
//                         |___/                                            
//
// Copyright (c) 2014-2021 ir. W.E. Huisman
// All rights reserved
//
// MIT License:
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#pragma once
#include "Version.h"

// Constants in ServerMain and ServerApp
// Define in the derived class from "MarlinServer" in the "LoadConstants()" function
//
extern const char* APPLICATION_NAME;       // Name of the application EXE file!!
extern const char* PRODUCT_NAME;           // Short name of the product (one word only)
extern const char* PRODUCT_DISPLAY_NAME;   // "Service for PRODUCT_NAME: <description of the service>"
extern const char* PRODUCT_COPYRIGHT;      // Copyright line of the product (c) <year> etc.
extern const char* PRODUCT_VERSION;        // Short version string (e.g.: "3.2.0") Release.major.minor ONLY!
extern const char* PRODUCT_MESSAGES_DLL;   // Filename of the WMI Messages dll.
extern const char* PRODUCT_SITE;           // Standard base URL absolute path e.g. "/MarlinServer/"

// Load product and application constants
// in the constants above
void LoadConstants(char* p_app_name);

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
