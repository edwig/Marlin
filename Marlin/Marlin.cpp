/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Marlin.cpp
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
// Copyright (c) 2014-2022 ir. W.E. Huisman
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
#include "stdafx.h"
#include "Marlin.h"

// Not much here: just the names of a bunch of things

// Global constants for an application
// Actual values are set in LoadConstants() in:
// StandAlone : ServerMain.cpp
// IIS Server : MarlinServer derived class
const char* APPLICATION_NAME     = nullptr;  // Name of the application EXE file!!
const char* PRODUCT_NAME         = nullptr;  // Short name of the product (one word only). Also used in the WMI event log.
const char* PRODUCT_DISPLAY_NAME = nullptr;  // "Service for PRODUCT_NAME: <description of the service>"
const char* PRODUCT_COPYRIGHT    = nullptr;  // Copyright line of the product (c) <year> etc.
const char* PRODUCT_VERSION      = nullptr;  // Short version string (e.g.: "3.2.0") Release.major.minor ONLY!
const char* PRODUCT_MESSAGES_DLL = nullptr;  // Filename of the WMI Messages dll.
const char* PRODUCT_SITE         = nullptr;  // Standard base URL absolute path e.g. "/MarlinServer/"
const char* PRODUCT_ADMIN_EMAIL  = nullptr;  // Default administrator to be notified in case of a problem
