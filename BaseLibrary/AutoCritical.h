/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: AutoCritical.h
//
// BaseLibrary: Indispensable general objects and functions
// 
// Copyright (c) 2014-2025 ir. W.E. Huisman
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

class Critical
{
public:
  Critical();
 ~Critical();

  void Lock();
  bool TryLock();
  void Unlock();

private:
  CRITICAL_SECTION m_critical;
};

// New type critical section recursive mutex lock
class CritSection
{
public:
  explicit CritSection(Critical& p_section) : m_section(&p_section)
  {
    p_section.Lock();
  }

  ~CritSection()
  {
    m_section->Unlock();
  }
private:
  Critical* m_section;
};

// Old type of critical section
class AutoCritSec
{
public:
  explicit AutoCritSec(CRITICAL_SECTION* section) : m_section(section)
  {
    EnterCriticalSection(m_section);
  }
  ~AutoCritSec()
  {
    LeaveCriticalSection(m_section);
  }
  void Unlock() { LeaveCriticalSection(m_section); };
  void Relock() { EnterCriticalSection(m_section); };
private:
  CRITICAL_SECTION* m_section;
};

class AutoTrySection
{
public:
  explicit AutoTrySection(CRITICAL_SECTION* section) : m_section(section)
  {
    m_succeeded = TryEnterCriticalSection(m_section) != 0;
  }
  ~AutoTrySection()
  {
    if(m_succeeded)
    {
      LeaveCriticalSection(m_section);
    }
  }
  bool HasLock()
  {
    return m_succeeded;
  }
private:
  bool              m_succeeded;
  CRITICAL_SECTION* m_section;
};

