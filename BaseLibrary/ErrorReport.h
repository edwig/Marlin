/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ErrorReport.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2024 ir. W.E. Huisman
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

class StackTrace;
struct _EXCEPTION_POINTERS;

extern __declspec(thread) bool g_exception;
extern __declspec(thread) bool g_reportException;

// To be called before program begins running
void PrepareProcessForSEH();
// To be called after SEH handlers are used
void ResetProcessAfterSEH();

class ErrorReport
{
public:
  ErrorReport();
  virtual ~ErrorReport();

  static void RegisterProduct(XString p_appName,XString p_version,XString p_build,XString p_date);

  // Send an error report
  static void Report(const XString& p_subject,unsigned int p_skip,      XString  p_directory,XString  p_url);
  static void Report(const XString& p_subject,const StackTrace& p_trace,XString  p_directory,XString  p_url);
  static void Report(int   p_signal          ,unsigned int p_skip,      XString  p_directory,XString  p_url);
  static bool Report(DWORD p_error,struct _EXCEPTION_POINTERS* p_exc,   XString& p_directory,XString& p_url);
  static bool Report(DWORD p_error,struct _EXCEPTION_POINTERS* p_exc);

  // GETTERS
  XString GetProduct() { return m_product; }

  // Multi threading
  CRITICAL_SECTION m_lock;  // Locking on a thread basis
protected:
  // Go send an error report
  virtual XString DoReport(const XString&    p_subject
                          ,const StackTrace& p_trace
                          ,const XString&    p_webroot
                          ,const XString&    p_url) const;

  XString m_product;    // Product or application name
  XString m_version;    // Product version number (normally publicly seen)
  XString m_build;      // Product build information (build numer, normally hidden)
  XString m_date;       // Product build date
};
