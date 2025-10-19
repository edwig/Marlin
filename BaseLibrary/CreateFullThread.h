/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: CreateFullThread.h
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

// Prototype for the starting point of a thread
typedef unsigned (__stdcall *LPFN_THREADSTART)(void* p_context);

// Create a thread that inherits the process security context
HANDLE CreateFullThread(LPFN_THREADSTART p_start                    // Starting point of the new thread
                       ,void*            p_context                  // Context argument of the starting point
                       ,unsigned*        p_threadID  = nullptr      // Possibly return the thread ID
                       ,int              p_stacksize = 0            // Possibly set extra stack space for the thread
                       ,bool             p_inherit   = false        // Sub-processes can inherit the security handle
                       ,bool             p_suspended = false);      // Create suspended (Use "ResumeThread")

// Setting the name of a thread
void   SetThreadName(char* threadName,DWORD dwThreadID);

