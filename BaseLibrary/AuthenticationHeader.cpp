/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: AuthenticationHeader.h
//
// BaseLibrary: Indispensable general objects and functions
//
// // Copyright (c) 2014-2022 ir. W.E. Huisman
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
  authenticate = EncodeStringForTheWire(authenticate);

  // STEP 3) Reverse our string
  authenticate.MakeReverse();

  // STEP 4) Apply a Caesar cipher revert(ascii 126 ~->ascii 32 (space))
  //         with the formula ('~' - char + ' ')
  for(int index = 0;index < authenticate.GetLength(); ++index)
  {
    uchar ch = (uchar) authenticate.GetAt(index);
    if(ch >= _T(' ') && ch <= _T('~'))
    {
      ch = _T('~') - ch + _T(' ');
      authenticate.SetAt(index,ch);
    }
  }

  // STEP 5) Five random characters in front and as a trailer
  // '~' (ASCII 127) - ' ' (ASCII 32) = 95. Equi-distance of printable chars
  time_t now;
  XString before,after;
  srand((unsigned int)time(&now));
  for(int index = 0;index < 5;++index)
  {
    before += (TCHAR)(_T(' ') + (TCHAR)(rand() % 95) - _T(' '));
    after  += (TCHAR)(_T(' ') + (TCHAR)(rand() % 95) - _T(' '));
  }
  authenticate = before + authenticate + after;
    
  // STEP 6) Base64 encryption 
  Base64 base;
  return base.Encrypt(authenticate);
}

// DECODE ALGORITHM
//
// 1) Empty the result
// 2) Base64 decode the string
// 3) If long enough, remove first and last five random characters
// 4) Apply Caesar Cipher by inverting in between ' ' and '~'
// 5) Search for the LAST separator in the string
// 6) Split the strings and reverse the order
// 7) Do the UTF-8 decoding (if necessary for your language)

bool DecodeAuthentication(XString p_scramble,XString& p_user,XString& p_password)
{
  // STEP 1) Reset the output
  p_user.Empty();
  p_password.Empty();

  // STEP 2) Decode base64
  Base64 base;
  XString authenticate = base.Decrypt(p_scramble);
  if(p_scramble.IsEmpty() || authenticate.IsEmpty())
  {
    return false;
  }

  // STEP 3) Remove prefix and postfix of five characters
  if(authenticate.GetLength() > 5)
  {
    authenticate = authenticate.Mid(5);
  }
  if(authenticate.GetLength() > 5)
  {
    authenticate = authenticate.Mid(0,authenticate.GetLength() - 5);
  }
  if(authenticate.IsEmpty())
  {
    return false;
  }

  // STEP 4) Invert the Caesar cipher
  for(int index = 0;index < authenticate.GetLength(); ++index)
  {
    uchar ch = (uchar) authenticate.GetAt(index);
    if(ch >= _T(' ') && ch <= _T('~'))
    {
      ch = _T('~') - ch + _T(' ');
      authenticate.SetAt(index,ch);
    }
  }

  // STEP 5: Search the separator
  int pos1 = authenticate.Find(_T("^"));
  if(pos1 > 0)
  {
    // Just in case someone uses it in a password
    // Can be used multiple times and on the last position!
    int pos2(0);
    while((pos2 = authenticate.Find(_T("^"),pos1 + 1)) >= 0)
    {
      pos1 = pos2;
    }

    // Split result
    p_password = authenticate.Left(pos1);
    p_user     = authenticate.Mid(pos1 + 1);

    // STEP 6: Reverse the strings
    p_password.MakeReverse();
    p_user.MakeReverse();

    // STEP 7: Assume the input is in UTF-8 format
    //         Don't do this in other languages that are already in UTF-8
    p_user     = DecodeStringFromTheWire(p_user);
    p_password = DecodeStringFromTheWire(p_password);

    return true;
  }
  // No separator!!
  return false;
}
