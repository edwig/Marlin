/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: AuthenticationHeader.h
//
// BaseLibrary: Indispensable general objects and functions
//
// // Copyright (c) 2014-2025 ir. W.E. Huisman
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
#include "pch.h"
#include "AuthenticationHeader.h"
#include "ConvertWideString.h"
#include "Base64.h"
#include <time.h>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

#define EXTRABUF 5

// ALGORITHM
//
// 1) Add user and password together with a '~' in between
// 2) Encode in UTF-8 if your language is not already that!
// 3) Reverse the complete string
// 4) Apply Ceasar Cipher by inverting in between ' ' and '~'
// 5) Add five random characters at the begining and the end
// 6) Do Base64 encrypting
//
XString CreateAuthentication(XString p_user,XString p_password)
{
  XString authenticate;
  // STEP 1) Put together with a less used separator
  authenticate = p_user + _T("^") + p_password;

  // STEP 2) Make it an UTF-8 string
  //         Don't do this in other languages that are already in UTF-8
  BYTE* buffer = nullptr;
  int   length = 0;

#ifdef _UNICODE
  TryCreateNarrowString(authenticate,_T(""),false,&buffer,length);
#else
  XString ext = EncodeStringForTheWire(authenticate);
  length = ext.GetLength();
  buffer = new BYTE[length + 1];
  strncpy_s((char*)buffer,length + 1,ext.GetString(),length);
#endif


  // STEP 3) Reverse our string in the buffer
  BYTE* resbuffer = new BYTE[length + 1 + 2 * EXTRABUF];
  for(int ind = 0;ind <length; ++ind)
  {
    resbuffer[EXTRABUF + ind] = buffer[length - ind - 1];
  }

  // STEP 4) Apply a Caesar cipher revert(ascii 126 ~->ascii 32 (space))
  //         with the formula ('~' - char + ' ')
  for(int index = 0;index < length; ++index)
  {
    BYTE ch = resbuffer[EXTRABUF + index];
    if(ch >= _T(' ') && ch <= _T('~'))
    {
      ch = _T('~') - ch + _T(' ');
      resbuffer[EXTRABUF + index] = ch;
    }
  }

  // STEP 5) Five random characters in front and as a trailer
  // '~' (ASCII 127) - ' ' (ASCII 32) = 95. Equi-distance of printable chars
  time_t now;
  srand((unsigned int)time(&now));
  for(int index = 0;index < EXTRABUF;++index)
  {
    resbuffer[index]                     = (BYTE)(_T(' ') + (BYTE)(rand() % 95));
    resbuffer[EXTRABUF + length + index] = (BYTE)(_T(' ') + (BYTE)(rand() % 95));
  }
  resbuffer[length + 2 * EXTRABUF] = 0;
    
  // STEP 6) Base64 encryption 
  Base64 base;
  XString result = base.Encrypt(resbuffer,length + 2 * EXTRABUF);

  delete [] buffer;
  delete [] resbuffer;

  return result;
}

// DECODE ALGORITHM
//
// 1) Empty the result
// 2) Base64 decode the string
// 3) Check if longer than 2 extra buffers
// 4) Apply Caesar Cipher by inverting in between ' ' and '~'
// 5) Revert the buffer
// 6) Revert the UTF-8 in the buffer to stirng
// 7) Search for the LAST separator in the string
//    Split the strings and reverse the order

bool 
DecodeAuthentication(XString p_scramble,XString& p_user,XString& p_password)
{
  bool result(false);

  // STEP 1) Reset the output
  p_user.Empty();
  p_password.Empty();

  // STEP 2) Decode base64
  Base64 base;
  int length = (int)base.Ascii_length(p_scramble.GetLength());
  BYTE* buffer = new BYTE[length + 1];
  base.Decrypt(p_scramble,buffer,length + 1);
  length = (int) strlen((char*)buffer);
  if(p_scramble.IsEmpty() || length == 0)
  {
    return false;
  }

  // STEP 3) Remove prefix and postfix of five characters
  length -= 2 * EXTRABUF;
  if(length <= 0)
  {
    return false;
  }

  // STEP 4) Invert the Caesar cipher
  for(int index = 0;index < length; ++index)
  {
    BYTE ch = buffer[EXTRABUF + index];
    if(ch >= _T(' ') && ch <= _T('~'))
    {
      ch = _T('~') - ch + _T(' ');
      buffer[EXTRABUF + index] = ch;
    }
  }

  // STEP 5: Revert the buffer
  BYTE* resbuffer = new BYTE[length + 1 + 2 * EXTRABUF];
  for(int ind = 0;ind < length; ++ind)
  {
    resbuffer[ind] = buffer[EXTRABUF + length - ind - 1];
  }
  resbuffer[length] = 0;

  // STEP 6: Revert UTF-8 buffer to string
  XString authenticate;
#ifdef _UNICODE
  bool bom(false);
  TryConvertNarrowString(resbuffer,length,_T(""),authenticate,bom);
#else
  XString buf(resbuffer);
  authenticate = DecodeStringFromTheWire(buf);
#endif

  // STEP 7: Search the separator
  int pos = authenticate.Find(_T("^"));
  if(pos > 0)
  {
    // Split result
    p_user     = authenticate.Left(pos);
    p_password = authenticate.Mid(pos + 1);
    result     = true;
  }

  delete [] buffer;
  delete [] resbuffer;

  return result;
}
