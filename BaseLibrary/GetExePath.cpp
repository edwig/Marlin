////////////////////////////////////////////////////////////////////////
//
//
#include "pch.h"
#include "BaseLibrary.h"
#include "GetExePath.h"

static char g_staticAddress;

HMODULE GetModuleHandle()
{
  // Getting the module handle, if any
  // If it fails, the process names will be retrieved
  // Thus we get the *.DLL handle in IIS instead of a
  // %systemdrive\system32\inetsrv\w3wp.exe path
  HMODULE module = NULL;
  GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
                   ,static_cast<LPCTSTR>(&g_staticAddress)
                   ,&module);
  return module;
}

XString GetExeFile()
{
  char buffer[_MAX_PATH + 1];

  HMODULE module = GetModuleHandle();

  // Retrieve the path
  GetModuleFileName(module,buffer,_MAX_PATH);
  return XString(buffer);
}

XString GetExePath()
{
  // Retrieve the path
  XString applicatiePlusPad = GetExeFile();

  int slashPositie = applicatiePlusPad.ReverseFind('\\');
  if(slashPositie == 0)
  {
    return "";
  }
  return applicatiePlusPad.Left(slashPositie + 1);
}

void
TerminateWithoutCleanup(int p_exitcode)
{
  //  Dat was het dan... Jammer, maar helaas
  HANDLE hProcess = OpenProcess(PROCESS_TERMINATE,FALSE,GetCurrentProcessId());
  if(!hProcess || !TerminateProcess(hProcess,p_exitcode))
  {
    if(hProcess) CloseHandle(hProcess);
    _exit(p_exitcode);
  }
  // Geen rechten, dan maar kale abort
  abort();
}

// WARNING:
// CANNOT BE CALLED FROM AN INTERNET IIS APPLICATION
void
CheckExePath(XString p_runtimer,XString p_productName)
{
  char buffer   [_MAX_PATH];
  char drive    [_MAX_DRIVE];
  char directory[_MAX_DIR];
  char filename [_MAX_FNAME];
  char extension[_MAX_EXT];

  GetModuleFileName(GetModuleHandle(NULL),buffer,_MAX_PATH);
  _splitpath_s(buffer,drive,_MAX_DRIVE,directory,_MAX_DIR,filename,_MAX_FNAME,extension,_MAX_EXT);

  XString runtimer = XString(filename) + XString(extension);
  XString title = "Installation";
  if(runtimer.CompareNoCase(p_runtimer))
  {
    XString errortext;
    errortext.Format("You have started the '%s' program, but in fact it really is: '%s'\n"
                       "Your product %s cannot function properly because of this rename action.\n"
                       "\n"
                       "Contact your friendly (system) administrator about this problem.\n"
                       "Repair the installation and then retry this action."
                       ,runtimer.GetString(),p_runtimer.GetString(),p_productName.GetString());
    ::MessageBox(NULL,errortext,title,MB_OK | MB_ICONERROR);
    TerminateWithoutCleanup(-4);
  }
}
