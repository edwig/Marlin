/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestPorts.h
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
#pragma once

#ifdef MARLIN_IIS
// All tests running on these ports in Marlin IIS
// See project: MarlinServerIIS
const int TESTING_HTTP_PORT   =   80;   //            Site: /MarlinTest/
const int TESTING_HTTPS_PORT  = 1221;   // Port + 1   Site: /SecureTest/
const int TESTING_CLCERT_PORT = 1222;   // Port + 2   Site: /SecureClientCert/
const int TESTING_TOKEN_PORT  = 1223;   // Port + 3   Site: /MarlinToken/
const int TESTING_SECURE_WS   = 1224;   // Port + 4   Site: /SecureSockets/
#else
// All tests running on these ports in Marlin Standalone
// See project: MarlinServer
const int TESTING_HTTP_PORT   = 1200;
const int TESTING_HTTPS_PORT  = 1201;   // Port + 1
const int TESTING_CLCERT_PORT = 1202;   // Port + 2
const int TESTING_TOKEN_PORT  = 1203;   // Port + 3
const int TESTING_SECURE_WS   = 1204;   // Port + 4
#endif
