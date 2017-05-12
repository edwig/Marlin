/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ThreadPool.h
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
#pragma once
#include <vector>
#include <deque>

// Define this macro to debug the threadpool
// #define DEBUG_THREADPOOL  1

// Threads minimum and maximum
constexpr auto NUM_THREADS_MINIMUM =   4;   // No use for a threadpool below this number
constexpr auto NUM_THREADS_DEFAULT =  10;   // Default max threads
constexpr auto NUM_THREADS_MAXIMUM = 120;   // More than this is not wise under Windows-OS
                                            // Theoretically on a hexacore machine with hyperthreading

// Standard stack size of a thread in 64 bits architectures
constexpr auto THREAD_STACKSIZE = (2 * 1024 * 1024);

// Callbacks
typedef void (* LPFN_CALLBACK)(void *);

// Forward declaration of our threadpool
class ThreadPool;
class AutoIncrementPoolMax;

#define COMPLETION_WORK   1
#define COMPLETION_CALL   2
#define COMPLETION_STOP   3

// Registration of a waiting/running thread
class ThreadRegister
{
public:
    ThreadPool*   m_pool;
    HANDLE        m_thread;
    unsigned      m_threadId;
};

using ThreadMap = std::vector<ThreadRegister*>;

// Registratio of sleeping threads
class SleepingThread
{
public:
  unsigned  m_threadId;
  DWORD_PTR m_unique;
  void*     m_payload;
  HANDLE    m_event;
  bool      m_abort;
};

using SleepingMap = std::vector<SleepingThread*>;

// The queue of work items, when all threads are busy
class ThreadWork
{
public:
  LPFN_CALLBACK m_callback;
  void*         m_argument;
};

// Queue of work items still to process
using WorkMap = std::deque<ThreadWork>;

class ThreadPool
{
public:
  ThreadPool();
  ThreadPool(int p_minThreads,int p_maxThreads);
 ~ThreadPool();

  // OUR PRIMARY FUNCTION!

  // Submit an item, starting a thread on it
  void SubmitWork(LPFN_CALLBACK p_callback,void* p_argument);
  // Submitting cleanup jobs. Runs when threadpool stops
  void SubmitCleanup(LPFN_CALLBACK p_cleanup,void* p_argument);

  // Try setting (raising/decreasing) the minimum number of threads
  bool TrySetMinimum(int p_minThreads);
  // Try setting (raising/decreasing) the maximum number of threads
  bool TrySetMaximum(int p_maxThreads);
  // Setting the stack size
  void SetStackSize(int p_stackSize);
  // Extend the maximum for a period of time
  void ExtendMaximumThreads (AutoIncrementPoolMax& p_increment);
  void RestoreMaximumThreads(AutoIncrementPoolMax* p_increment);

  // Sleeping and waking-up a thread
  // Sleeps ANY thread. Also threads not originating in this threadpool
  void* SleepThread (DWORD_PTR p_unique,void* p_payload = nullptr);
  bool  WakeUpThread(DWORD_PTR p_unique,void* p_result  = nullptr);
  void* GetSleepingThreadPayload(DWORD_PTR p_unique);
  bool  EliminateSleepingThread (DWORD_PTR p_unique);

  // Create a hartbeat thread (Can be called **ONCE**)
  bool  CreateHartbeat(LPFN_CALLBACK p_callback,void* p_argument,DWORD p_hartbeat);
  // Stop a heartbeat thread
  void  StopHartbeat();

  // GETTERS

  int  GetMinThreads()          { return m_minThreads;          };
  int  GetMaxThreads()          { return m_maxThreads;          };
  int  GetStackSize()           { return m_stackSize;           };
  int  GetWorkOverflow()        { return (int)m_work.size();    };
  int  GetCleanupJobs()         { return (int)m_cleanup.size(); };

  // "Running A Thread" is public, but really should only be called 
  // from within the static work functions to get things working
  // Do **NOT** call from your application!!
  DWORD RunAThread(ThreadRegister* p_register);
  DWORD RunHartbeat();

private:
  // CONTROLING THE THREADPOOL

  // Stop a thread for good
  void StopThread(ThreadRegister* p_reg);
  // Pool locking from a single thread state change
  void LockPool();
  void UnlockPool();
  // Remove a thread from the threadpool
  void RemoveThreadPoolThread(unsigned p_threadID);
  // More work to do on a thread  (pool must be locked!!)
  bool WorkToDo(LPFN_CALLBACK& p_callback,void*& p_argument);
  // Initialize our new threadpool
  void InitThreadPool();
  // Stopping the thread pool
  void StopThreadPool();
  // Create a thread in the threadpool
  ThreadRegister* CreateThreadPoolThread();
  // Running all cleanup jobs for the threadpool
  void RunCleanupJobs();

  // This is the real callback. 
  // Overload for your needs, in your own class derived from ThreadPool
  virtual void DoTheCallback(LPFN_CALLBACK p_callback,void* p_argument);
  // The following can only be called on an overload with COMPLETION_CALL
  virtual void DoTheCallback(void* p_argument);

  // DATA MEMBERS OF THE THREADPOOL

  bool              m_initialized     { false   };              // TP properly initialized
  bool              m_openForWork     { false   };              // TP open for work (not closing)
  long              m_curThreads      { 0       };              // TP current number of threads
  long              m_bsyThreads      { 0       };              // TP busy    number of threads
  int               m_minThreads      { NUM_THREADS_MINIMUM };  // TP minimum number of threads
  int               m_maxThreads      { NUM_THREADS_MAXIMUM };  // TP maximum number of threads
  int               m_stackSize       { THREAD_STACKSIZE    };  // TP size of SP stack of each thread
  int               m_processors      { 1       };              // Number of logical processors on the system
  HANDLE            m_completion      { nullptr };              // I/O Completion port for I/O and thread sync
  ThreadMap         m_threads;                                  // Map with all running and waiting threads
  SleepingMap       m_sleeping;                                 // Registration of sleeping threads
  WorkMap           m_work;                                     // Map with the backlog of work to do
  WorkMap           m_cleanup;                                  // Cleanup jobs after closing the queue
  CRITICAL_SECTION  m_critical;                                 // Locking synchronization object
  // Hartbeat section
  LPFN_CALLBACK     m_hartbeatCallback { nullptr };             // HB callback function
  void*             m_hartbeatPayload  { nullptr };             // HB callback parameter to use
  DWORD             m_hartbeat         { 0       };             // HB milliseconds between hartbeats
  HANDLE            m_hartbeatEvent    { nullptr };             // HB event to wak up the hartbeat
};

// Auto locking mechanism for the threadpool
class AutoLockTP
{
public:
  AutoLockTP(CRITICAL_SECTION* p_lock)
  {
    EnterCriticalSection(m_lock = p_lock);
  }
 ~AutoLockTP()
  {
    LeaveCriticalSection(m_lock);
  }
private:
  CRITICAL_SECTION* m_lock;
};

// Mechanism to increment the poolsize for a period of time
// Intended for long running threads to call, just before entering 
// the WaitForSingleObject API 
//
class AutoIncrementPoolMax
{
public:
  AutoIncrementPoolMax(ThreadPool* p_pool)
  {
    m_pool = p_pool;
    m_pool->ExtendMaximumThreads(*this);
  }
  ~AutoIncrementPoolMax()
  {
    m_pool->RestoreMaximumThreads(this);
  }
private:
  friend ThreadPool;
  ThreadPool* m_pool;
};
