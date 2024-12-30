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

#define CLIENT_CONFIG "STAND ALONE"

// All tests running on these ports in Marlin Standalone
const int TESTING_HTTP_PORT   = 1200;  // For site "/MarlinTest/" 
const int TESTING_HTTPS_PORT  = 1201;  // For site "/SecureTest/" 
const int TESTING_CLCERT_PORT = 1202;  // For site "/SecureClientCert/" 
const int TESTING_TOKEN_PORT  = 1203;  // For site "/MarlinToken/"
const int TESTING_SECURE_WS   = 1204;  // For site "/SecureSockets/"

// COMPILE FOR A CROSS-HOST TEST, OR A LOCALHOST TEST
#define MARLIN_HOST _T("localhost")

#define TEST_WEBSOCKETS 1