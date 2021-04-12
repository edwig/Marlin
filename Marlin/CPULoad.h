/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: CPULoad.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2021 ir. W.E. Huisman
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
//////////////////////////////////////////////////////////////////////////
//

// ONLY INCLUDED BY ThreadPool.cpp !!
//
//////////////////////////////////////////////////////////////////////////

static float
CalculateCPULoad(unsigned long long idleTicks,unsigned long long totalTicks)
{
  static unsigned long long _previousTotalTicks = 0;
  static unsigned long long _previousIdleTicks  = 0;

  unsigned long long totalTicksSinceLastTime = totalTicks - _previousTotalTicks;
  unsigned long long idleTicksSinceLastTime  = idleTicks  - _previousIdleTicks;


  float ret = 1.0f - ((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime) / totalTicksSinceLastTime : 0);

  _previousTotalTicks = totalTicks;
  _previousIdleTicks  = idleTicks;
  return ret;
}

static unsigned long long
FileTimeToInt64(const FILETIME & ft)
{
  return (((unsigned long long)(ft.dwHighDateTime)) << 32) | ((unsigned long long)ft.dwLowDateTime);
}

// Returns 1.0f for "CPU fully pinned", 0.0f for "CPU idle", or somewhere in between
// You'll need to call this at regular intervals, since it measures the load between
// the previous call and the current one.  Returns -1.0 on error.
float GetCPULoad(CRITICAL_SECTION* m_lock)
{
  FILETIME idleTime,kernelTime,userTime;
  EnterCriticalSection(m_lock);
  float load = GetSystemTimes(&idleTime,&kernelTime,&userTime) ?
               CalculateCPULoad(FileTimeToInt64(idleTime),FileTimeToInt64(kernelTime) + FileTimeToInt64(userTime)) : -1.0f;
  LeaveCriticalSection(m_lock);
  return load;
}
