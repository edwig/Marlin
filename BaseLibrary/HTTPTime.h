/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPTime.cpp
//
// BaseLibrary: Indispensable general objects and functions
// 
// Created: 2014-2025 ir. W.E. Huisman
// MIT License
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
#include <winhttp.h>

// Replacements for:
// BOOL WINAPI WinHttpTimeFromSystemTime(_In_ const SYSTEMTIME *pst,_Out_ LPWSTR pwszTime);
// BOOL WINAPI WinHttpTimeToSystemTime  (_In_ LPCWSTR pwszTime,_Out_ SYSTEMTIME *pst);

// As the MSDN documentation states: these functions should conform to RFC 2616 section 3.3
// BUT THEY DONT!!!!!!!
// Namely the MS-Exchange 2010 server sends dates that do not conform strictly to the RFC.
// So in order to be able to interpret mixed definitions, we created a much looser implementation

bool    HTTPTimeFromSystemTime(const SYSTEMTIME* p_systemTime,XString& p_string);
bool    HTTPTimeToSystemTime  (const XString& p_string,SYSTEMTIME* p_systemtime);
XString HTTPGetSystemTime();
void    AddSecondsToSystemTime(SYSTEMTIME* p_timeIn,SYSTEMTIME* p_timeOut,double p_seconds);
