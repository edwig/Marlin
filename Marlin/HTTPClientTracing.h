/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPClientTracing.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2021 ir. W.E. Huisman
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

class HTTPClient;
class LogAnalysis;

// All HttpSetOption/HttpQueryOption are found here:
// https://msdn.microsoft.com/en-us/library/aa384066(v=vs.85).aspx

class HTTPClientTracing
{
public:
  HTTPClientTracing(HTTPClient* p_client);
 ~HTTPClientTracing();

  // Trace of all the settings of the session and the request handle
  void Trace(char* p_when,HINTERNET p_session,HINTERNET p_request);

private:
  bool    QueryBool  (HINTERNET p_handle,DWORD p_option,BOOL*  p_bool,   const char* p_optionName);
  bool    QueryWord  (HINTERNET p_handle,DWORD p_option,DWORD* p_word,   const char* p_optionName);
  bool    QueryVoid  (HINTERNET p_handle,DWORD p_option,void** p_pointer,const char* p_optionName);
  bool    QueryObject(HINTERNET p_handle,DWORD p_option,void*  p_pointer,DWORD p_size,const char* p_optionName);
  CString QueryString(HINTERNET p_handle,DWORD p_option,const char*  p_optionName);

  // Works for this client
  HTTPClient*   m_client;
  LogAnalysis*  m_log;
};
