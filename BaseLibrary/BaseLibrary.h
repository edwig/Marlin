/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: BaseLibrary.h
// BaseLibary: Collection of Indispensable base classes
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
// Copyright (c) 2014-2024 ir. W.E. Huisman
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

// Minimal requirements from the MS-Windows OS
#include <windows.h>

// This is the compilation/include order of the base-library
#include "XString.h"
#include "StdException.h"
#include "WinFile.h"
#include "bcd.h"

// VERSION NUMBER OF THIS LIBRARY
#define BASELIBRARY_VERSION 1.2.0

// Selecting the right library to link with automatically
// So we do not need to worry about which library to use in the linker settings
// As long as we use the $(SolutionDir)Lib\ as the library location of all our projects

#ifdef UNICODE

#if defined _M_IX86
#define BASELIBRARY_PLATFORM "Ux86"
#else
#define BASELIBRARY_PLATFORM "Ux64"
#endif

#else // UNICODE

#if defined _M_IX86
#define BASELIBRARY_PLATFORM "x86"
#else
#define BASELIBRARY_PLATFORM "x64"
#endif

#endif  // UNICODE

#if defined _DEBUG
#define BASELIBRARY_CONFIGURATION "D"
#else
#define BASELIBRARY_CONFIGURATION "R"
#endif 

#ifndef BASELIBRARY_NOAUTOLINK
#pragma comment(lib,"BaseLibrary_"                        BASELIBRARY_PLATFORM BASELIBRARY_CONFIGURATION ".lib")
#pragma message("Automatically linking with BaseLibrary_" BASELIBRARY_PLATFORM BASELIBRARY_CONFIGURATION ".lib")
#else
#pragma message("Creating library BaseLibrary_" BASELIBRARY_PLATFORM BASELIBRARY_CONFIGURATION ".lib")
#endif 
