/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ErrorReport.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
#include "StdAfx.h"
#include "ErrorReport.h"
#include "StackTrace.h"
#include "ProcInfo.h"
#include "WebConfig.h"
#include "HTTPMessage.h"
#include "AutoCritical.h"
#include "Version.h"
#include <io.h>
#include <iostream>
#include <fstream>
#include <exception>
#include <signal.h>
#include <assert.h>
#include <memory>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Error report running
__declspec(thread) bool g_exception = false;
__declspec(thread) bool g_reportException = true;

static ErrorReport* s_instance = NULL;

// Names for exceptions and signals
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
  const char *m_name;
} 
Exceptions[] =
{
  { EXCEPTION_ACCESS_VIOLATION,         "Access violation"                          },
  { EXCEPTION_ARRAY_BOUNDS_EXCEEDED,    "Array bounds exceeded"                     },
  { EXCEPTION_BREAKPOINT,               "Breakpoint"                                },
  { EXCEPTION_DATATYPE_MISALIGNMENT,    "Datatype misalignment"                     },
  { EXCEPTION_FLT_DENORMAL_OPERAND,     "Denormal floating-point operand"           },
  { EXCEPTION_FLT_DIVIDE_BY_ZERO,       "Floating-point divide by zero"             },
  { EXCEPTION_FLT_INEXACT_RESULT,       "Inexact floating-point result"             },
  { EXCEPTION_FLT_INVALID_OPERATION,    "Invalid floating-point operation"          },
  { EXCEPTION_FLT_OVERFLOW,             "Floating-point overflow"                   },
  { EXCEPTION_FLT_STACK_CHECK,          "Floating-point stack check"                },
  { EXCEPTION_FLT_UNDERFLOW,            "Floating-point underflow"                  },
  { EXCEPTION_GUARD_PAGE,               "Guard page accessed"                       },
  { EXCEPTION_ILLEGAL_INSTRUCTION,      "Illegal instruction"                       },
  { EXCEPTION_IN_PAGE_ERROR,            "Page-in error"                             },
  { EXCEPTION_INT_DIVIDE_BY_ZERO,       "Integer divide by zero"                    },
  { EXCEPTION_INT_OVERFLOW,             "Integer overflow"                          },
  { EXCEPTION_INVALID_DISPOSITION,      "Invalid disposition"                       },
  { EXCEPTION_INVALID_HANDLE,           "Invalid handle"                            },
  { EXCEPTION_NONCONTINUABLE_EXCEPTION, "Non-continuable exception"                 },
  { EXCEPTION_PRIV_INSTRUCTION,         "Privileged instruction"                    },
  { EXCEPTION_SINGLE_STEP,              "Single step"                               },
  { EXCEPTION_STACK_OVERFLOW,           "Stack overflow"                            },
  { EXCEPTION_CPP,                      "Unhandled C++ exception"                   },
  { EXCEPTION_CPP,                      "Unhandled COM+ exception"                  },
  { STATUS_SXS_EARLY_DEACTIVATION,      "Activation context not topmost"            },
  { STATUS_SXS_INVALID_DEACTIVATION,    "Activation context not for current thread" },
};

static struct 
{
  int   code;
  const char *m_name;
} 
Signals[] =
{
  { SIGABRT, "Abnormal termination"         },
  { SIGFPE,  "Floating-point error"         },
  { SIGILL,  "Illegal instruction"          },
  { SIGINT,  "CTRL+C interrupt"             },
  { SIGSEGV, "Segmentation violation"       },
  { SIGTERM, "Termination request received" },
};

//////////////////////////////////////////////////////////////////////////
//
// Convert time to string
//
static CString
SystemTimeToString(SYSTEMTIME const& st, bool p_relative = false)
{
  char buffer[25];
  if (p_relative)
  {
    sprintf_s(buffer,25,"%02d:%02d:%02d"
             ,st.wHour, st.wMinute, st.wSecond);
  }
  else
  {
    sprintf_s(buffer,25
             ,"%02d-%02d-%04d %02d:%02d:%02d"
             ,st.wDay,st.wMonth,st.wYear
             ,st.wHour,st.wMinute,st.wSecond);
  }
  return buffer;
}

CString
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

static CString
FileTimeToMarker(SYSTEMTIME const& st)
{
  char buffer[25];
  sprintf_s(buffer,25
           ,"%04d_%02d_%02d_%02d%02d%02d"
           ,st.wYear,st.wMonth, st.wDay
           ,st.wHour,st.wMinute,st.wSecond);

  return buffer;
}

// Write a string to a file and return the filename
//
CString
ErrorReportWriteToFile(const CString& p_filename
                      ,const CString& p_report
                      ,const CString& p_webroot
                      ,const CString& p_url)
{
  SYSTEMTIME nu;
  GetLocalTime(&nu);
  CString current = FileTimeToMarker(nu);
  CString pathname(p_webroot);

  // Creating productdir
  CString productDir(p_url);
  productDir.Trim("/");
  productDir.Replace("/","\\");
  if(productDir.Right(1) != '\\')
  {
    productDir += "\\";
  }

  // Checking webroot
  if(pathname.Right(1) != '\\')
  {
    pathname += "\\";
  }

  // Getting the paths to try
  CString path1 = pathname + productDir;
  CString path2 = pathname;

  // Getting a file to write to
  if(p_filename.IsEmpty())
  {
    path1 += "CrashReport_Marlin_Server_";
    path2 += "CrashReport_Marlin_Server_";
    path1 += current + "_XXXXXX";
    path2 += current + "_XXXXXX";
    char buffer1[MAX_PATH + 1] = "";
    char buffer2[MAX_PATH + 1] = "";
    strncpy_s(buffer1,path1,MAX_PATH);
    strncpy_s(buffer2,path2,MAX_PATH);
    if(_mktemp_s(buffer1) != 0)
    {
      return "";
    }
    if(_mktemp_s(buffer2) != 0)
    {
      return "";
    }
    path1 = CString(buffer1) + ".txt";
    path2 = CString(buffer2) + ".txt";
  }
  else
  {
    path1 = p_filename;
    path2.Empty();
  }
  // Opening the tempfile for writing
  // Try the product dir first,
  // if not: write in the webroot: so it stands out
  while(!path1.IsEmpty())
  {
    FILE* fp = NULL;
    fopen_s(&fp,path1,"w");
    if(fp)
    {
      fputs(p_report,fp);
      if(fclose(fp))
      {
        DeleteFile(path1);
        return "";
      }
      return path1;
    }
    path1 = path2;
    path2.Empty();
  }
  return "";
}

// Send an error report to the developers
//
ErrorReport::ErrorReport()
{
  assert(s_instance == NULL);
  s_instance = this;
  InitializeCriticalSection(&m_lock);
}

ErrorReport::~ErrorReport()
{
  assert(s_instance == this);
  s_instance = NULL;
  DeleteCriticalSection(&m_lock);
}

/*static*/ void
ErrorReport::Report(const CString& p_subject, unsigned int p_skip /* = 0 */, CString p_directory, CString p_url)
{
  assert(s_instance != NULL);

  // One thread at the time
  AutoCritSec lock(&(s_instance->m_lock));

  // Send the report
  StackTrace trace(p_skip + 1);
  s_instance->DoReport(p_subject,trace,p_directory,p_url);
}

/*static*/ void
ErrorReport::Report(const CString& p_subject,const StackTrace& p_trace,CString p_directory,CString p_url)
{
  assert(s_instance != NULL);

  // One thread at the time
  AutoCritSec lock(&(s_instance->m_lock));

  // Send the report
  s_instance->DoReport(p_subject, p_trace,p_directory,p_url);
}

/*static*/ bool
ErrorReport::Report(DWORD p_error,struct _EXCEPTION_POINTERS* p_exc)
{
  CString tempdir;
  CString name("Crash_");
  if(tempdir.GetEnvironmentVariable("TMP"))
  {
    return Report(p_error,p_exc,tempdir,name);
  }
  return false;
}

/*static*/ bool
ErrorReport::Report(DWORD p_errorCode
                   ,struct _EXCEPTION_POINTERS* p_exception
                   ,CString& p_directory
                   ,CString& p_url)
{
  if (!g_reportException)
  {
    g_reportException = true;
    return false;
  }
  assert(s_instance != NULL);

  // One thread at the time
  AutoCritSec lock(&(s_instance->m_lock));

  // Getting the subject
  CString subject;
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
    subject.Format("Unknown error (0x%08x)",(unsigned int) p_errorCode);
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
        subject += CString(" (") + typeInfo->name() + ")";
      }
    }
  }

  // Getting the memory address of the exception
  if (p_errorCode == EXCEPTION_ACCESS_VIOLATION || p_errorCode == EXCEPTION_IN_PAGE_ERROR)
  {
    switch (p_exception->ExceptionRecord->ExceptionInformation[0])
    {
      case 0:  subject += " reading ";   break;
      case 1:  subject += " writing ";   break;
      case 8:  subject += " executing "; break;
      default: subject += " (unknown) "; break;
    }
    CString tmp;
#ifdef _M_X64
    tmp.Format("0x%I64X", p_exception->ExceptionRecord->ExceptionInformation[1]);
#else
    tmp.Format("0x%lx",p_exception->ExceptionRecord->ExceptionInformation[1]);
#endif
    subject += tmp;
  }
  // Getting the stack trace
  StackTrace trace(p_exception->ContextRecord, p_errorCode == EXCEPTION_CPP ? 2 : 0);

  // Sending the report
  s_instance->DoReport(subject,trace,p_directory,p_url);

  // Reset the exception handler
  return false;
}

/*static*/ void
ErrorReport::Report(int p_signal,unsigned int p_skip /* = 0 */,CString p_directory,CString p_url)
{
  assert(s_instance != NULL);

  // One thread at the time
  AutoCritSec lock(&(s_instance->m_lock));

  // Getting the subject
  CString subject;
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
    subject.Format("Unknown signal (%d)", p_signal);
  }
  // Getting the stacktrace
  StackTrace trace(p_skip + 1);

  // Sending an error report
  s_instance->DoReport(subject,trace,p_directory,p_url);
}

// Standard crash report as a file on the webroot URL directory
//
void
ErrorReport::DoReport(const CString&    p_subject
                     ,const StackTrace& p_trace
                     ,const CString&    p_webroot
                     ,const CString&    p_url) const
{
  // Getting proces information
  std::unique_ptr<ProcInfo> procInfo(new ProcInfo);
  FILETIME creationTime, exitTime, kernelTime, userTime;
  GetProcessTimes(GetCurrentProcess(),
                  &creationTime,
                  &exitTime,
                  &kernelTime,
                  &userTime);

  // Getting the error address
  const CString &errorAddress = p_trace.FirstAsString();

  CString version;
  version.Format("%s.%s",MARLIN_VERSION_NUMBER,MARLIN_VERSION_BUILD);

  // Format the message
  CString message = "SYSTEMUSER: " + procInfo->m_username + " on " + procInfo->m_computer + "\n"
                    "PROGRAM   : " MARLIN_PRODUCT_NAME ": version: " + version + ": " + p_subject + " in " + errorAddress + "\n"
                    "-------------\n"
                    "Version:       " + MARLIN_VERSION_NUMBER + "\n"
                    "Buildnumber:   " + MARLIN_VERSION_BUILD  + "\n"
                    "Builddate:     " + MARLIN_VERSION_DATE   + "\n\n"
                    + p_trace.AsString() + "\n";

  /// Info about the proces
  message += "Process\n"
             "-------\n";
  message += "Commandline:       " + CString(GetCommandLine()) + "\n";
  message += "Starttime:         " + FileTimeToString(creationTime) + "\n";
  message += "Local time:        " + procInfo->m_datetime + "\n";
  message += "Processorusage:    " + FileTimeToString(userTime, true) + " (user), "
                                   + FileTimeToString(kernelTime, true) + " (kernel)\n\n";
  // Info about the system
  message += "System\n"
             "------\n";
  message += "Computername:      " + procInfo->m_computer      + "\n";
  message += "Current user:      " + procInfo->m_username      + "\n";
  message += "Windows version:   " + procInfo->m_platform      + "\n";
  message += "Windows path:      " + procInfo->m_windows_path  + "\n";
  message += "Current path:      " + procInfo->m_current_path  + "\n";
  message += "Searchpath:        " + procInfo->m_pathspec      + "\n";
  message += "Nummeric format:   " + procInfo->m_system_number + " (system), " + procInfo->m_user_number + " (user)\n";
  message += "Date format:       " + procInfo->m_system_date   + " (system), " + procInfo->m_user_date   + " (user)\n";
  message += "Time format:       " + procInfo->m_system_time   + " (system), " + procInfo->m_user_time   + " (user)\n\n";

  // Info about loaded modules
  message += "Modules\n"
             "-------";

  for(auto module : procInfo->m_modules)
  {
    // Info about this module
    message += "\n"                 + module->m_full_path + "\n"
               "\tAddress:        " + module->m_load_address      + "\n"
               "\tOriginal name:  " + module->m_original_filename + "\n"
               "\tVersion:        " + module->m_fileversion       + "\n"
               "\tDescription:    " + module->m_file_description  + "\n"
               "\tProductname:    " + module->m_product_name      + "\n"
               "\tProductversion: " + module->m_product_version   + "\n";

  }
  // Writing the crash message
  ErrorReportWriteToFile("", message,p_webroot,p_url);
}


//////////////////////////////////////////////////////////////////////////
//
// Tinkering with the standard signal handlers
// to accomodate the Structured Exception Handlers (SEH)
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

  // Gather stack information. 
  ErrorReport::Report(signal,0,"","");
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
    signal(SignalStop[i], CallSignalHandler);
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