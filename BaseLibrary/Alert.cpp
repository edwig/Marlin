/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Alert.cpp
//
// BaseLibrary: Indispensable general objects and functions
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
#include "pch.h"
#include "Alert.h"
#include "GetExePath.h"
#include <strsafe.h>
#include <sys/timeb.h>
#include <time.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
//
// CONFIGURATION
//
//////////////////////////////////////////////////////////////////////////

bool g_alertConfigured = false;
int  g_alertModules    = 0;
char g_alertPath[MAX_ALERT_MODULES][MAX_PATH + 1];
unsigned __int64 g_alertCounter = 0;

// Registers an alert log path for a module
// Returns the module's alert number
int ConfigureApplicationAlerts(XString p_path)
{
  if(g_alertModules >= 0 && g_alertModules < MAX_ALERT_MODULES)
  {
    StringCchCopy(g_alertPath[g_alertModules],MAX_PATH,p_path.GetString());
    if(p_path.GetLength() > 3)
    {
      g_alertConfigured = true;
      if(p_path.Right(1) != "\\")
      {
        StringCchCat(g_alertPath[g_alertModules],MAX_PATH,"\\");
      }
    }
    // Return the registered module number
    // To be used for every WMI log transaction and error log
    return g_alertModules++;
  }
  // Failed to register an extra module
  return -1;
}

// Returns the alert log path for a module
XString GetAlertlogPath(int p_module)
{
  if(p_module >= 0 && p_module < g_alertModules)
  {
    return XString(g_alertPath[p_module]);
  }
  return "";
}

//////////////////////////////////////////////////////////////////////////
//
// ALERT
//
//////////////////////////////////////////////////////////////////////////

// Create the alert. Returns the alert number (natural ordinal number)
__int64 CreateAlert(LPCTSTR p_function,LPCTSTR p_oserror,LPCTSTR p_eventdata,int p_module /*=0*/)
{
  // See if we are configured
  if(g_alertConfigured == false)
  {
    return 0;
  }

  // Check that it is a valid alert module
  if(p_module < 0 || p_module >= g_alertModules)
  {
    return 0;
  }

  // One extra alert number: threadsafe
  unsigned __int64 alert = InterlockedIncrement(&g_alertCounter);

  try
  {
    XString fileName;

    // Getting the time of the alert
    __timeb64 now;
    struct tm today;
    _ftime64_s(&now);
    _localtime64_s(&today,&now.time);

    // Creating a filename for the alert
    fileName.Format("%sAlert_%4.4d_%2.2d_%2.2d_%2.2d_%2.2d_%2.2d_%3.3d_%I64u.log"
                    ,g_alertPath[p_module]
                    ,today.tm_year + 1900
                    ,today.tm_mon + 1
                    ,today.tm_mday
                    ,today.tm_hour
                    ,today.tm_min
                    ,today.tm_sec
                    ,now.millitm
                    ,alert);

    // Create the file (and directory) and open it in write mode
    WinFile file(fileName);
    if(file.Open(winfile_write))
    {
      // Directly print our information to the alert file
      file.Write("APPLICATION ALERT FILE\n");
      file.Write("======================\n");
      file.Write("\n");
      file.Format("Server module: %s\n",GetExeFile().GetString());
      file.Format("Alert moment : %4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d.%3.3d\n"
                  ,today.tm_year + 1900
                  ,today.tm_mon  + 1
                  ,today.tm_mday
                  ,today.tm_hour
                  ,today.tm_min
                  ,today.tm_sec
                  ,now.millitm);
      file.Format("Alert number : %I64d\n",alert);
      file.Format("App function : %s\n",p_function);
      file.Format("Last os error: %s\n",p_oserror);
      file.Format("Event text   : %s\n",p_eventdata);
      file.Write("\n");
      file.Write("*** End of alert ***\n");

      // Ignore return value: Nothing can be done if file cannot be flushed
      file.Close();
    }
  }
  catch(...)
  {
    // Tight spot: nothing we can do now!!!
    // WMI Event already done
    // Logfile already done
    // Let's ignore this situation
    // Alert directory not set / full ??
  }
  return alert;
}
