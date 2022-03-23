/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Base64.h
//
// BaseLibrary: Indispensable general objects and functions
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

#define SWAP(a, b) ((a) ^= (b), (b) ^= (a), (a) ^= (b))

class Base64
{
public:
  static XString         Encrypt(XString p_unencrypted);
  static XString         Decrypt(XString p_encrypted);
  static unsigned char*  Encrypt(const unsigned char* srcp,int len,unsigned char * dstp);
  static void*           Decrypt(const unsigned char* srcp,int len,unsigned char * dstp);
  static size_t          B64_length  (size_t len);
  static size_t          Ascii_length(size_t len);
};

class CRC4 
{
public:
  char *Encrypt(char *pszText,const char *pszKey);
  char *Decrypt(char *pszText,const char *pszKey);
};