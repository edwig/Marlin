/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HPFCounter.cpp
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
#include "BaseLibrary.h"
#include "HPFCounter.h"

// XTOR: Started timer
// No need to call ::Start() on it
HPFCounter::HPFCounter()
{
  m_total = 0.0;
  m_end.QuadPart = 0L;
  QueryPerformanceFrequency(&m_frequency);
  QueryPerformanceCounter(&m_start);

  // Thread times init to 0.
  m_userTime.dwHighDateTime   = 0L;
  m_userTime.dwLowDateTime    = 0L;
  m_kernelTime.dwHighDateTime = 0L;
  m_kernelTime.dwLowDateTime  = 0L;

  m_userTimeSlice.dwHighDateTime   = 0L;
  m_userTimeSlice.dwLowDateTime    = 0L;
  m_kernelTimeSlice.dwHighDateTime = 0L;
  m_kernelTimeSlice.dwLowDateTime  = 0L;
}

// DTOR: Go away!
HPFCounter::~HPFCounter()
{
}

// Start the timer (again)
void
HPFCounter::Start()
{
  if(m_start.QuadPart == 0L)
  {
    QueryPerformanceCounter(&m_start);
  }
}

// Stop the timer
void
HPFCounter::Stop()
{
  if(m_start.QuadPart)
  {
    QueryPerformanceCounter(&m_end);
    m_total += ((double)(m_end.QuadPart - m_start.QuadPart)) / (double)m_frequency.QuadPart;

    m_start.QuadPart = 0L;
    m_end  .QuadPart = 0L;
  }

  // Get the thread times, ignoring creation and end
  FILETIME creation   = { 0, 0};
  FILETIME exitTime   = { 0, 0};
  FILETIME kernelTime = { 0, 0};
  FILETIME userTime   = { 0, 0};
  GetThreadTimes(GetCurrentThread(),&creation,&exitTime,&kernelTime,&userTime);

//   TRACE ("Thread times\n");
//   TRACE ("- User   hi : %x\n",userTime.dwHighDateTime);
//   TRACE ("- User   lo : %x\n",userTime.dwLowDateTime);
//   TRACE ("- Kernel hi : %x\n",kernelTime.dwHighDateTime);
//   TRACE ("- Kernel lo : %x\n\n",kernelTime.dwLowDateTime);

  // Calculate how much kernel time we just used since last time
  FILETIME slice = SubFiletimes(&kernelTime,&m_kernelTime);
  m_kernelTimeSlice = AddFiletimes(&m_kernelTimeSlice,&slice);
  m_kernelTime = kernelTime;

//   TRACE("Kernel Slice times\n");
//   TRACE ("- slice hi : %x\n",m_kernelTimeSlice.dwHighDateTime);
//   TRACE ("- slice lo : %x\n",m_kernelTimeSlice.dwLowDateTime);

  // Calculate how much user time we just used since last time
  slice = SubFiletimes(&userTime,&m_userTime);
  m_userTimeSlice = AddFiletimes(&m_userTimeSlice,&slice);
  m_userTime = userTime;

//   TRACE("User Slice times\n");
//   TRACE ("- slice hi : %x\n",m_userTimeSlice.dwHighDateTime);
//   TRACE ("- slice lo : %x\n",m_userTimeSlice.dwLowDateTime);
}

// Reset the total time, requery the start
void
HPFCounter::Reset()
{
  m_total = 0.0;
  m_end.QuadPart = 0L;
  if(m_start.QuadPart)
  {
    // Running timer
    QueryPerformanceCounter(&m_start);
  }
  m_kernelTimeSlice.dwHighDateTime = 0;
  m_kernelTimeSlice.dwLowDateTime  = 0;
  m_userTimeSlice  .dwHighDateTime = 0;
  m_userTimeSlice  .dwLowDateTime  = 0;
}

// Get time in miliseconds
double 
HPFCounter::GetCounter()
{
  Stop();
  return m_total;
}

FILETIME
HPFCounter::AddFiletimes(PFILETIME p_time1,PFILETIME p_time2)
{
  TheTime time1 = (((TheTime)(p_time1->dwHighDateTime)) << 32) + ((TheTime)p_time1->dwLowDateTime);
  TheTime time2 = (((TheTime)(p_time2->dwHighDateTime)) << 32) + ((TheTime)p_time2->dwLowDateTime);
  TheTime result = time1 + time2;

  FILETIME res;
  res.dwLowDateTime  = result & 0xFFFFFFFF;
  res.dwHighDateTime = result >> 32;

  return res;
}

FILETIME
HPFCounter::SubFiletimes(PFILETIME p_time1,PFILETIME p_time2)
{
  TheTime time1 = (((TheTime)(p_time1->dwHighDateTime)) << 32) + ((TheTime)p_time1->dwLowDateTime);
  TheTime time2 = (((TheTime)(p_time2->dwHighDateTime)) << 32) + ((TheTime)p_time2->dwLowDateTime);
  TheTime result = time1 - time2;

  FILETIME res;
  res.dwLowDateTime  = result & 0xFFFFFFFF;
  res.dwHighDateTime = result >> 32;

  return res;
}

// Query stopped or running timer
double 
HPFCounter::QueryCounter()
{
  // Stopped timer
  if(m_start.QuadPart == 0L)
  {
    return m_total;
  }

  // Timer is running. Add performance to total
  LARGE_INTEGER now;
  double total = m_total;

  QueryPerformanceCounter(&now);
  total += ((double)(now.QuadPart - m_start.QuadPart)) / (double)m_frequency.QuadPart;

  return total;
}
