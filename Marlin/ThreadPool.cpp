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
static unsigned _stdcall RunHeartbeat(void* p_pool);

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
  DeleteCriticalSection(&m_critical);
}

// Shutdown mode simply sets the code that
// allows work to enter the threadpool to false
void
ThreadPool::Shutdown()
{
  m_openForWork = false;
}

// Running the threadpool
void 
ThreadPool::Run()
{
  InitThreadPool();
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

  // Adjust maximum of threads for the number of processors
  if(m_maxThreads > 2 * m_processors)
  {
    m_maxThreads = 2 * m_processors;
  }

  // Create IO Completion Port
  // Must be done before creating the threads!
  // But could already have been done by association of an I/O handle
  if(!m_completion)
  {
    m_completion = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,NULL,0);
  }

  // Create our minimum threads
  for(int ind = 0; ind < m_minThreads; ++ind)
  {
    CreateThreadPoolThread();
  }
  // Open for business
  m_initialized = true;
  m_openForWork = true;
}

// Create a thread in the threadpool
ThreadRegister*
ThreadPool::CreateThreadPoolThread()
{
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
      AutoLockTP lock(&m_critical);
      m_threads.push_back(th);
    }
  }
  return th;
}

// Remove a thread from the threadpool
void
ThreadPool::RemoveThreadPoolThread(unsigned p_threadID)
{
  EnterCriticalSection(&m_critical);

  // If called for a specific thread, find that thread
  ThreadMap::iterator it = m_threads.begin();
  while(it != m_threads.end())
  {
    ThreadRegister* th = *it;

    if(th->m_threadId == p_threadID)
    {
      TP_TRACE0("Removing thread from threadpool by closing the handle\n");
      // Erase from the threadpool, getting next thread
      it = m_threads.erase(it);
      // Free register memory
      delete th;

      // Free the lock
      // After the close we cannot leave the critical section any more
      // So we CANNOT use the AutoLockTP on the stack!!
      LeaveCriticalSection(&m_critical);
      return;
    }
    // Go to next thread
    ++it;
  }
  // Not found: still leave the critical section
  LeaveCriticalSection(&m_critical);
}

// Does the thread belong to our threadpool?
bool 
ThreadPool::IsThreadInThreadPool(unsigned p_threadID)
{
  AutoLockTP lock(&m_critical);

  for(auto& thread : m_threads)
  {
    if(thread->m_threadId == p_threadID)
    {
      return true;
    }
  }
  return false;
}

// Try setting (raising/decreasing) the minimum number of threads
// Can only succeed when not initialized yet
bool
ThreadPool::TrySetMinimum(int p_minThreads)
{
  if(!m_initialized)
  {
    if(p_minThreads < NUM_THREADS_MINIMUM)
    {
      p_minThreads = NUM_THREADS_MINIMUM;
    }
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
ThreadPool::RestoreMaximumThreads(AutoIncrementPoolMax& p_increment)
{
  if(this == p_increment.m_pool)
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

// Associate I/O handle with the completion port
DWORD
ThreadPool::AssociateIOHandle(HANDLE p_handle,ULONG_PTR p_key)
{
  DWORD error = 0;

  m_completion = CreateIoCompletionPort(p_handle,m_completion,p_key,0);
  if(!m_completion)
  {
    error = GetLastError();
    TP_TRACE1("Threadpool cannot register file handle for asynchroneous I/O completion. Error: %d",error);
  }

  SetFileCompletionNotificationModes(p_handle,FILE_SKIP_SET_EVENT_ON_HANDLE);

  return error;
}

// Setting the thread initialization function
bool
ThreadPool::SetThreadInitFunction(LPFN_CALLBACK p_init,LPFN_CALLBACK p_abort,void* p_argument)
{
  if(m_initialization == nullptr)
  {
    m_initialization = p_init;
    m_abortfunction  = p_abort;
    m_initParameter  = p_argument;
    return true;
  }
  return false;
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

  // Check that there is a initialization routine
  if(m_initialization)
  {
    TP_TRACE0("Running thread initialization routine\n");
    (*m_initialization)(m_initParameter);
  }

  // MAIN LOOP
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

    if(!ok && error == ERROR_OPERATION_ABORTED)
    {
      // Asynchronous I/O was aborted (e.g: file handle was closed)
      // Call the abort function, so the caller can clean up the mess
      if(m_abortfunction)
      {
        (*m_abortfunction)(overlapped);
      }
      break;
    }

    // Or a special case: WE stop a thread
    if(key == COMPLETION_STOP)
    {
      break;
    }

    // Should we add another thread to the pool?
    if((m_bsyThreads == m_curThreads) &&
       (m_bsyThreads  < m_maxThreads) &&
       (GetCPULoad()  < 0.75) &&
       m_openForWork)
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
  RemoveThreadPoolThread(GetCurrentThreadId());

  TP_TRACE0("Thread about to exit. To be removed\n");
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
  m_cleanup.clear();
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
  // See if a heartbeat was already running
  if(m_heartbeat)
  {
    return false;
  }
  m_heartbeatCallback = p_callback;
  m_heartbeatPayload  = p_argument;
  m_heartbeat         = p_hartbeat;
  m_heartbeatEvent    = CreateEvent(NULL,FALSE,FALSE,NULL);

  if(m_heartbeatEvent)
  {
    HANDLE thread = (HANDLE)_beginthreadex(nullptr,m_stackSize,::RunHeartbeat,(void*)this,0,NULL);
    if(thread)
    {
      TP_TRACE0("Created a hartbeat thread!\n");
      CloseHandle(thread);
      return true;
    }
  }
  return false;
}

// Go running our heartbeat
/*static */unsigned _stdcall RunHeartbeat(void* p_pool)
{
  ThreadPool* pool = reinterpret_cast<ThreadPool*>(p_pool);
  return pool->RunHeartbeat();
}

// Running the heartbeat thread
DWORD
ThreadPool::RunHeartbeat()
{
  bool running = true;
  do 
  {
    TP_TRACE0("Hartbeat thread sleeping after hartbeat\n");
    DWORD result = WaitForSingleObject(m_heartbeatEvent,m_heartbeat);
    switch(result)
    {
      case WAIT_ABANDONED:  // Fall through
      case WAIT_FAILED:     // ERROR
                            TP_TRACE0("Failed on hartbeat thread\n");
                            break;
      case WAIT_OBJECT_0:   // Called to stop
                            running = false;
                            break;
      case WAIT_TIMEOUT:    // Hart beat! Do the call
                            TP_TRACE0("Hartbeat waking up\n");
                            (*m_heartbeatCallback)(m_heartbeatPayload);
                            break;
    }
  } 
  while(running);

  TP_TRACE0("Hartbeat thread stopping\n");
  // At the end of the heartbeat
  CloseHandle(m_heartbeatEvent);
  m_heartbeatEvent    = nullptr;
  m_heartbeatCallback = nullptr;
  m_heartbeatPayload  = nullptr;
  // Create heartbeat can be called again!
  m_heartbeat = 0;

  return 0;
}

// Stop a heartbeat thread
void 
ThreadPool::StopHeartbeat()
{
  if(m_heartbeatEvent)
  {
    TP_TRACE0("Stopping the hartbeat externally\n");
    SetEvent(m_heartbeatEvent);
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

//////////////////////////////////////////////////////////////////////////
//
// SLEEPING THREADS
//
//////////////////////////////////////////////////////////////////////////

// Sleeping and waking-up a thread
// Use an application global unique number!!
void*
ThreadPool::SleepThread(DWORD_PTR p_unique,void* p_payload)
{
  // See if we are initialized
  InitThreadPool();

  // Check that we are open for work
  if(m_openForWork == false)
  {
    TP_TRACE0("INTERNAL ERROR: Threadpool not open for work. Program in closing mode.\n");
    return nullptr;
  }

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
  
  // Time to wake up again
  switch (result)
  {
    case WAIT_ABANDONED:  TP_TRACE0("Thread abandoned in sleep\n");       break;
    case WAIT_OBJECT_0:   TP_TRACE0("Correct waking of the thread\n");    break;
    case WAIT_TIMEOUT:    TP_TRACE0("Timeout on a non-timeout thread\n"); break;
    case WAIT_FAILED:     TP_TRACE0("Failed to fall asleep!!\n");         break;
  }

  SleepingThread* sleeper = nullptr;

  // Find and remove the sleeping thread info (in a locking block!)
  { AutoLockTP lock(&m_critical);

    SleepingMap::iterator it;
    for(it = m_sleeping.begin();it != m_sleeping.end(); ++it)
    {
      sleeper = *it;
      if(sleeper->m_unique == p_unique)
      {
        // Done with the sleeping thread info
        bool  abort   = sleeper->m_abort;
        void* payload = sleeper->m_payload;
        CloseHandle(sleeper->m_event);
        delete sleeper;
    
        // Remove from the queue
        m_sleeping.erase(it);

        // See if we must abort
        if(abort)
        {
          break;
        }
        return payload;
      }
    }
  }

  // If we found a sleeper. Terminate it for the ABORT action
  // This must be done OUTSIDE the locking section
  // Otherwise we never release the lock and block the pool
  if(sleeper)
  {
    DWORD id = GetCurrentThreadId();
    if(IsThreadInThreadPool(id))
    {
      InterlockedDecrement(&m_curThreads);
      InterlockedDecrement(&m_bsyThreads);
      RemoveThreadPoolThread(id);
    }
    // Now do the opposite of _beginthread
    _endthread();
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

// REALLY? SHOULD YOU. THIS WILL LEEK AN OVERLAPPED STRUCTURE
// AND EVERYTHING THAT HINGES ON IT!
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

// Wake up all sleeping threads as part of the shutdown
void 
ThreadPool::WakeUpAllSleepers()
{
  std::vector<DWORD_PTR> map;

  // Copy the map within a lock section
  {
    AutoLockTP lock(&m_critical);
    for(auto& sleep : m_sleeping)
    {
      map.push_back(sleep->m_unique);
    }
  }

  if(!map.empty())
  {
    // Wake up all sleeping threads
    TP_TRACE1("Waking up %d sleeping threads\n",map.size());
    for(auto& unique : map)
    {
      WakeUpThread(unique);
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//
// STOPPING AND TEARING DOWN THE THREADPOOL
//
//////////////////////////////////////////////////////////////////////////

#define WF_IDLE_SLEEP   1
#define WF_IDLE_HEART   2
#define WF_IDLE_CLEAN   3
#define WF_IDLE_WORK    4
#define WF_IDLE_THREADS 5

bool
ThreadPool::WaitingForIdle(int p_part)
{
  // Wait for the queue to become idle
  bool idle = false;
  unsigned waitTime = 50;
  // wait 50,100,200,400,800,1600,3200,6400 milliseconds
  for(unsigned ind = 0;ind < 8; ++ind)
  {
    // Wait period of time
    Sleep(waitTime);
    waitTime *= 2;
    // See if threadpool has become idle
    { AutoLockTP lock(&m_critical);
      switch(p_part)
      {
        case WF_IDLE_SLEEP:   if(m_sleeping.empty())idle = true;
                              break;
        case WF_IDLE_HEART:   if(m_heartbeat == 0)  idle = true;
                              break;
        case WF_IDLE_CLEAN:   if(m_cleanup.empty()) idle = true;
                              break;
        case WF_IDLE_WORK:    if(m_work.empty())    idle = true;
                              break;
        case WF_IDLE_THREADS: if(m_threads.empty()) idle = true;
                              break;
      }
    }
    if(idle)
    {
      break;
    }
  }
  return idle;
}

// Stopping the thread pool
void
ThreadPool::StopThreadPool()
{
  TP_TRACE0("Stopping threadpool\n");
  // Not open for work, No more SubmitWork or SleepThread can occur
  m_openForWork = false;

  // Try to wake all sleeping threads
  WakeUpAllSleepers();
  WaitingForIdle(WF_IDLE_SLEEP);

  // Stop the heartbeat (if any)
  StopHeartbeat();
  WaitingForIdle(WF_IDLE_HEART);

  // Run the cleanup jobs first (if any)
  RunCleanupJobs();
  WaitingForIdle(WF_IDLE_CLEAN);

  // Waiting for the work queue to drain
  // Waiting for max of 30 seconds
  WaitingForIdle(WF_IDLE_WORK);

  // Try stopping all threads, signaling to remove
  int number = (int)m_threads.size() + m_curThreads;
  for(int num = 0;num < number;++num)
  {
    TP_TRACE1("Stopping thread: %d\n",ind);
    StopThread(0);
  }
  TP_TRACE1("Total items of work still in the work-queue: %d\n",m_work.size());

  // Closing our completion port,
  // forcing threads out of the wait state (if any left)
  CloseHandle(m_completion);
  m_completion = nullptr;

  // Wait for the queue to become idle
  bool idle = WaitingForIdle(WF_IDLE_THREADS);

  // No longer initialize after this point
  AutoLockTP lock(&m_critical);
  m_initialized = false;

  // Force the queue to a halt!!
  // Tear those threads out of the sky
  // Do not complain about "TerminateThread". 
  // It is the only way. We know!
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

