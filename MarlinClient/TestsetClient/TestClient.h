/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestClient.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
#include "TestPorts.h"
#include "StdException.h"

#define NUM_RM_TESTS 3

class   HTTPClient;
class   LogAnalysis;

// In TestClient.cpp
extern bool doDetails;

void xprintf(const char* p_format,...);
void WaitForKey();

// Define your other host here!
// By commenting out the marlin_host above, and uncommenting this one
// #define MARLIN_HOST "my-other-machine"  

// In the various testing files
extern int TestJSON(void);
extern int TestUnicode(void);
extern int TestURLChars(void);
extern int TestCryptography(void);
extern int TestReader(void);
extern int TestConvert(void);
extern int TestNamespaces(void);
extern int TestCookiesOverwrite(void);
extern int TestDecryptCookie(void);
extern int TestFindClientCertificate(void);
extern int TestWebSocketAccept(void);
extern int TestWebSocket(LogAnalysis* p_log);
extern int TestCloseWebSocket(void);
extern int TestEvents(HTTPClient* p_client);
extern int TestCookies(HTTPClient& p_client);
extern int TestContract(HTTPClient* p_client,bool p_json,bool p_tokenProfile);
extern int TestJsonData(HTTPClient* p_client);
extern int TestPatching(HTTPClient* p_client);
extern int TestFormData(HTTPClient* p_client);
extern int TestBaseSite(HTTPClient* p_client);
extern int TestSecureSite(HTTPClient* p_client);
extern int TestCompression(HTTPClient* p_client);
extern int TestWebservices(HTTPClient& p_client);
extern int TestClientCertificate(HTTPClient* p_client);
extern int TestMSGraph(HTTPClient* p_client);