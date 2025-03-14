/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServiceReporting.h
//
// BaseLibrary: Indispensable general objects and functions
// 
// Copyright (c) 2014-2025 ir. W.E. Huisman
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

// Product_display_name cannot be longer than this
#define SERVICE_NAME_LENGTH   50

#define SVC_ERROR   ((DWORD)0xC0020001L)
#define SVC_INFO    ((DWORD)0x40020002L)
#define SVC_SUCCESS ((DWORD)0x00020003L)
// BEWARE: To use, you must define
// MessageId=0x1 as SymbolicName=SVC_ERROR
// MessageId=0x2 as SymbolicName=SVC_INFO
// MessageId=0x3 as SymbolicName=SVC_SUCCESS
// in the *.MC file of your server application

// Products service name is in this global variable
extern TCHAR g_svcname[];

void  SvcReportSuccessEvent(LPCTSTR p_message);
void  SvcReportInfoEvent (bool p_doFormat,LPCTSTR p_message,...);
void  SvcReportErrorEvent(int p_module,bool p_doFormat,LPCTSTR p_function,LPCTSTR p_message, ...);
