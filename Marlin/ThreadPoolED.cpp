/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ThreadPoolED.cpp
//
// Marlin Server: Internet server/client
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
#include "StdAfx.h"
#include "ThreadPoolED.h"
#include "CPULoad.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
//
// ThreadPool-Event-Driven (ED)
//
// This was also the original threadpool of the Marlin project
// As long as threads are available, they are awoken on a wait-event
// This threadpool was replaced by the Asynchronous I/O Completion port
// threadpool that now bears the name 'ThreadPool'
//
//////////////////////////////////////////////////////////////////////////

// A means to be free to debug the threadpool in debug mode
#ifdef DEBUG_THREADPOOL
#define TP_TRACE0(sz)        TRACE0(sz)
#define TP_TRACE1(sz,p1)     TRACE1(sz,p1)
#define TP_TRACE2(sz,p1,p2)  TRACE2(sz,p1,p2)
#else
#define TP_TRACE0(sz)        ;
#define TP_TRACE1(sz,p1)     ;
#define TP_TRACE2(sz,p1,p2)  ;
#endif

// Static function, running a thread
static unsigned _stdcall RunThread(void* p_myThread);

//////////////////////////////////////////////////////////////////////////
//
// The Threadpool
//
ThreadPoolED::ThreadPoolED()
{
  InitializeCriticalSection(&m_critical);
}

ThreadPoolED::ThreadPoolED(int p_minThreads,int p_maxThreads)
           :m_minThreads(p_minThreads)
           ,m_maxThreads(p_maxThreads)
{
  InitializeCriticalSection(&m_critical);
}

ThreadPoolED::~ThreadPoolED()
{
  StopThreadPool();
  RunCleanupJobs();
  DeleteCriticalSection(&m_critical);
}

// Pool locking from a single thread state change
void 
ThreadPoolED::LockPool()
{
  EnterCriticalSection(&m_critical);
}

void 
ThreadPoolED::UnlockPool()
{
  LeaveCriticalSection(&m_critical);
}

// Initialize our new threadpool
void
ThreadPoolED::InitThreadPool()
{
  // See if we have already done this
  if(m_initialized)
  {
    return;
  }
  TP_TRACE0("Init threadpool\n");
  m_initialized = true;

  // Check the logic of maxThreads
  if(m_maxThreads < NUM_THREADS_DEFAULT)
  {
    m_maxThreads = NUM_THREADS_DEFAULT;
  }
  if(m_maxThreads > NUM_THREADS_MAXIMUM)
  {
    m_maxThreads = NUM_THREADS_MAXIMUM;
  }
  // Check the logic of minThreads
  if(m_minThreads >= m_maxThreads)
  {
    m_minThreads = NUM_THREADS_DEFAULT / 2;
  }
  // HARD CODED MINIMUM. OTHERWISE THERE IS NO REASON TO 
  // GO THROUGH ALL THE TROUBLE OF CREATING A THREADPOOL!
  if(m_minThreads < NUM_THREADS_MINIMUM)
  {
    m_minThreads = NUM_THREADS_MINIMUM;
  }

  // Create our minimum threads
  for(int ind = 0; ind < m_minThreads; ++ind)
  {
    CreateThreadPoolThread();
  }
  // Open for business
  m_openForWork = true;
}

// Stopping the thread pool
void  
ThreadPoolED::StopThreadPool()
{
  TP_TRACE0("Stopping threadpool\n");
  m_openForWork = false;

  // Waiting for the work queue to drain
  // Waiting for max of 30 seconds
  for(unsigned ind = 0; ind < 3000; ++ind)
  {
    if(m_work.empty()) break;
    Sleep(100);
  }
  // Try stopping all threads, signaling to remove
  {
    // Extra lock on the stack, so we change the state of all threads
    // before having them processing their events.
    AutoLockTP lock(&m_critical);

    for(auto& thread : m_threads)
    {
      TP_TRACE1("Stopping thread: %d\n",ind);
      StopThread(thread);
    }
    TP_TRACE1("Total items of work still in the work-queue: %d\n",m_work.size());
  }
  // Wait for the queue to become idle
  bool idle = false;
  unsigned waitTime = 50;
  // wait 50,100,200,400,800,1600,3200,6400 milliseconds
  for(unsigned ind = 0;ind < 8; ++ ind)
  {
    // Wait period of time
    Sleep(waitTime);
    waitTime *= 2;
    // See if threadpool has become idle
    {
      AutoLockTP lock(&m_critical);
      if(m_threads.size() == 0)
      {
        idle = true;
        break;
      }
    }
  }

  // No longer initialize after this point
  AutoLockTP lock(&m_critical);
  m_initialized = false;

  // Force the queue to a halt!!
  // Do not complain about "TerminateThread". It is the only way. We know!
  if(!idle)
  {
    for(auto& thread : m_threads)
    {
      TP_TRACE1("!! TERMINATING thread: %d\n",ind);
      TerminateThread(thread->m_thread,0);
      CloseHandle(thread->m_event);
    }
  }

  // Free all thread registrations
  for(auto& thread : m_threads)
  {
    delete thread;
  }
  m_threads.clear();
}

// Create a thread in the threadpool
ThreadRegister*
ThreadPoolED::CreateThreadPoolThread(DWORD p_heartbeat /*=INFINITE*/)
{
  AutoLockTP lock(&m_critical);

  ThreadRegister* th = new ThreadRegister();

  if(th)
  {
    th->m_pool     = this;
    th->m_state    = ThreadState::THST_Init;
    th->m_callback = nullptr;
    th->m_argument = nullptr;
    th->m_timeout  = p_heartbeat;
    th->m_event    = CreateEvent(nullptr,FALSE,FALSE,nullptr);

    if(th->m_event == nullptr)
    {
      // Didn't work. Destroy thread management and return error
      TP_TRACE0("FAILED to create an event for a threadpool thread\n");
      delete th;
      th = nullptr;
    }
    else
    {
      // Now create our thread
      TP_TRACE0("Creating thread pool thread\n");
      th->m_thread = (HANDLE) _beginthreadex(nullptr,m_stackSize,RunThread,(void*)th,0,&th->m_threadId);

      if(th->m_thread == INVALID_HANDLE_VALUE)
      {
        // Error on thread creation
        TP_TRACE0("FAILED creating threadpool thread\n");
        CloseHandle(th->m_event);
        delete th;
        th = nullptr;
      }
      else
      {
        // Working thread now in the pool
        // So it can be found in the future
        m_threads.push_back(th);
      }
    }
  }
  return th;
}

// Remove a thread from the threadpool
void
ThreadPoolED::RemoveThreadPoolThread(HANDLE p_thread /*=NULL*/)
{
  AutoLockTP lock(&m_critical);

  // If called for a specific thread, find that thread
  ThreadMap::iterator it = m_threads.begin();
  while(it != m_threads.end())
  {
    ThreadRegister* th = *it;
    if(( p_thread && (th->m_thread == p_thread) && (th->m_state == ThreadState::THST_Stopped)) ||
       (!p_thread && (th->m_state == ThreadState::THST_Stopped)))
    {
      TP_TRACE0("Removing thread from threadpool by closing the handle\n");
      // Close our pulsing event;
      CloseHandle(th->m_event);
      // Erase from the threadpool, getting next thread
      it = m_threads.erase(it);
      // Free register memory
      delete th;
      // If only one, than we're done
      if(p_thread)
      {
        return;
      }
    }
    else
    {
      // Go to next thread
      ++it;
    }
  }
}

// Number of waiting threads
// Threadpool must be locked
int   
ThreadPoolED::WaitingThreads()
{
  int waiting = 0;
  for(auto& thread : m_threads)
  {
    if(thread->m_state == ThreadState::THST_Waiting)
    {
      ++waiting;
    }
  }
  return waiting;
}

// Try setting (raising/decreasing) the minimum number of threads
// Can only succeed when not initialized yet
bool
ThreadPoolED::TrySetMinimum(int p_minThreads)
{
  if(!m_initialized)
  {
    m_minThreads = p_minThreads;
    TP_TRACE1("Set new minimum for threadpool: %d\n",p_minThreads);
    return true;
  }
  TP_TRACE0("FAILED: Cannot set threadpool minimum after init\n");
  return false;
}

// Try setting (raising/decreasing) the maximum number of threads
bool
ThreadPoolED::TrySetMaximum(int p_maxThreads)
{
  // Check argument
  // Check the logic of maxThreads
  if(p_maxThreads < NUM_THREADS_DEFAULT)
  {
    p_maxThreads = NUM_THREADS_DEFAULT;
  }
  if(p_maxThreads > (4 * NUM_THREADS_MAXIMUM))
  {
    p_maxThreads = 4 * NUM_THREADS_MAXIMUM;
  }
  // Raising the bar is simple
  if(m_maxThreads < p_maxThreads)
  {
    m_maxThreads = p_maxThreads;
    return true;
  }
  AutoLockTP lock(&m_critical);

  // Only if not so many threads are running.
  if(m_threads.size() <= (unsigned) p_maxThreads)
  {
    m_maxThreads = p_maxThreads;
    TP_TRACE1("Set new maximum for threadpool: %d\n",p_maxThreads);
    return true;
  }
  // Otherwise not possible
  TP_TRACE0("FAILED: Cannot set threadpool maximum after init\n");
  return false;
}

// Intended for long running threads to call, just before entering 
// the WaitForSingleObject API 
void
ThreadPoolED::ExtendMaximumThreads(AutoIncrementPoolMax& p_increment)
{
  if(this == p_increment.m_pool)
  {
    InterlockedIncrement((long*)&m_minThreads);
    InterlockedIncrement((long*)&m_maxThreads);
    TP_TRACE2("Number of minimum/maximum threads extended to: %d/%d\n",m_minThreads,m_maxThreads);
  }
}

void
ThreadPoolED::RestoreMaximumThreads(AutoIncrementPoolMax* p_increment)
{
  if(this == p_increment->m_pool)
  {
    InterlockedDecrement((long*)&m_minThreads);
    InterlockedDecrement((long*)&m_maxThreads);
    TP_TRACE2("Number of minimum/maximum threads decreased to: %d/%d\n",m_minThreads,m_maxThreads);
  }
}

// Setting the stack size
void 
ThreadPoolED::SetStackSize(int p_stackSize)
{
  // Stack cannot be smaller than 1 MB
  if(p_stackSize > (1024 * 1024))
  {
    m_stackSize = p_stackSize;
    TP_TRACE1("Threadpool stack size default set to: %d\n",m_stackSize);
  }
}

void
ThreadPoolED::SetUseCPULoad(bool p_useCpuLoad)
{
  m_useCPULoad = p_useCpuLoad;
}

// Try to shrink the threadpool
void  
ThreadPoolED::ShrinkThreadPool(ThreadRegister* p_reg)
{
  TP_TRACE0("Threadpool shrinking requested\n");
  int waiting = WaitingThreads();
  if(waiting > 1 && m_threads.size() > (unsigned)m_minThreads)
  {
    if(p_reg)
    {
      p_reg->m_state = ThreadState::THST_Stopping;
      TP_TRACE0("1 thread state set to 'stopping'\n");
    }
  }
}

// Running our thread!
/*static*/ unsigned _stdcall
RunThread(void* p_myThread)
{
  // If we come to here, we exist!
  ThreadRegister* reg = reinterpret_cast<ThreadRegister*>(p_myThread);
  return reg->m_pool->RunAThread(reg);
}

unsigned
ThreadPoolED::RunAThread(ThreadRegister* p_register)
{
  // Install SEH to regular exception translator
  _set_se_translator(SeTranslator);

  TP_TRACE0("Run thread main loop\n");
  unsigned exitCode = 0;

  // Change our state from init to waiting-for work
  p_register->m_state = ThreadState::THST_Waiting;
  
  // Enter our endless loop
  while(p_register->m_state != ThreadState::THST_Stopping)
  {
    // Waiting to do something
    DWORD result = WaitForSingleObjectEx(p_register->m_event,p_register->m_timeout,true);

    TP_TRACE0("Waking thread from threadpool\n");
    if(p_register->m_state == ThreadState::THST_Stopping)
    {
      TP_TRACE0("Waked thread was in 'stopping' mode");
      break;
    }

    if(result == WAIT_OBJECT_0 || result == WAIT_TIMEOUT)
    { 
      // Be sure to lock the pool
      LockPool();

      do
      {
        // Who called me?
        if(p_register->m_callback || p_register->m_argument)
        {
          // Copy on the stack for reliability
          LPFN_CALLBACK callback = p_register->m_callback;
          void*         argument = p_register->m_argument;

          // Set to running and run do our job
          p_register->m_state = ThreadState::THST_Running;
          UnlockPool();

          // GO DO OUR BUSINESS!!
          // Callback can be overridden by virtual methods!
          TP_TRACE2("THREAD: Doing the callback [%X:%X]\n",callback,argument);
          DoTheCallback(callback,argument);

          // Re-lock the pool
          LockPool();

          // Return to waiting if we were still running
          if(p_register->m_state == ThreadState::THST_Running)
          {
            p_register->m_state = ThreadState::THST_Waiting;
          }
        }
        else
        {
          TP_TRACE0("INTERNAL ERROR: thread without work!\n");
        }
        // Worker thread should do this
        if(p_register->m_timeout == INFINITE)
        {
          // Empty for next submit of work
          p_register->m_callback = nullptr;
          p_register->m_argument = nullptr;
          // Try to shrink the pool
          ShrinkThreadPool(p_register);
        }
      }
      while(WorkToDo(p_register));
      
      // Release the threadpool
      UnlockPool();
    }
    else if(result == WAIT_IO_COMPLETION)
    {
      // I/O File has been written!! Ignore this state for now!
      TP_TRACE0("IO Completion detected\n");
    }
    else
    {
      // Something went wrong!
      // eg. (result == WAIT_IO_COMPLETION)
      // in case QueueUserAPC forced us out of the wait state
      TP_TRACE0("INTERNAL ERROR: Waked from wait state with status\n");
      p_register->m_state = ThreadState::THST_Stopped;
      exitCode = 3;
      break;
    }
  }

  // Try removing ourselves
  // We are now out-of-business
  TP_TRACE0("Thread to stopped state. To be removed\n");
  p_register->m_state = ThreadState::THST_Stopped;
  RemoveThreadPoolThread(p_register->m_thread);

  return exitCode;
}

// This is the real callback.
// You may overload this member
void
ThreadPoolED::DoTheCallback(LPFN_CALLBACK p_callback,void* p_argument)
{
  (*(p_callback))(p_argument);
}

// Running all cleanup jobs for the threadpool
void 
ThreadPoolED::RunCleanupJobs()
{
  // Simply call all jobs from the main thread
  for(auto& job : m_cleanup)
  {
    TP_TRACE0("Calling cleanup job\n");
    (*job.m_callback)(job.m_argument);
  }
}

// More work to do on a thread
// Pool is/MUST BE already in a locked state
bool 
ThreadPoolED::WorkToDo(ThreadRegister* p_reg)
{
  // Can only be called for a waiting thread or a stopping thread
  if(p_reg->m_state != ThreadState::THST_Waiting && 
     p_reg->m_state != ThreadState::THST_Stopping)
  {
    return false;
  }
  // See if there are items in the work queue
  if(m_work.size() == 0)
  {
    return false;
  }
  // User first arguments in the work queue
  p_reg->m_callback = m_work[0].m_callback;
  p_reg->m_argument = m_work[0].m_argument;
  p_reg->m_timeout  = m_work[0].m_timeout;
  // Remove first element in the work queue
  m_work.pop_front();

  TP_TRACE0("WORK POPPPED from the work queue!\n");

  return true;
}

// Find first waiting thread in the threadpool
// That is NOT waiting for a timeout
ThreadRegister*
ThreadPoolED::FindWaitingThread()
{
  for(auto& threadreg : m_threads)
  {
    if(threadreg->m_state   == ThreadState::THST_Waiting && 
       threadreg->m_timeout == INFINITE)
    {
      return threadreg;
    }
  }
  return nullptr;
}

// Set a thread to do something in the future
BOOL
ThreadPoolED::SubmitWorkToThread(ThreadRegister* p_reg,LPFN_CALLBACK p_callback,void* p_argument,DWORD p_heartbeat)
{
  TP_TRACE2("Submitting work to threadpool [%X:%X]\n",p_callback,p_argument);
  // See if the slot is really free!
  if(p_reg->m_callback || p_reg->m_argument)
  {
    return FALSE;
  }
  // Recall what to do
  p_reg->m_callback = p_callback;
  p_reg->m_argument = p_argument;
  p_reg->m_timeout  = p_heartbeat;

  // If no heartbeat, start work as soon as possible
  if(p_heartbeat == INFINITE)
  {
    // Wake the thread
    TP_TRACE0("Waking the thread\n");
    return SetEvent(p_reg->m_event);
  }
  // Hart beat thread is never started until 'heartbeat-time' is reached
  return TRUE;
}

// Stop a thread for good
void  
ThreadPoolED::StopThread(ThreadRegister* p_reg)
{
  AutoLockTP lock(&m_critical);

  if(p_reg)
  {
    ThreadState state = p_reg->m_state;
    if(state == ThreadState::THST_Running || state == ThreadState::THST_Waiting)
    {
      TP_TRACE0("Stopping thread (via state/event)\n");
      p_reg->m_state = ThreadState::THST_Stopping;
    }
    if(state == ThreadState::THST_Waiting)
    {
      SetEvent(p_reg->m_event);
    }
  }
}

// Stop a heartbeat thread
void 
ThreadPoolED::StopHeartbeat(LPFN_CALLBACK p_callback)
{
  AutoLockTP lock(&m_critical);

  for(auto& reg : m_threads)
  {
    if(reg->m_callback == p_callback && reg->m_timeout != INFINITE)
    {
      // Can only stop 1 thread at a time!
      TP_TRACE0("Found heartbeat thread to stop\n");
      StopThread(reg);
      return;      
    }
  }
}

// OUR PRIMARY FUNCTION
// TRY TO GET SOME WORK DONE
void
ThreadPoolED::SubmitWork(LPFN_CALLBACK p_callback,void* p_argument,DWORD p_heartbeat /*=INFINITE*/)
{
  ThreadRegister* reg = nullptr;
  // Lock the pool
  AutoLockTP lock(&m_critical);

  TP_TRACE0("Submit work\n");
  // See if we are initialized
  InitThreadPool();

  // Check that we are open for work
  if(m_openForWork == false)
  {
    TP_TRACE0("INTERNAL ERROR: Threadpool not open for work. Program in closing mode.\n");
    return;
  }

  // Special case: try to create heartbeat thread
  if(p_heartbeat != INFINITE)
  {
    TP_TRACE0("Create a heartbeat thread\n");
    reg = CreateThreadPoolThread(p_heartbeat);
    if(reg)
    {
      if(SubmitWorkToThread(reg,p_callback,p_argument,p_heartbeat))
      {
        return;
      }
    }
  }

  // Find a waiting thread
  reg = FindWaitingThread();
  if(reg)
  {
    TP_TRACE0("Found a waiting thread. Submitting.\n");
    if(SubmitWorkToThread(reg,p_callback,p_argument,p_heartbeat))
    {
      return;
    }
  }

  // Find CPU load in case we need it
  float load = 0.0;
  if(m_useCPULoad)
  {
    GetCPULoad();
    TP_TRACE1("CPU Load: %f\n",load);
  }

  // If running thread < maximum, create a thread
  // But only if the CPU load is below 90 percent and throttling is 'on'
  if((m_threads.size() < (unsigned)m_maxThreads) && (!m_useCPULoad || (load < 0.9)))
  {
    TP_TRACE0("Create new thread to submit to.\n");
    reg = CreateThreadPoolThread();
    if(reg)
    {
      if(SubmitWorkToThread(reg,p_callback,p_argument,p_heartbeat))
      {
        return;
      }
    }
  }

  // Overstressed: Queue the work for later use
  TP_TRACE1("Thread pool overstressed [%d]: queueing for later processing\n",m_threads.size());
  ThreadWork work;
  work.m_callback = p_callback;
  work.m_argument = p_argument;
  work.m_timeout  = p_heartbeat;
  m_work.push_back(work);
  TP_TRACE1("Work queue now [%d] items\n",m_work.size());
}

// Submitting cleanup jobs
void 
ThreadPoolED::SubmitCleanup(LPFN_CALLBACK p_cleanup,void* p_argument)
{
  ThreadWork job;
  job.m_callback = p_cleanup;
  job.m_argument = p_argument;
  job.m_timeout  = INFINITE;
  m_cleanup.push_back(job);
  TP_TRACE1("Cleanup jobs queue [%d] items\n",m_cleanup.size());
}

// Sleeping and waking-up a thread
// Use an application global unique number!!
void*
ThreadPoolED::SleepThread(DWORD_PTR p_unique,void* p_payload)
{
  bool found = false;
  ThreadRegister* reg = nullptr;

  // Only locking the pool for the duration of the search
  {  AutoLockTP lock(&m_critical);

    for(auto& thread : m_threads)
    {
      if(thread->m_threadId == GetCurrentThreadId())
      {
        // Arguments of this thread already spent!
        thread->m_callback = (LPFN_CALLBACK)p_unique;
        thread->m_argument = p_payload;
        found = true;
        reg   = thread;
        break;
      }
    }
  }
  // See if we can safely go sleeping on this thread
  if(!found || reg == nullptr || reg->m_state != ThreadState::THST_Running)
  {
    // Nope: thread not found
    TP_TRACE0("ALARM: Thread to put to sleep NOT found in the threadpool.");
    return nullptr;
  }

  // For the duration of this call, increment the number of max threads
  AutoIncrementPoolMax increment(this);

  // Putting the thread to sleep
  DWORD result = WaitForSingleObjectEx(reg->m_event,reg->m_timeout,true);

  if(result == WAIT_OBJECT_0      || 
     result == WAIT_IO_COMPLETION ||
     result == WAIT_TIMEOUT        )
  {
    // See if we must eliminate ourselves
    if(reg->m_state == ThreadState::THST_Stopped && reg->m_callback == nullptr)
    {
      // First: remove from registry, then end it
      RemoveThreadPoolThread(reg->m_thread);
      _endthreadex(3);  // Never returns from this function
      return nullptr;   // Never comes to here
    }
    // Regular return
    return reg->m_callback;
  }
  // Wait failed, other errors
  return nullptr;
}

// Wake up the thread by pulsing it's event
bool  
ThreadPoolED::WakeUpThread(DWORD_PTR p_unique,void* p_result /*=nullptr*/)
{
  AutoLockTP lock(&m_critical);

  for(auto& reg : m_threads)
  {
    if(reg->m_callback == (LPFN_CALLBACK)p_unique)
    {
      if(p_result)
      {
        reg->m_callback = (LPFN_CALLBACK)p_result;
      }
      // Wakup thread from WaitForSingleObjectEx
      SetEvent(reg->m_event);
      return true;
    }
  }
  return false;
}

// Getting the payload
// The intention is to leave an answer here for the waiting thread!
void*
ThreadPoolED::GetSleepingThreadPayload(DWORD_PTR p_unique)
{
  AutoLockTP lock(&m_critical);

  for(auto& reg : m_threads)
  {
    if(reg->m_callback == (LPFN_CALLBACK)p_unique)
    {
      return reg->m_argument;
    }
  }
  return nullptr;
}

// Eliminate the sleeping thread.
// REALLY: SHOULD YOU CALL THIS??
void
ThreadPoolED::EliminateSleepingThread(DWORD_PTR p_unique)
{
  ThreadRegister* thread = nullptr;
  { AutoLockTP lock(&m_critical);

    for(auto& reg : m_threads)
    {
      if(reg->m_callback == (LPFN_CALLBACK)p_unique)
      {
        thread = reg;
        break;
      }
    }
  }
  // Sleeping thread not found. Do nothing.
  if(thread == nullptr)
  {
    return;
  }

  // Mark the fact that we should terminate
  HANDLE th = thread->m_thread;
  thread->m_state    = ThreadState::THST_Stopped;
  thread->m_callback = nullptr;

  // Pulse the thread and wait for it to die
  SetEvent(thread->m_event);
  WaitForSingleObject(th,INFINITE);
}

