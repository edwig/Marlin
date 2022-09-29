/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: StackTrace.h
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
#include <list>

// From <winnt.h>
typedef struct _CONTEXT CONTEXT;

// Represents a stacktrace
//
class StackTrace 
{
public:
  // Maximum number of stack frames to list
  enum { MaxFrames = 64 };

  // Construction
  StackTrace(unsigned int p_skip = 0);
  StackTrace(CONTEXT* p_context, unsigned int p_skip = 0)
  {
    Process(p_context, p_skip);
  }

  // Convert to string types
  XString AsString(bool p_path = true) const;
  XString AsXMLString() const;

  // Convert first to string
  XString FirstAsString() const;
  XString FirstAsXMLString() const;

private:
  // Frame in a stacktrace
  struct Frame
  {
    DWORD_PTR  m_address { NULL };
    XString    m_module;
    XString    m_function;
    DWORD_PTR  m_offset  { NULL };
    XString    m_fileName;
    DWORD      m_line    { 0 };
  };
  // Full trace
  typedef std::list<Frame> Trace;
  Trace   m_trace;

  // Getting the stacktrace
  void Process(CONTEXT *p_context, unsigned int p_skip);
};

