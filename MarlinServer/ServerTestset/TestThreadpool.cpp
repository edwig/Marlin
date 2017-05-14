/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestTime.cpp
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
#include "stdafx.h"
#include "TestServer.h"
#include "ThreadPool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int totalChecks = 2;
static unsigned number = 0;
static ThreadPool* pool = nullptr;
const  unsigned TH_SLEEP =  314;
const  unsigned CYCLES   = 1000;
const  unsigned WAITTIME =  100;

void callback1(void* p_pnt)
{
  bool result = false;
  xprintf("TESTING THE THREADPOOL\n");
  xprintf("======================\n");

  // Counting (doing work :-) )
  for(unsigned int index = 0;index < CYCLES; ++index)
  {
    number = index;
  }
  xprintf("Going to sleep on: %s\n",(LPCTSTR)p_pnt);
  p_pnt = pool->SleepThread(TH_SLEEP,p_pnt);
  if(p_pnt == nullptr)
  {
    xprintf("Cannot sleep on thread, or thread terminated\n");
  }
  else
  {
    xprintf("Waking from sleep: %s\n",(LPCTSTR)p_pnt);
    result = true;
    --totalChecks;
  }
  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  qprintf("Test awoken thread result   : %s\n",result ? "OK" : "ERROR");
  // Ready with the thread
}

int TestThreadPool(ThreadPool* p_pool)
{
  pool = p_pool;
  int errors = 0;
  char* text1("This is a longer text for the pool.");
  char* text2("This is another! text for the pool.");

  xprintf("TESTING SLEEPING/WAKING THREAD FUNCTIONS OF THREADPOOL\n");
  xprintf("======================================================\n");

  p_pool->SubmitWork(callback1,(void*)text1);

  // Wait until thread has done work and is gone sleeping
  while(number < (CYCLES - 1))
  {
    Sleep(WAITTIME);
  }
  Sleep(3 * WAITTIME);
  
  // Waking the thread with another string as result
  bool result = p_pool->WakeUpThread(TH_SLEEP,(void*)text2);
  if(result)
  {
    --totalChecks;
  }

  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  qprintf("Test sleep/waking thread    : %s\n",result ? "OK" : "ERROR");

  // AGAIN NOW ELIMINATING THE THREAD
  number = 0;
  p_pool->SubmitWork(callback1,(void*)text1);
  
  // Wait until thread has done work and is gone sleeping
  while(number < (CYCLES - 1))
  {
    Sleep(WAITTIME);
  }
  Sleep(3 * WAITTIME);

  // Remove the thread and test for no leaking memory
  // p_pool->EliminateSleepingThread(TH_SLEEP);
  result = p_pool->WakeUpThread(TH_SLEEP,(void*)text2);

  return errors;
}

int 
AfterTestThreadpool()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("(HTTP)Threadpool sleeping/waking/submitwork    : %s\n",totalChecks > 0 ? "ERROR" : "OK");
  return totalChecks > 0;
}
