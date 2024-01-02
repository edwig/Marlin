/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MarlinServer.cpp
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
#include "MarlinServer.h"
#include <assert.h>

// The one-and-only Marlin server
MarlinServer* s_theServer = nullptr;

MarlinServer::MarlinServer()
{
  assert(s_theServer == nullptr);
  s_theServer = this;
}

MarlinServer::~MarlinServer()
{
  assert(s_theServer == this);
  s_theServer = nullptr;
}

// Should not come to here!
// You should derive a class from "MarlinServer" and do your thing there!
//
bool MarlinServer::Startup()
{
  assert(s_theServer != nullptr);
  return false;
}

void MarlinServer::ShutDown()
{
}