/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: CommandBus.h
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
#pragma once
#include "ThreadPool.h"
#include <map>

typedef struct _subscriber
{
  LPFN_CALLBACK m_target;
  void*         m_argument;
}
Subscriber;

using CommandMap = std::multimap<XString,Subscriber>;
using Commands   = std::pair<CommandMap::iterator,CommandMap::iterator>;

class CommandBus
{
public:
  CommandBus(XString p_name,ThreadPool* p_pool);
 ~CommandBus();

  // FUNCTIONS
 
  // Subscribe a function to a command
  bool SubscribeCommand(XString p_command,LPFN_CALLBACK p_function,void* p_default = NULL);
  // Un-Subscribe a command function
  bool UnSubscribe(XString p_command,LPFN_CALLBACK p_function);
  // Publish new command for all subscribers
  bool PublishCommand(XString p_command,void* p_argument);
  // Find out if command bus is (still) open
  bool IsOpen();
  // Close bus for new publishing
  void Close();
  // Find out if command has subscribers
  int  GetNumberOfSubscribers(XString p_command);

  // SETTERS
  void SetName(XString p_name)     { m_name = p_name; };
  void SetPool(ThreadPool* p_pool) { m_pool = p_pool; };

  // GETTERS
  XString     GetName()   { return m_name; };
  ThreadPool* GetPool()   { return m_pool; };

private:
  XString     m_name;               // Name of the command bus
  bool        m_open { false  };    // Open for publishing
  ThreadPool* m_pool { nullptr};    // Thread pool used by the bus
  CommandMap  m_subscribers;        // Command/subscriber-target pairs
  // Multi-threading lock for the bus
  CRITICAL_SECTION m_lock;
};
