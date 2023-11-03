/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SeverMain.h
//
// Copyright (c) 2014-2022 ir. W.E. Huisman
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

// Standard starting timing of services
#define SVC_DEFAULT_SERVICE_START_PENDING    3000  // 3 seconds
#define SVC_MINIMUM_WAIT_HINT                1000  // 1 second minimum to start
#define SVC_MAXIMUM_WAIT_HINT               10000  // 10 seconds at maximum to start
#define SVC_MAXIMUM_STOP_TIME               30000  // Service must be able to stop within 30 seconds

// Prototypes of static server functions for the MS-Windows service

void WINAPI SvcMain(DWORD,LPTSTR *); 
void WINAPI SvcCtrlHandler(DWORD); 

// Prototypes of all other functions
void        ReadConfig();
int         SvcInstall(LPCTSTR username,LPCTSTR password);
int         SvcDelete();
void        SvcInit(DWORD,LPTSTR *); 
void        ReportSvcStatus(DWORD,DWORD,DWORD);
int         InstallMessageDLL();
int         SvcStart();
int         SvcStop();
void        DeleteEventLogRegistration();
BOOL        StopDependentServices();
void        PrintCopyright();
void        PrintHelp();
void        CheckPlatform();
int         QueryService();

// Prototypes for the stand-alone server
int         StandAloneStart();
int         StandAloneStop();
BOOL        SvcInitStandAlone();
int         GetMarlinServiceStatusStandAlone(int& p_status);
void        ReportSvcStatusStandAlone(int p_status);
int         QueryServiceStandAlone();

// Prototypes for the IIS server
int         StartIISApp();
int         StopIISApp();
int         QueryIISApp();
