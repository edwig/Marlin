/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ErrorReport.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2024 ir. W.E. Huisman
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
#include "pch.h"
#include "ErrorReport.h"
#include "StackTrace.h"
#include "ProcInfo.h"
#include "HTTPMessage.h"
#include "AutoCritical.h"
#include "ServiceReporting.h"
#include <WinFile.h>
#include <io.h>
#include <iostream>
#include <fstream>
#include <exception>
#include <signal.h>
#include <memory>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

// Error report running
__declspec(thread) bool g_exception       = false;
__declspec(thread) bool g_reportException = true;

// The one-and-only ErrorReport instance
static ErrorReport* g_instance = NULL;

// Names for exceptions and signals MVC++ specific
//
#define EXCEPTION_CPP       0xE06D7363
#define EXCEPTION_COMPLUS   0xE0524F54
#define CPP_EXCEPTION_MAGIC 0x19930520

#ifndef STATUS_SXS_EARLY_DEACTIVATION
#define STATUS_SXS_EARLY_DEACTIVATION   (0xC015000FL)
#endif
#ifndef STATUS_SXS_INVALID_DEACTIVATION
#define STATUS_SXS_INVALID_DEACTIVATION (0xC0150010L)
#endif

static struct
{
  DWORD code;
  const TCHAR *m_name;
} 
Exceptions[] =
{
  { EXCEPTION_ACCESS_VIOLATION,         _T("Access violation")                          },
  { EXCEPTION_ARRAY_BOUNDS_EXCEEDED,    _T("Array bounds exceeded")                     },
  { EXCEPTION_BREAKPOINT,               _T("Breakpoint")                                },
  { EXCEPTION_DATATYPE_MISALIGNMENT,    _T("Datatype misalignment")                     },
  { EXCEPTION_FLT_DENORMAL_OPERAND,     _T("Denormalized floating-point operand")       },
  { EXCEPTION_FLT_DIVIDE_BY_ZERO,       _T("Floating-point divide by zero")             },
  { EXCEPTION_FLT_INEXACT_RESULT,       _T("Inexact floating-point result")             },
  { EXCEPTION_FLT_INVALID_OPERATION,    _T("Invalid floating-point operation")          },
  { EXCEPTION_FLT_OVERFLOW,             _T("Floating-point overflow")                   },
  { EXCEPTION_FLT_STACK_CHECK,          _T("Floating-point stack check")                },
  { EXCEPTION_FLT_UNDERFLOW,            _T("Floating-point underflow")                  },
  { EXCEPTION_GUARD_PAGE,               _T("Guard page accessed")                       },
  { EXCEPTION_ILLEGAL_INSTRUCTION,      _T("Illegal instruction")                       },
  { EXCEPTION_IN_PAGE_ERROR,            _T("Page-in error")                             },
  { EXCEPTION_INT_DIVIDE_BY_ZERO,       _T("Integer divide by zero")                    },
  { EXCEPTION_INT_OVERFLOW,             _T("Integer overflow")                          },
  { EXCEPTION_INVALID_DISPOSITION,      _T("Invalid disposition")                       },
  { EXCEPTION_INVALID_HANDLE,           _T("Invalid handle")                            },
  { EXCEPTION_NONCONTINUABLE_EXCEPTION, _T("Non-continuable exception")                 },
  { EXCEPTION_PRIV_INSTRUCTION,         _T("Privileged instruction")                    },
  { EXCEPTION_SINGLE_STEP,              _T("Single step")                               },
  { EXCEPTION_STACK_OVERFLOW,           _T("Stack overflow")                            },
  { EXCEPTION_CPP,                      _T("Unhandled C++ exception")                   },
  { EXCEPTION_CPP,                      _T("Unhandled COM+ exception")                  },
  { STATUS_SXS_EARLY_DEACTIVATION,      _T("Activation context not topmost")            },
  { STATUS_SXS_INVALID_DEACTIVATION,    _T("Activation context not for current thread") },
};

static struct 
{
  int   code;
  const TCHAR *m_name;
} 
Signals[] =
{
  { SIGABRT, _T("Abnormal termination")         },
  { SIGFPE,  _T("Floating-point error")         },
  { SIGILL,  _T("Illegal instruction")          },
  { SIGINT,  _T("CTRL+C interrupt")             },
  { SIGSEGV, _T("Segmentation violation")       },
  { SIGTERM, _T("Termination request received") },
};

//////////////////////////////////////////////////////////////////////////
//
// Convert time to string
//
static XString
SystemTimeToString(SYSTEMTIME const& st, bool p_relative = false)
{
  TCHAR buffer[25];
  if (p_relative)
  {
    _stprintf_s(buffer,25,_T("%02d:%02d:%02d")
               ,st.wHour, st.wMinute, st.wSecond);
  }
  else
  {
    _stprintf_s(buffer,25
               ,_T("%02d-%02d-%04d %02d:%02d:%02d")
               ,st.wDay,st.wMonth,st.wYear
               ,st.wHour,st.wMinute,st.wSecond);
  }
  return buffer;
}

XString
FileTimeToString(FILETIME const& ft, bool p_relative = false)
{
  SYSTEMTIME st;

  if (p_relative)
  {
    FileTimeToSystemTime(&ft, &st);
  }
  else
  {
    FILETIME lft;
    FileTimeToLocalFileTime(&ft, &lft);
    FileTimeToSystemTime(&lft, &st);
  }
  return SystemTimeToString(st, p_relative);
}

static XString
FileTimeToMarker(SYSTEMTIME const& st)
{
  TCHAR buffer[25];
  _stprintf_s(buffer,25
             ,_T("%04d_%02d_%02d_%02d%02d%02d")
             ,st.wYear,st.wMonth, st.wDay
             ,st.wHour,st.wMinute,st.wSecond);

  return buffer;
}

// Write a string to a file and return the filename
//
XString
ErrorReportWriteToFile(const XString& p_filename
                      ,const XString& p_report
                      ,const XString& p_webroot
                      ,const XString& p_url)
{
  SYSTEMTIME nu;
  GetLocalTime(&nu);
  XString current = FileTimeToMarker(nu);
  XString pathname(p_webroot);

  // Creating product dir
  XString productDir(p_url);
  productDir.Trim(_T("/"));
  productDir.Replace(_T("/"),_T("\\"));
  if(productDir.Right(1) != '\\')
  {
    productDir += _T("\\");
  }

  // Checking webroot
  if(pathname.Right(1) != '\\')
  {
    pathname += _T("\\");
  }

  // Getting the paths to try
  XString path1 = pathname + productDir;
  XString path2 = pathname;

  // Getting a file to write to
  if(p_filename.IsEmpty())
  {
    path1 += _T("CrashReport_");
    path2 += _T("CrashReport_");
    path1 += g_instance->GetProduct();
    path2 += g_instance->GetProduct();
    path1 += current + _T("_XXXXXX");
    path2 += current + _T("_XXXXXX");
    TCHAR buffer1[MAX_PATH + 1] = _T("");
    TCHAR buffer2[MAX_PATH + 1] = _T("");
    _tcsncpy_s(buffer1,path1,MAX_PATH);
    _tcsncpy_s(buffer2,path2,MAX_PATH);
    if(_tmktemp_s(buffer1) != 0)
    {
      goto failed;
    }
    if(_tmktemp_s(buffer2) != 0)
    {
      goto failed;
    }
    path1 = XString(buffer1) + _T(".txt");
    path2 = XString(buffer2) + _T(".txt");
  }
  else
  {
    path1 = p_filename;
    path2.Empty();
  }
  // Opening the temporary file for writing
  // Try the product directory first,
  // if not: write in the webroot: so it stands out
  while(!path1.IsEmpty())
  {
    WinFile file(path1);
    if(file.Open(winfile_write,FAttributes::attrib_none,Encoding::UTF8))
    {
      file.Write(p_report);
      if(!file.Close())
      {
        DeleteFile(path1);
        goto failed;
      }
      return path1;
    }
    path1 = path2;
    path2.Empty();
  }

failed:
  // Failed to write it to a file.
  // As a last restort, try to write it the the MS-Windows Event log
  SvcReportErrorEvent(0,false,_T(__FUNCTION__),p_report.GetString());

  return XString();
}

// Send an error report to the developers
//
ErrorReport::ErrorReport()
{
  if(g_instance != nullptr)
  {
    SvcReportInfoEvent(0,false,_T(__FUNCTION__),_T("Registered more than one automatic 'ErrorReport'"));
  }
  g_instance = this;
  InitializeCriticalSection(&m_lock);
}

ErrorReport::~ErrorReport()
{
  if(g_instance == this)
  {
    g_instance = nullptr;
  }
  else if(g_instance)
  {
    SvcReportErrorEvent(0,false,_T(__FUNCTION__), _T("UnRegistering 'ErrorReport' in the wrong order"));
  }
  DeleteCriticalSection(&m_lock);
}

void
ErrorReport::RegisterProduct(XString p_appName,XString p_version,XString p_build,XString p_date)
{
  if(g_instance && g_instance->GetProduct().IsEmpty())
  {
    g_instance->m_product = p_appName;
    g_instance->m_version = p_version;
    g_instance->m_build   = p_build;
    g_instance->m_date    = p_date;
  }
}

/*static*/ void
ErrorReport::Report(const XString& p_subject, unsigned int p_skip /* = 0 */, XString p_directory, XString p_url)
{
  if(g_instance == nullptr)
  {
    SvcReportErrorEvent(0,false,_T(__FUNCTION__), _T("Missing registered ErrorReport"));
    abort();
  }

  // One thread at the time
  AutoCritSec lock(&(g_instance->m_lock));

  // Send the report
  StackTrace trace(p_skip + 1);
  g_instance->DoReport(p_subject,trace,p_directory,p_url);
}

/*static*/ void
ErrorReport::Report(const XString& p_subject,const StackTrace& p_trace,XString p_directory,XString p_url)
{
  if(g_instance == nullptr)
  {
    SvcReportErrorEvent(0,false,_T(__FUNCTION__), _T("Missing registered ErrorReport"));
    abort();
  }

  // One thread at the time
  AutoCritSec lock(&(g_instance->m_lock));

  // Send the report
  g_instance->DoReport(p_subject,p_trace,p_directory,p_url);
}

/*static*/ bool
ErrorReport::Report(DWORD p_error,struct _EXCEPTION_POINTERS* p_exc)
{
  XString tempdir;
  XString name;
  if(tempdir.GetEnvironmentVariable(_T("TMP")))
  {
    return Report(p_error,p_exc,tempdir,name);
  }
  return false;
}

/*static*/ bool
ErrorReport::Report(DWORD p_errorCode
                   ,struct _EXCEPTION_POINTERS* p_exception
                   ,XString& p_directory
                   ,XString& p_url)
{
  if (!g_reportException)
  {
    g_reportException = true;
    return false;
  }
  if(g_instance == nullptr)
  {
    SvcReportErrorEvent(0,false,_T(__FUNCTION__),_T("Missing registered ErrorReport"));
    abort();
  }

  // One thread at the time
  AutoCritSec lock(&(g_instance->m_lock));

  // Getting the subject
  XString subject;
  for (unsigned int i = 0;
       i < sizeof(Exceptions) / sizeof(Exceptions[0]);
       i++)
  {
    if (Exceptions[i].code == p_errorCode)
    {
      subject = Exceptions[i].m_name;
      break;
    }
  }
  if (subject.IsEmpty())
  {
    subject.Format(_T("Unknown error (0x%08x)"),(unsigned int) p_errorCode);
  }

  // Getting the name of a C++ exception
  // For more information: see the article "Visual C++ Exception-Handling Instrumentation"
  // Dr. Dobb's Journal, 1 december 2002,
  // http://www.ddj.com/windows/184416600.
  //
  // The _s__ThrowInfo is a Compiler internal of the Microsoft VC++ compiler!!
  // It is known by default as an undocumented feature of this compiler
  //
  if (p_errorCode == EXCEPTION_CPP
   && (p_exception->ExceptionRecord->ExceptionInformation[0] == CPP_EXCEPTION_MAGIC))
  {
    const _s__ThrowInfo *throwInfo = reinterpret_cast<const _s__ThrowInfo *>(p_exception->ExceptionRecord->ExceptionInformation[2]);
    
    if (throwInfo && throwInfo->pCatchableTypeArray && throwInfo->pCatchableTypeArray->nCatchableTypes > 0)
    {
      const std::type_info *typeInfo=reinterpret_cast<const std::type_info *>((*(throwInfo->pCatchableTypeArray->arrayOfCatchableTypes)[0]).pType);

      if (typeInfo != NULL)
      {
        subject += XString(_T(" (")) + CA2CT(typeInfo->name()) + _T(")");
      }
    }
  }

  // Getting the memory address of the exception
  if (p_errorCode == EXCEPTION_ACCESS_VIOLATION || p_errorCode == EXCEPTION_IN_PAGE_ERROR)
  {
    switch (p_exception->ExceptionRecord->ExceptionInformation[0])
    {
      case 0:  subject += _T(" reading ");   break;
      case 1:  subject += _T(" writing ");   break;
      case 8:  subject += _T(" executing "); break;
      default: subject += _T(" (unknown) "); break;
    }
    XString tmp;
#ifdef _M_X64
    tmp.Format(_T("0x%I64X"), p_exception->ExceptionRecord->ExceptionInformation[1]);
#else
    tmp.Format(_T("0x%lx"),p_exception->ExceptionRecord->ExceptionInformation[1]);
#endif
    subject += tmp;
  }
  // Getting the stack trace
  StackTrace trace(p_exception->ContextRecord, p_errorCode == EXCEPTION_CPP ? 2 : 0);

  // Sending the report
  g_instance->DoReport(subject,trace,p_directory,p_url);

  // Reset the exception handler
  return false;
}

/*static*/ void
ErrorReport::Report(int p_signal,unsigned int p_skip /* = 0 */,XString p_directory,XString p_url)
{
  if(g_instance == nullptr)
  {
    SvcReportErrorEvent(0,false,_T(__FUNCTION__),_T("Missing registered ErrorReport"));
    abort();
  }

  // One thread at the time
  AutoCritSec lock(&(g_instance->m_lock));

  // Getting the subject
  XString subject;
  for (unsigned int i=0; i < sizeof(Signals) / sizeof(Signals[0]); i++)
  {
    if (Signals[i].code == p_signal)
    {
      subject = Signals[i].m_name;
      break;
    }
  }
  if (subject.IsEmpty())
  {
    subject.Format(_T("Unknown signal (%d)"), p_signal);
  }
  // Getting the stacktrace
  StackTrace trace(p_skip);

  // Sending an error report
  g_instance->DoReport(subject,trace,p_directory,p_url);
}

// Standard crash report as a file on the webroot URL directory
//
XString
ErrorReport::DoReport(const XString&    p_subject
                     ,const StackTrace& p_trace
                     ,const XString&    p_webroot
                     ,const XString&    p_url) const
{
  // Getting process information
  std::unique_ptr<ProcInfo> procInfo(new ProcInfo);
  FILETIME creationTime, exitTime, kernelTime, userTime;
  GetProcessTimes(GetCurrentProcess(),
                  &creationTime,
                  &exitTime,
                  &kernelTime,
                  &userTime);

  // Getting the error address
  const XString &errorAddress = p_trace.FirstAsString();

#ifdef _UNICODE
  XString charsets(_T("Unicode"));
#else
  XString charsets(_T("ANSI/MBCS"));
#endif

  // Format the message
  XString message = _T("SYSTEMUSER: ") + procInfo->m_username + _T(" on ") + procInfo->m_computer + _T("\n")
                    _T("PROGRAM   : ") + m_product +     _T(": version: ") + m_version + _T(": ") + p_subject + _T(" in ") + errorAddress + _T("\n")
                    _T("-------------\n")
                    _T("Version:      ") + m_version + _T("\n")
                    _T("Buildnumber:  ") + m_build   + _T("\n")
                    _T("Builddate:    ") + m_date    + _T("\n")
                    _T("Characterset: ") + charsets  + _T("\n\n")
                    + p_trace.AsString() + _T("\n");

  /// Info about the process
  message += _T("Process\n")
             _T("-------\n");
  message += _T("Commandline:       ") + XString(GetCommandLine()) + _T("\n");
  message += _T("Starttime:         ") + FileTimeToString(creationTime) + _T("\n");
  message += _T("Local time:        ") + procInfo->m_datetime + _T("\n");
  message += _T("Processorusage:    ") + FileTimeToString(userTime,  true) + _T(" (user), ")
                                       + FileTimeToString(kernelTime,true) + _T(" (kernel)\n\n");
  // Info about the system
  message += _T("System\n")
             _T("------\n");
  message += _T("Computername:      ") + procInfo->m_computer      + _T("\n");
  message += _T("Current user:      ") + procInfo->m_username      + _T("\n");
  message += _T("Windows version:   ") + procInfo->m_platform      + _T("\n");
  message += _T("Windows path:      ") + procInfo->m_windows_path  + _T("\n");
  message += _T("Current path:      ") + procInfo->m_current_path  + _T("\n");
  message += _T("Searchpath:        ") + procInfo->m_pathspec      + _T("\n");
  message += _T("Nummeric format:   ") + procInfo->m_system_number + _T(" (system), ") + procInfo->m_user_number + _T(" (user)\n");
  message += _T("Date format:       ") + procInfo->m_system_date   + _T(" (system), ") + procInfo->m_user_date   + _T(" (user)\n");
  message += _T("Time format:       ") + procInfo->m_system_time   + _T(" (system), ") + procInfo->m_user_time   + _T(" (user)\n\n");

  // Info about loaded modules
  message += _T("Modules\n")
             _T("-------");

  for(const auto& module : procInfo->m_modules)
  {
    // Info about this module
    message += _T("\n")                 + module->m_full_path + _T("\n")
               _T("\tAddress:        ") + module->m_load_address      + _T("\n")
               _T("\tOriginal name:  ") + module->m_original_filename + _T("\n")
               _T("\tVersion:        ") + module->m_fileversion       + _T("\n")
               _T("\tDescription:    ") + module->m_file_description  + _T("\n")
               _T("\tProductname:    ") + module->m_product_name      + _T("\n")
               _T("\tProductversion: ") + module->m_product_version   + _T("\n");

  }
  // Writing the crash message
  XString filename;
  return ErrorReportWriteToFile(filename,message,p_webroot,p_url);
}

//////////////////////////////////////////////////////////////////////////
//
// Tinkering with the standard signal handlers
// to accommodate the Structured Exception Handlers (SEH)
//
//////////////////////////////////////////////////////////////////////////

// Signals to catch (stop using old handlers)
//
static int SignalStop[] =
{
  SIGABRT, SIGINT, SIGTERM, // SIGSEGV, SIGILL, SIGBREAK
};

static std::terminate_handler oldHandler = nullptr;

static void
SignalHandler(int signal)
{
#ifdef _DEBUG
  // If we are running the debugger: use that mechanism
  if (IsDebuggerPresent())
  {
    DebugBreak();
  }
#endif // _DEBUG

  XString tempdir;
  if(tempdir.GetEnvironmentVariable(_T("TMP")))
  {
    // Gather stack information in our TMP directory
    ErrorReport::Report(signal,0,tempdir,_T(""));
    return;
  }
  // Gather stack information in the C:\ root directory
  ErrorReport::Report(signal,0,_T(""),_T(""));
}

static void
CallTerminateHandler()
{
  // This function is called if somewhere in the program the 
  // std::terminate is being called. E.g. if a double exception takes place
  // (an exception within an exception)
  SignalHandler(SIGABRT);
}

static void
CallSignalHandler(int signal)
{
  // Call this function if an old-style POSIX signal gets raised
  // Can happen within the abort() function.
  SignalHandler(signal);
}

// To be called before program begins running
void
PrepareProcessForSEH()
{
  // Setting our own terminate-handler, saving the old handler
  oldHandler = std::set_terminate(CallTerminateHandler);

  // Install new signal handlers
  for (unsigned int i = 0; i < sizeof(SignalStop) / sizeof(SignalStop[0]); i++)
  {
    signal(SignalStop[i],CallSignalHandler);
  }
  // Change the behaviour of the abort() function to not show any messages
  // if it is being called. We are doing the messaging.
  _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
}

// To be called after SEH handlers are used
void
ResetProcessAfterSEH()
{
  // Restore the old terminate() handler
  std::set_terminate(oldHandler);

  // Reset all signals to default handlers
  for (unsigned int i = 0; i < sizeof(SignalStop) / sizeof(SignalStop[0]); i++)
  {
    signal(SignalStop[i], SIG_DFL);
  }
}
