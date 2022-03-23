/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: CreateURLPrefix.h
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

enum class PrefixType
{
  URLPRE_Strong   // Strong binding      "+"
 ,URLPRE_Named    // Short server name   "marlin"
 ,URLPRE_FQN      // Full qualified name "marlin.organization.lan"
 ,URLPRE_Address  // Direct IP address   "p.q.r.s"
 ,URLPRE_Weak     // Weak binding        "*"
};

//#define NI_MAXHOST 1024

// Types for "GetHostName"
#define HOSTNAME_SHORT  1
#define HOSTNAME_IP     2
#define HOSTNAME_FULL   3

XString GetHostName(int p_type);

XString SocketToServer(PSOCKADDR_IN6 p_address);

XString CreateURLPrefix(PrefixType p_type
                       ,bool       p_secure
                       ,int        p_port
                       ,XString    p_path);