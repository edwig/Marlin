/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: StackTrace.cpp
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
#include "StackTrace.h"
#include "XMLParser.h"
#include "WebConfig.h"

// To prevent:
// C:\Program Files (x86)\Windows Kits\8.1\Include\um\dbghelp.h(1544): warning C4091: 'typedef ': ignored on left of '' when no variable is declared
// The bug is in the Windows 8.1 SDK, not in our code base.
#pragma warning (disable: 4091)
#include <dbghelp.h>
#pragma warning (error: 4091)

#include <intrin.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Static buffers for symbol-lookup
// The buffers are static, so that they will work, even if the stack is overstressed
//
static IMAGEHLP_MODULE  g_module;
static char             g_symbuf[sizeof(IMAGEHLP_SYMBOL) + 256];
static IMAGEHLP_LINE    g_line;

// Dynamic loading wrapper around dbghelp.dll
// Needed for 32Bits and 64bits versions of dbghelp32.dll
// dbghelp32.dll and dbghelp64.dll included
//
namespace 
{

  // A bit of a hack for correct symbolname translation in 64 bit versions
#define STR_(x) #x
#define STR(x) STR_(x)

  struct DbgHelp
  {
    // Constructie
    //
    DbgHelp()
    {
      // Load the dbghelp library
      CString debugHelper = WebConfig::GetExePath();
#ifdef _WIN64
      debugHelper += "dbghelp64.dll";
#else
      debugHelper += "dbghelp32.dll";
#endif
      m_module = LoadLibrary(debugHelper);
      if(m_module)
      {
        // Find our entry points
        fnSymInitialize           = (SymInitialize_t *)          GetProcAddress(m_module, STR(SymInitialize));
        fnStackWalk               = (StackWalk_t *)              GetProcAddress(m_module, STR(StackWalk));
        fnSymFunctionTableAccess  = (SymFunctionTableAccess_t *) GetProcAddress(m_module, STR(SymFunctionTableAccess));
        fnSymGetModuleBase        = (SymGetModuleBase_t *)       GetProcAddress(m_module, STR(SymGetModuleBase));
        fnSymGetModuleInfo        = (SymGetModuleInfo_t *)       GetProcAddress(m_module, STR(SymGetModuleInfo));
        fnSymGetSymFromAddr       = (SymGetSymFromAddr_t *)      GetProcAddress(m_module, STR(SymGetSymFromAddr));
        fnSymGetLineFromAddr      = (SymGetLineFromAddr_t *)     GetProcAddress(m_module, STR(SymGetLineFromAddr));
        fnSymCleanup              = (SymCleanup_t *)             GetProcAddress(m_module, STR(SymCleanup));
      }
    }
    // Destruction
    //
    ~DbgHelp()
    {
      if(m_module != NULL)
      {
        FreeLibrary(m_module);
      }
    }

    // Library-handle
    HMODULE m_module;

    // Functionpointers
    typedef BOOL (__stdcall SymInitialize_t)(HANDLE hProcess,
                                             PCSTR  UserSearchPath,
                                             BOOL   fInvadeProcess);
    SymInitialize_t *fnSymInitialize { nullptr};

    typedef BOOL (__stdcall StackWalk_t)(    DWORD                          MachineType,
                                             HANDLE                         hProcess,
                                             HANDLE                         hThread,
                                             LPSTACKFRAME                   StackFrame,
                                             PVOID                          ContextRecord,
                                             PREAD_PROCESS_MEMORY_ROUTINE   ReadMemoryRoutine,
                                             PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
                                             PGET_MODULE_BASE_ROUTINE       GetModuleBaseRoutine,
                                             PTRANSLATE_ADDRESS_ROUTINE     TranslateAddress);
    StackWalk_t *fnStackWalk {nullptr};

    typedef PVOID (__stdcall SymFunctionTableAccess_t)(HANDLE hProcess,DWORD_PTR AddrBase);
    SymFunctionTableAccess_t *fnSymFunctionTableAccess {nullptr};

    typedef DWORD_PTR (__stdcall SymGetModuleBase_t)(HANDLE hProcess,DWORD_PTR dwAddr);
    SymGetModuleBase_t *fnSymGetModuleBase {nullptr};

    typedef BOOL (__stdcall SymGetModuleInfo_t)(HANDLE           hProcess,
                                                DWORD_PTR        dwAddr,
                                                PIMAGEHLP_MODULE ModuleInfo);
    SymGetModuleInfo_t *fnSymGetModuleInfo {nullptr};

    typedef BOOL (__stdcall SymGetSymFromAddr_t)(HANDLE           hProcess,
                                                 DWORD_PTR        qwAddr,
                                                 PDWORD_PTR       pwdDisplacement,
                                                 PIMAGEHLP_SYMBOL Symbol);
    SymGetSymFromAddr_t *fnSymGetSymFromAddr {nullptr};

    typedef BOOL (__stdcall SymGetLineFromAddr_t)(HANDLE          hProcess,
                                                  DWORD_PTR       dwAddr,
                                                  PDWORD          pdwDisplacement,
                                                  PIMAGEHLP_LINE  Line);
    SymGetLineFromAddr_t *fnSymGetLineFromAddr {nullptr};

    typedef BOOL (__stdcall SymCleanup_t)(HANDLE hProcess);
    SymCleanup_t *fnSymCleanup {nullptr};
  };
}

#pragma auto_inline(off)
DWORD_PTR
GetInstructionPointer()
{
  return DWORD_PTR(_ReturnAddress());
}

DWORD_PTR
GetStackPointer()
{
  return DWORD_PTR(_AddressOfReturnAddress()) + sizeof(void *);
}
#pragma auto_inline(on)

StackTrace::StackTrace(unsigned int p_skip /* = 0 */)
{
  // Getting context information
  CONTEXT context;
#ifdef _WIN64
  context.Rip = GetInstructionPointer();
  context.Rsp = GetStackPointer();
  context.Rbp = DWORD_PTR(_AddressOfReturnAddress()) - sizeof(void *);
#else /* niet _WIN64 */
  context.Eip = GetInstructionPointer();
  context.Esp = GetStackPointer();
  context.Ebp = DWORD_PTR(_AddressOfReturnAddress()) - sizeof(void *);
#endif

  // Build a strackframe
  Process(&context, p_skip + 1);
}

void
StackTrace::Process(CONTEXT *context, unsigned int overslaan)
{
  HANDLE thread  = GetCurrentThread();
  HANDLE process = GetCurrentProcess();

  // Open dbghelp
  DbgHelp dbgHelp;
  if(dbgHelp.m_module == NULL)
  {
    // dbghelp not found
    return;
  }

  // Initialize symbol handling
  dbgHelp.fnSymInitialize(process, const_cast<LPSTR>(static_cast<LPCSTR>(WebConfig::GetExePath())), TRUE);

  // Initialize first stack frame
#ifdef _WIN64
  STACKFRAME64 stackFrame;
  memset(&stackFrame, '\0', sizeof(stackFrame));
  stackFrame.AddrPC   .Offset = context->Rip;
  stackFrame.AddrStack.Offset = context->Rsp;
  stackFrame.AddrFrame.Offset = context->Rbp;
#else /* not _WIN64 */
  STACKFRAME stackFrame;
  memset(&stackFrame, '\0', sizeof(stackFrame));
  stackFrame.AddrPC   .Offset = context->Eip;
  stackFrame.AddrStack.Offset = context->Esp;
  stackFrame.AddrFrame.Offset = context->Ebp;
#endif
  stackFrame.AddrPC   .Mode = AddrModeFlat;
  stackFrame.AddrStack.Mode = AddrModeFlat;
  stackFrame.AddrFrame.Mode = AddrModeFlat;

  // Iterate through the stack
  unsigned int i=0;
  while (dbgHelp.fnStackWalk(
#ifdef _WIN64
    IMAGE_FILE_MACHINE_AMD64,
#else /* not _WIN64 */
    IMAGE_FILE_MACHINE_I386,
#endif
    process,
    thread,
    &stackFrame,
    context,
    NULL,
    dbgHelp.fnSymFunctionTableAccess,
    dbgHelp.fnSymGetModuleBase,
    NULL) && (i++ < MaxFrames))
  {
    // Skipping ?
    if (overslaan > 0)
    {
      overslaan--;
      continue;
    }

    // For all stackframes the topmost points to the program counter
    // for the next instruction. Correct by subtracting for a call instruction
    DWORD_PTR adres = stackFrame.AddrPC.Offset;
    if (i != 1)
    {
      adres -= 5;
    }

    // Alloc a nem entry for the trace
    m_trace.push_back(Frame());
    Frame &frame = m_trace.back();
    frame.m_address = adres;

    {
      // Getting the module for this address
      g_module.SizeOfStruct = sizeof(g_module);
      if (dbgHelp.fnSymGetModuleInfo(process, adres, &g_module))
      {
        frame.m_module = g_module.ModuleName;
      }
    }

    {
      // Initializing the buffer for the symbol name
      IMAGEHLP_SYMBOL *symbol = reinterpret_cast<IMAGEHLP_SYMBOL *>(g_symbuf);
      symbol->SizeOfStruct    = sizeof(g_symbuf);
      symbol->MaxNameLength   = sizeof(g_symbuf) - sizeof(SYMBOL_INFO) + 1;

      // Getting the symbol name for this address
      DWORD_PTR offset;
      if (dbgHelp.fnSymGetSymFromAddr(process, adres, &offset, symbol))
      {
        frame.m_function = symbol->Name;
        frame.m_offset   = offset;
      }
    }

    {
      // Getting the linenumber of the address
      g_line.SizeOfStruct = sizeof(g_line);
      DWORD offset;
      if (dbgHelp.fnSymGetLineFromAddr(process, adres, &offset, &g_line))
      {
        // Get the position of the one-before-last seperator in the path
        char *bestand = g_line.FileName, *tmp = g_line.FileName, *p = g_line.FileName;
        while (*p)
        {
          if (*(p++) == '\\')
          {
            bestand = tmp;
            tmp = p;
          }
        }

        frame.m_fileName = bestand;
        frame.m_line = g_line.LineNumber;
      }
    }
  }
  // Ready with symbol handling
  dbgHelp.fnSymCleanup(process);
}

// Convert to string
//
CString
StackTrace::AsString(bool p_path /* = true */) const
{
  // Bouw een stringrepresentatie op
  CString tmp;
  CString result = "Address         Module                Function\n"
                   "--------------- --------------------- ------------------------\n";

  for(Trace::const_iterator iter = m_trace.begin(),end = m_trace.end();iter != end;++iter)
  {
    const Frame& frame = *iter;
    // Add address and function
#ifdef _WIN64
    tmp.Format("0x%012I64X  %-20.20s", frame.m_address, frame.m_module.GetString());
#else
    tmp.Format("0x%lX  %-20.20s",frame.m_address,frame.m_module.GetString());
#endif
    result += tmp;
    if (frame.m_function.IsEmpty())
    {
      result += "\n";
      continue;
    }

    // Add function and offset
#ifdef _WIN64
    tmp.Format("  %s + 0x%I64X",frame.m_function.GetString(),frame.m_offset);
#else
    tmp.Format("  %s + 0x%08X", frame.m_function.GetString(),frame.m_offset);
#endif
    result += tmp;
    if (!p_path || frame.m_fileName.IsEmpty())
    {
      result += "\n";
      continue;
    }

    // Add file and line number
    tmp.Format("  [%s:%d]\n", frame.m_fileName.GetString(), frame.m_line);
    result += tmp;
  }

  return result;
}

CString
StackTrace::AsXMLString() const
{
  // Build a string representation
  CString result = "<stack>\n", tmp;
  for (Trace::const_iterator iter = m_trace.begin(), end = m_trace.end();
       iter != end;
       ++iter)
  {
    const Frame &frame = *iter;

    // Add address
#ifdef _WIN64
    tmp.Format("\t<frame pc=\"0x%I64X\">",frame.m_address);
#else
    tmp.Format("\t<frame pc=\"0x%08X\">", frame.m_address);
#endif
    result += tmp;

    // Add module
    if (!frame.m_module.IsEmpty())
    {
      tmp.Format("<module>%s</module>", PrintXmlString(frame.m_module).GetString());
      result += tmp;
    }

    // Add function and offset
    if (!frame.m_function.IsEmpty())
    {
#ifdef _WIN64
      tmp.Format("<symbool naam=\"%s\" offset=\"%I64d\" />",
                 PrintXmlString(frame.m_function).GetString(),
                 frame.m_offset);
#else
      tmp.Format("<symbool naam=\"%s\" offset=\"%d\" />",
                 PrintXmlString(frame.m_function).GetString(),
                 frame.m_offset);
#endif
      result += tmp;
    }

    // Add file and line number
    if (!frame.m_fileName.IsEmpty())
    {
      tmp.Format("<regel bestand=\"%s\" nummer=\"%d\" />",
                 PrintXmlString(frame.m_fileName).GetString(),
                 frame.m_line);
      result += tmp;
    }

    result += "</frame>\n";
  }

  return result + "</stack>\n";
}

// Convert first to string
//
CString
StackTrace::FirstAsString() const
{
  if(m_trace.empty())
  {
#ifdef _WIN64
    return "No stack trace: missing dbghelp64.dll?";
#else
    return "No stack trace: missing dbghelp32.dll?";
#endif
  }
  const Frame &frame = m_trace.front();

  // Getting the result
  CString result;
  if (!frame.m_function.IsEmpty())
  {
#ifdef _WIN64
    result.Format("%s + 0x%I64X",frame.m_function.GetString(),frame.m_offset);
#else
    result.Format("%s + 0x%08X", frame.m_function.GetString(),frame.m_offset);
#endif
  }
  else
  {
#ifdef _WIN64
    result.Format("0x%I64X",frame.m_address);
#else
    result.Format("0x%08X", frame.m_address);
#endif
  }

  return result;
}

CString
StackTrace::FirstAsXMLString() const
{
  return PrintXmlString(FirstAsString());
}
