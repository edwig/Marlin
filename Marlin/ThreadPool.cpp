/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ThreadPool.cpp
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
#include "StdAfx.h"
#include "ThreadPool.h"
#include "CPULoad.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
static unsigned _stdcall RunHartbeat(void* p_pool);

//////////////////////////////////////////////////////////////////////////
//
// The Threadpool
//
ThreadPool::ThreadPool()
{
  InitializeCriticalSection(&m_critical);
}

ThreadPool::ThreadPool(int p_minThreads,int p_maxThreads)
           :m_minThreads(p_minThreads)
           ,m_maxThreads(p_maxThreads)
{
  InitializeCriticalSection(&m_critical);
}

ThreadPool::~ThreadPool()
{
  StopThreadPool();
  RunCleanupJobs();
  DeleteCriticalSection(&m_critical);
}

// Pool locking from a single thread state change
void 
ThreadPool::LockPool()
{
  EnterCriticalSection(&m_critical);
}

void 
ThreadPool::UnlockPool()
{
  LeaveCriticalSection(&m_critical);
}

// Initialize our new threadpool
void
ThreadPool::InitThreadPool()
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

  // Getting the number of logical processors on the system
  SYSTEM_INFO info;
  GetNativeSystemInfo(&info);
  m_processors = info.dwNumberOfProcessors;

  // Create IO Completion Port
  // Must be done before creating the threads!
  m_completion = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,NULL,0);

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
ThreadPool::StopThreadPool()
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
    for (long ind = 0; ind < m_curThreads; ++ind)
    {
      StopThread(0);
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
ThreadPool::CreateThreadPoolThread()
{
  AutoLockTP lock(&m_critical);

  ThreadRegister* th = new ThreadRegister();
  if(th)
  {
    // Connect to the pool
    th->m_pool = this;

    // Now create our thread
    TP_TRACE0("Creating thread pool thread\n");
    th->m_thread = (HANDLE) _beginthreadex(nullptr,m_stackSize,RunThread,(void*)th,0,&th->m_threadId);

    if(th->m_thread == INVALID_HANDLE_VALUE)
    {
      // Error on thread creation
      TP_TRACE0("FAILED creating threadpool thread\n");
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
  return th;
}

// Remove a thread from the threadpool
void
ThreadPool::RemoveThreadPoolThread(unsigned p_threadID)
{
  AutoLockTP lock(&m_critical);

  // If called for a specific thread, find that thread
  ThreadMap::iterator it = m_threads.begin();
  while(it != m_threads.end())
  {
    ThreadRegister* th = *it;

    if(th->m_threadId == p_threadID)
    {
      TP_TRACE0("Removing thread from threadpool by closing the handle\n");
      // Close our thread handle
      CloseHandle(th->m_thread);
      // Erase from the threadpool, getting next thread
      it = m_threads.erase(it);
      // Free register memory
      delete th;
      return;
    }
    // Go to next thread
    ++it;
  }
}

// Try setting (raising/decreasing) the minimum number of threads
// Can only succeed when not initialized yet
bool
ThreadPool::TrySetMinimum(int p_minThreads)
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
ThreadPool::TrySetMaximum(int p_maxThreads)
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
ThreadPool::ExtendMaximumThreads(AutoIncrementPoolMax& p_increment)
{
  if(this == p_increment.m_pool)
  {
    InterlockedIncrement((long*)&m_minThreads);
    InterlockedIncrement((long*)&m_maxThreads);
    TP_TRACE2("Number of minimum/maximum threads extended to: %d/%d\n",m_minThreads,m_maxThreads);
  }
}

void
ThreadPool::RestoreMaximumThreads(AutoIncrementPoolMax* p_increment)
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
ThreadPool::SetStackSize(int p_stackSize)
{
  // Stack cannot be smaller than 1 MB
  if(p_stackSize < (1024 * 1024))
  {
    p_stackSize = 1024 * 1024;
  }
  m_stackSize = p_stackSize;
  TP_TRACE1("Threadpool stack size default set to: %d\n",m_stackSize);
}

// Running our thread!
/*static*/ unsigned _stdcall
RunThread(void* p_myThread)
{
  // If we come to here, we exist!
  ThreadRegister* reg = (ThreadRegister*) p_myThread;
  return reg->m_pool->RunAThread(reg);
}

DWORD
ThreadPool::RunAThread(ThreadRegister* /*p_register*/)
{
  bool stayInThePool = true;

  TP_TRACE0("Thread is entering the pool\n");
  InterlockedIncrement(&m_curThreads);
  InterlockedIncrement(&m_bsyThreads);

  do 
  {
    DWORD     bytes = 0;
    DWORD     error = 0;
    ULONG_PTR key   = 0;
    float     load  = 0.0;
    LPOVERLAPPED overlapped = nullptr;

    // Stops executing and wait in I/O completion port
    InterlockedDecrement(&m_bsyThreads);
    BOOL ok = GetQueuedCompletionStatus(m_completion,&bytes,&key,&overlapped,INFINITE);
    error = GetLastError();
    // Start executing again
    InterlockedIncrement(&m_bsyThreads);

    // Special case: stop a thread
    if(key == COMPLETION_STOP)
    {
      break;
    }

    // Should we add another thread to the pool?
    if((m_bsyThreads == m_curThreads) &&
       (m_bsyThreads  < m_maxThreads) &&
       (GetCPULoad()  < 0.75))
    {
      CreateThreadPoolThread();
    }

    // Timeout from the completion port?
    if(!ok && (error == WAIT_TIMEOUT))
    {
      // Thread timed out. Not much for the application to do
      // thread can stop, even if it has some outstanding I/O
      stayInThePool = false;
    }

    // PROCESSING A ACTION IN THE THREADPOOL
    if(ok || overlapped)
    {
      if(key == COMPLETION_WORK && overlapped == INVALID_HANDLE_VALUE)
      {
        // Thread woke to do some interesting work....
        LPFN_CALLBACK callback = nullptr;
        void*         payload  = nullptr;
        if(WorkToDo(callback,payload))
        {
          DoTheCallback(callback,payload);
        }
      }
      else if (key == COMPLETION_CALL)
      {
        // Implement your overload of this special call
        DoTheCallback(overlapped);
      }
      else
      {
        // The completion key **IS** the callback mechanism
        LPFN_CALLBACK callback = (LPFN_CALLBACK)key;
        (*callback)(overlapped);
      }
    }

    // Find CPU load and see if we must remain in the threadpool
    load = GetCPULoad();
    TP_TRACE1("CPU Load: %f\n",load);
    if((load > 0.9) && (m_curThreads > m_minThreads))
    {
      stayInThePool = false;
    }
  } 
  while(stayInThePool);

  // Leaving the main loop
  TP_TRACE0("Thread is leaving the pool\n");
  InterlockedDecrement(&m_bsyThreads);
  InterlockedDecrement(&m_curThreads);

  // Try removing ourselves
  // We are now out-of-business
  TP_TRACE0("Thread about to exit. To be removed\n");
  RemoveThreadPoolThread(GetCurrentThreadId());

  return 0;
}

// This is the real callback.
// You may overload this member
void
ThreadPool::DoTheCallback(LPFN_CALLBACK p_callback,void* p_argument)
{
  (*(p_callback))(p_argument);
}

void
ThreadPool::DoTheCallback(void* p_argument)
{
  // NOTHING HERE: OVERLOAD THIS MEMBER
  UNREFERENCED_PARAMETER(p_argument);
}

// Running all cleanup jobs for the threadpool
void 
ThreadPool::RunCleanupJobs()
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
ThreadPool::WorkToDo(LPFN_CALLBACK& p_callback,void*& p_argument)
{
  AutoLockTP lock(&m_critical);

  // See if there are items in the work queue
  if(m_work.size() == 0)
  {
    return false;
  }
  // User first arguments in the work queue
  p_callback = m_work[0].m_callback;
  p_argument = m_work[0].m_argument;
  // Remove first element in the work queue
  m_work.pop_front();

  TP_TRACE0("WORK POPPPED from the work queue!\n");

  return true;
}

// Stop a thread for good
// Stop the last thread in the pool. Does **NOT** stop the exact thread (anymore)
// Now only relevant for stopping ALL jobs at the end of the lifetime
void  
ThreadPool::StopThread(ThreadRegister* /*p_reg*/)
{
  if(!PostQueuedCompletionStatus(m_completion,0,COMPLETION_STOP,(LPOVERLAPPED)INVALID_HANDLE_VALUE))
  {
    TP_TRACE0("Cannot post a STOP command to the threadpool queue.\n");
  }
}


// Create a hartbeat thread (Can be called **ONCE**)
bool
ThreadPool::CreateHartbeat(LPFN_CALLBACK p_callback, void* p_argument, DWORD p_hartbeat)
{
  // See if a hartbeat was already running
  if(m_hartbeat)
  {
    return false;
  }
  m_hartbeatCallback = p_callback;
  m_hartbeatPayload  = p_argument;
  m_hartbeat         = p_hartbeat;
  m_hartbeatEvent    = CreateEvent(NULL,FALSE,FALSE,NULL);

  if(m_hartbeatEvent)
  {
    HANDLE thread = (HANDLE)_beginthreadex(nullptr,m_stackSize,::RunHartbeat,(void*)this,0,NULL);
    if(thread)
    {
      TP_TRACE0("Created a hartbeat thread!\n");
      CloseHandle(thread);
      return true;
    }
  }
  return false;
}

// Go running our hartbeat
/*static */unsigned _stdcall RunHartbeat(void* p_pool)
{
  ThreadPool* pool = reinterpret_cast<ThreadPool*>(p_pool);
  return pool->RunHartbeat();
}

// Running the hartbeat thread
DWORD
ThreadPool::RunHartbeat()
{
  bool running = true;
  do 
  {
    TP_TRACE0("Hartbeat thread sleeping after hartbeat\n");
    DWORD result = WaitForSingleObject(m_hartbeatEvent,m_hartbeat);
    switch(result)
    {
      case WAIT_ABANDONED:  // Fall through
      case WAIT_FAILED:     // ERROR
                            TP_TRACE0("Failed on hartbeat thread\n");
                            break;
      case WAIT_OBJECT_0:   // Called to stop
                            running = false;
                            break;
      case WAIT_TIMEOUT:    // Hartbeat! Do the call
                            TP_TRACE0("Hartbeat waking up\n");
                            (*m_hartbeatCallback)(m_hartbeatPayload);
                            break;
    }
  } 
  while(running);

  TP_TRACE0("Hartbeat thread stopping\n");
  // At the end of the hartbeat
  CloseHandle(m_hartbeatEvent);
  m_hartbeatEvent    = nullptr;
  m_hartbeatCallback = nullptr;
  m_hartbeatPayload  = nullptr;
  // Create hartbeat can be called again!
  m_hartbeat = 0;

  return 0;
}


// Stop a heartbeat thread
void 
ThreadPool::StopHartbeat()
{
  if(m_hartbeatEvent)
  {
    TP_TRACE0("Stopping the hartbeat externally\n");
    SetEvent(m_hartbeatEvent);
  }
}

// OUR PRIMARY FUNCTION
// TRY TO GET SOME WORK DONE
void
ThreadPool::SubmitWork(LPFN_CALLBACK p_callback,void* p_argument)
{
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

  // Queue the work for later use
  ThreadWork work;
  work.m_callback = p_callback;
  work.m_argument = p_argument;
  m_work.push_back(work);
  TP_TRACE1("Queueing 1 job. Work queue now [%d] items\n",m_work.size());

  // Post to free 1 thread from the pool
  if(!PostQueuedCompletionStatus(m_completion,0,COMPLETION_WORK,(LPOVERLAPPED)INVALID_HANDLE_VALUE))
  {
    TP_TRACE0("Posting of I/O Completion failed\n");
  }
}

// Submitting cleanup jobs
void 
ThreadPool::SubmitCleanup(LPFN_CALLBACK p_cleanup,void* p_argument)
{
  ThreadWork job;
  job.m_callback = p_cleanup;
  job.m_argument = p_argument;
  m_cleanup.push_back(job);
  TP_TRACE1("Cleanup jobs queue [%d] items\n",m_cleanup.size());
}

// Sleeping and waking-up a thread
// Use an application global unique number!!
void*
ThreadPool::SleepThread(DWORD_PTR p_unique,void* p_payload)
{
  SleepingThread* sleep = new SleepingThread();
  sleep->m_threadId = GetCurrentThreadId();
  sleep->m_unique   = p_unique;
  sleep->m_payload  = p_payload;
  sleep->m_event    = CreateEvent(NULL,FALSE,FALSE,NULL);
  sleep->m_abort    = false;

  // Add sleeping thread info the queue
  { AutoLockTP lock(&m_critical);
    m_sleeping.push_back(sleep);
  }

  // Go to sleep
  DWORD result = WaitForSingleObject(sleep->m_event,INFINITE);
  
  // Wakey, wakey
  switch (result)
  {
    case WAIT_ABANDONED:  TP_TRACE0("Thread abandoned in sleep\n");       break;
    case WAIT_OBJECT_0:   TP_TRACE0("Correct waking of the thread\n");    break;
    case WAIT_TIMEOUT:    TP_TRACE0("Timeout on a non-timeout thread\n"); break;
    case WAIT_FAILED:     TP_TRACE0("Failed to fall asleep!!\n");         break;
  }

  // Find and remove the sleeping thread info (in a locking block!)
  { AutoLockTP lock(&m_critical);

    SleepingMap::iterator it;
    for(it = m_sleeping.begin();it != m_sleeping.end(); ++it)
    {
      SleepingThread* thread = *it;
      if(thread->m_unique == p_unique)
      {
        void* payload = thread->m_payload;
        bool  abort   = thread->m_abort;
        CloseHandle(thread->m_event);
        delete thread;
        m_sleeping.erase(it);

        // See if we must abort
        if(abort)
        {
          _endthread();
          return nullptr;
        }
        return payload;
      }
    }
  }

  // Strange: sleeping thread info not found
  return nullptr;
}

// Wake up the thread by pulsing it's event
// The intention is to leave an answer here for the waiting thread!
bool
ThreadPool::WakeUpThread(DWORD_PTR p_unique,void* p_result /*=nullptr*/)
{
  AutoLockTP lock(&m_critical);

  for(auto& sleep : m_sleeping)
  {
    if(sleep->m_unique == p_unique)
    {
      // Set payload result and wake the thread
      sleep->m_payload = p_result;
      SetEvent(sleep->m_event);
      return true;
    }
  }
  return false;
}

// Getting the payload
// The intention is to leave an answer here for the waiting thread!
void*
ThreadPool::GetSleepingThreadPayload(DWORD_PTR p_unique)
{
  AutoLockTP lock(&m_critical);

  for(auto& sleep : m_sleeping)
  {
    if(sleep->m_unique == p_unique)
    {
      return sleep->m_payload;
    }
  }
  return nullptr;
}

bool  
ThreadPool::EliminateSleepingThread(DWORD_PTR p_unique)
{
  AutoLockTP lock(&m_critical);

  SleepingMap::iterator it;
  for(it = m_sleeping.begin();it != m_sleeping.end(); ++it)
  {
    SleepingThread* sleep = *it;
    if(sleep->m_unique == p_unique)
    {
      // Set the thread abort code
      sleep->m_abort = true;
      // And wake the sleeping thread
      SetEvent(sleep->m_event);
      return true;
    }
  }
  return false;
}