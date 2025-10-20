/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: EventlogRegistration.h
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

// Standard root folder for the event log folder we create
// Defaulting to "Applications"
extern LPCTSTR g_eventLogCategory;

int    RegisterMessagesDllForService(const XString& p_serviceName,const XString& p_messageDLL,XString& p_error);
bool UnRegisterMessagesDllForService(const XString& p_serviceName,XString& p_error);
bool IsMessageDLLRegistered(const XString& p_serviceName);