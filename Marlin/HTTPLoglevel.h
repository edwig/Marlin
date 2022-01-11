/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPLoglevel.h
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

// HTTP Logging Levels (HLL) of the server and client processing

#define HLL_NOLOG      0       // No logging is ever done
#define HLL_ERRORS     1       // Only errors are logged
#define HLL_LOGGING    2       // 1 + Logging of actions
#define HLL_LOGBODY    3       // 2 + Logging of actions and soap bodies
#define HLL_TRACE      4       // 3 + Tracing of settings
#define HLL_TRACEDUMP  5       // 4 + Tracing and HEX dumping of objects

// Last & highest logging level
#define HLL_HIGHEST    HLL_TRACEDUMP

// In order to work the object's member that holds the HLL logging level must 
// always be named "m_loglevel" :-)
// So you can write "   if(MUSTLOG(HLL_LOGBODY)) .... etc"
// And thusly optimize the performance of your software

#define MUSTLOG(x)  (m_logLevel >= (x))
