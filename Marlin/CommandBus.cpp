/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: CommandBus.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2024 ir. W.E. Huisman
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
#include "CommandBus.h"
#include "ThreadPool.h"
#include "AutoCritical.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

CommandBus::CommandBus(XString p_name,ThreadPool* p_pool)
           :m_name(p_name)
           ,m_pool(p_pool)
{
  InitializeCriticalSection(&m_lock);
  m_open = true;
}

CommandBus::~CommandBus()
{
  DeleteCriticalSection(&m_lock);
}

// Find out if command bus is open
bool 
CommandBus::IsOpen()
{
  AutoCritSec lock(&m_lock);
  return m_open;
}

// Close bus for new publishing
void
CommandBus::Close()
{
  AutoCritSec lock(&m_lock);
  m_open = false;
}

// Subscribe a function to a command
bool
CommandBus::SubscribeCommand(XString p_command,LPFN_CALLBACK p_function,void* p_default /*=NULL*/)
{
  AutoCritSec lock(&m_lock);

  // See if bus is stil 'open'
  if(IsOpen() == false)
  {
    return false;
  }

  Commands comm = m_subscribers.equal_range(p_command);
  for(CommandMap::iterator it = comm.first;it != comm.second;++it)
  {
    if(it->second.m_target == p_function)
    {
      // Command already subscribed for this function
      return false;
    }
  }
  // Create subscriber target
  Subscriber target;
  target.m_target   = p_function;
  target.m_argument = p_default;

  m_subscribers.insert(std::make_pair(p_command,target));
  return true;
}

// Find out if command has subscribers
int  
CommandBus::GetNumberOfSubscribers(XString p_command)
{
  AutoCritSec lock(&m_lock);

  int number = 0;
  Commands comm = m_subscribers.equal_range(p_command);

  for(CommandMap::iterator it = comm.first; it != comm.second; ++it)
  {
    ++number;
  }
  return number;
}

// Un-Subscribe a command function
bool 
CommandBus::UnSubscribe(XString p_command,LPFN_CALLBACK p_function)
{
  AutoCritSec lock(&m_lock);

  Commands comm = m_subscribers.equal_range(p_command);

  for(CommandMap::iterator it = comm.first; it != comm.second; ++it)
  {
    if(it->second.m_target == p_function)
    {
      m_subscribers.erase(it);
      return true;
    }
  }
  return false;
}

// Publish new command for all subscribers
bool 
CommandBus::PublishCommand(XString p_command,void* p_argument)
{
  AutoCritSec lock(&m_lock);

  // See if bus is stil 'open'
  if(IsOpen() == false)
  {
    return false;
  }

  // Find all subscribers
  bool result = false;
  Commands comm = m_subscribers.equal_range(p_command);

  // Publish to each subscriber through the threadpool
  for(CommandMap::iterator it = comm.first; it != comm.second; ++it)
  {
    m_pool->SubmitWork(it->second.m_target,p_argument);
    result = true;
  }
  return result;
}
