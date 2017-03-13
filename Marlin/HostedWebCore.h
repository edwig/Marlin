/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HostedWebCore.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2017 ir. W.E. Huisman
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
#include <hwebcore.h>

// extern "C"
// {
// 
//   // Activate Hosted Web Core
//   typedef HRESULT (_stdcall *PFN_WEB_CORE_ACTIVATE)(IN PCWSTR pszAppHostConfigFile
//                                          ,IN PCWSTR pszRootWebConfigFile
//                                          ,IN PCWSTR pszInstanceName);
// 
//   typedef HRESULT(_stdcall *PFN_WEB_CORE_SET_METADATA_DLL_ENTRY)(IN PCWSTR pszMetadataType,IN PCWSTR pszValue);
// 
//   // Stop the Hosted Web core (1 = immediate, 0 = use app pool settings)
//   typedef HRESULT(_stdcall *PFN_WEB_CORE_SHUTDOWN)(IN DWORD immediate);
// 
// }

// Callbacks to the application server
typedef void(*PFN_SERVERSTATUS)(void);
typedef void(*PFN_SETMETADATA)(void);

// The main entry for a Hosted Web Core application
int HWC_main(int argc,char* argv[]);