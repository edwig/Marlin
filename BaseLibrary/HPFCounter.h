/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HPFCounter.h
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

typedef __time64_t TheTime;

// HIGH PERFORMANCE COUNTER
// Can measure times smaller than 0.016 seconds 

class HPFCounter
{
public:
  // Create a started timer. No need to call ::Start()
  HPFCounter();
 ~HPFCounter();

  // Start or restart the timer
  void      Start();
  // Stop the timer
  void      Stop();
  // Reset the timer
  void      Reset();
  // Get time in miliseconds
  double    GetCounter();
  // Query a stopped or running timer
  double    QueryCounter();
  // Get pointer to the user time of the thread
  PFILETIME GetUserTime();
  // Get pointer to the kernel time of the thread
  PFILETIME GetKernelTime();
  // Get last user time slice
  PFILETIME GetUserTimeSlice();
  // Get last kernel time slice
  PFILETIME GetKernelTimeSlice();

  // Operations on filetimes
  static FILETIME AddFiletimes(PFILETIME p_time1,PFILETIME p_time2);
  static FILETIME SubFiletimes(PFILETIME p_time1,PFILETIME p_time2);

private:
  double        m_total;        // Seconds, and fractions of seconds
  LARGE_INTEGER m_start;        // Start moment
  LARGE_INTEGER m_end;          // End moment
  LARGE_INTEGER m_frequency;    // Machine dependent frequency
  FILETIME      m_userTime;     // 100 nanoseconds user time (total)
  FILETIME      m_kernelTime;   // 100 nanoseconds kernel time (total)
  FILETIME      m_userTimeSlice;
  FILETIME      m_kernelTimeSlice;
};

inline PFILETIME
HPFCounter::GetUserTime()
{
  return &m_userTime;
}

inline PFILETIME
HPFCounter::GetKernelTime()
{
  return &m_kernelTime;
}

inline PFILETIME
HPFCounter::GetUserTimeSlice()
{
  return &m_userTimeSlice;
}

inline PFILETIME
HPFCounter::GetKernelTimeSlice()
{
  return &m_kernelTimeSlice;
}