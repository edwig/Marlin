/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ExecuteProcess.h
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
#pragma once

// Maximum length of a command line in MS-Windows OS
// See: https://support.microsoft.com/nl-nl/help/830473/command-prompt-cmd-exe-command-line-string-limitation
#define MAX_COMMANDLINE 8191

// Standard waiting times for a process
#define WAIT_FOR_INPUT_IDLE   60000  // 1.0 minute
#define WAIT_FOR_PROCESS_EXIT 90000  // 1.5 minute

// Error codes that can be returned
#define EXECUTE_OK                 0
#define EXECUTE_NO_PROGRAM_FILE   -1
#define EXECUTE_NO_INPUT_IDLE     -2
#define EXECUTE_TIMEOUT           -3
#define EXECUTE_NOT_STARTED   0xFFFF

int ExecuteProcess(XString  p_program
                  ,XString  p_arguments
                  ,bool     p_currentdir
                  ,XString& p_errormessage
                  ,int      p_showWindow  = SW_HIDE    // SW_SHOW / SW_HIDE
                  ,bool     p_waitForExit = false
                  ,bool     p_waitForIdle = false
                  ,unsigned p_factor      = 1
                  ,DWORD*   p_threadID    = NULL);

