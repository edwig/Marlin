/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HostedWebCore.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2021 ir. W.E. Huisman
// All rights reserved
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
#include <hwebcore.h>

// Callbacks to the application server
// The application can implement these to
// 1) Show server status info in the HWC console
// 2) Set metadata for IIS from the application
typedef void(*PFN_SERVERSTATUS)(void);
typedef void(*PFN_SETMETADATA)(void);

// Names of the application config files
// You can set these before starting, or they can be
// taken from the command line of the application
extern CString g_applicationhost;      // ApplicationHost.config file to use
extern CString g_webconfig;            // Web.config file to use

// Shutdown mode (0=gracefull, 1=immediate forced)
extern DWORD g_hwcShutdownMode;

// The main entry for a Hosted Web Core application
int HWC_main(int argc,char* argv[]);