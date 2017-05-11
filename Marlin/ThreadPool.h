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

// State that a thread can be in
// enum class ThreadState
// {
//   THST_Init       // Creating thread in pool
//  ,THST_Waiting    // Waiting in threadpool
//  ,THST_Running    // Running in code
//  ,THST_Stopping   // Busy stopping to wait
//  ,THST_Stopped    // Thread already dead
// };

// Registration of a waiting/running thread
class ThreadRegister
{
public:
    ThreadPool*   m_pool;
    HANDLE        m_thread;
    unsigned      m_threadId;
};

using ThreadMap = std::vector<ThreadRegister*>;

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
  // Setting the CPU throttling
  void SetUseCPULoad(bool p_useCpuLoad);
  // Extend the maximum for a period of time
  void ExtendMaximumThreads (AutoIncrementPoolMax& p_increment);
  void RestoreMaximumThreads(AutoIncrementPoolMax* p_increment);

  // Sleeping and waking-up a thread
  void* SleepThread (DWORD_PTR p_unique,void* p_payload = nullptr);
  bool  WakeUpThread(DWORD_PTR p_unique,void* p_result  = nullptr);
  void* GetSleepingThreadPayload(DWORD_PTR p_unique);
  void  EliminateSleepingThread (DWORD_PTR p_unique);

  // Stop a heartbeat thread
  void  StopHartbeat(LPFN_CALLBACK p_callback);

  // GETTERS

  int  GetMinThreads()          { return m_minThreads;          };
  int  GetMaxThreads()          { return m_maxThreads;          };
  int  GetStackSize()           { return m_stackSize;           };
  int  GetWorkOverflow()        { return (int)m_work.size();    };
  int  GetWaitingThreads()      { return WaitingThreads();      };
  bool GetUseCPULoad()          { return m_useCPULoad;          };
  int  GetCleanupJobs()         { return (int)m_cleanup.size(); };

  // "Running A Thread" is public, but really should only be called 
  // from within the static work functions to get things working
  // Do **NOT** call from your application!!
  DWORD RunAThread(ThreadRegister* p_register);

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
  ThreadRegister* CreateThreadPoolThread(DWORD p_hartbeat = INFINITE);
  // Running all cleanup jobs for the threadpool
  void RunCleanupJobs();

  // This is the real callback. 
  // Overload for your needs, in your own class derived from ThreadPool
  virtual void DoTheCallback(LPFN_CALLBACK p_callback,void* p_argument);

  // DATA MEMBERS OF THE THREADPOOL

  bool              m_initialized     { false   };              // TP properly initialized
  bool              m_openForWork     { false   };              // TP open for work (not closing)
  long              m_curThreads      { 0       };              // TP current number of threads
  long              m_bsyThreads      { 0       };              // TP busy    number of threads
  int               m_minThreads      { NUM_THREADS_MINIMUM };  // TP minimum number of threads
  int               m_maxThreads      { NUM_THREADS_MAXIMUM };  // TP maximum number of threads
  int               m_stackSize       { THREAD_STACKSIZE    };  // TP size of SP stack of each thread
  bool              m_useCPULoad      { false   };              // TP uses CPU load for work throttling
  LPFN_CALLBACK     m_hartbeat        { nullptr };              // Main heartbeat callback function
  void*             m_hartbeatContext { nullptr };              // Pointer to the context of the heartbeat
  int               m_processors      { 1       };              // Number of logical processors on the system
  HANDLE            m_completion      { nullptr };              // I/O Completion port for I/O and thread sync
  ThreadMap         m_threads;                                  // Map with all running and waiting threads
  WorkMap           m_work;                                     // Map with the backlog of work to do
  WorkMap           m_cleanup;                                  // Cleanup jobs after closing the queue
  CRITICAL_SECTION  m_critical;                                 // Locking synchronization object
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
