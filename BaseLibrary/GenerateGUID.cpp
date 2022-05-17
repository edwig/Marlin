/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: GenerateGUID.cpp
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
#include "GenerateGUID.h"
#include <combaseapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Caller **MUST** do the COM+ (un)initialize with
// CoInitialize
// CoUninitialize

_Check_return_ WINOLEAPI CoCreateGuid(_Out_ GUID* pguid);

XString GenerateGUID()
{
  _TUCHAR *guidStr = 0x00;
  GUID    *pguid   = 0x00;
  XString result;
  
  pguid = new GUID; 
  // COM+ Initialize is already done
  if(SUCCEEDED(CoCreateGuid(pguid)))
  {
    // Convert the GUID to a string
    if(UuidToString(pguid,&guidStr) == RPC_S_OK)
    {
      result = guidStr;
    }
  }
  delete pguid;
  return result;
}
