/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: DefuseBOM.cpp
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
#include "pch.h"
#include "DefuseBOM.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Check for a Byte-Order-Mark (BOM)
BOMOpenResult CheckForBOM(const unsigned char* p_pointer
                         ,BOMType&             p_type
                         ,unsigned int&        p_skip)
{
  // Preset nothing
  p_type = BOMType::BT_NO_BOM;
  p_skip = 0;

  // Get first four characters in the message as integers
  int c1 = * p_pointer;
  int c2 = *(p_pointer + 1);
  int c3 = *(p_pointer + 2);
  int c4 = *(p_pointer + 3);

  // Check if Big-Endian UTF-8 BOM
  if(c1 == 0xEF && c2 == 0xBB && c3 == 0xBF)
  {
    // Yes BE-BOM in UTF-8
    p_skip = 3;
    p_type = BOMType::BT_BE_UTF8;
    return BOMOpenResult::BOR_Bom;
  }
  // Check UTF-8 BOM in other Endian 
  if(c1 == 0xBB || c1 == 0xBF)
  {
    // UTF-8 but incompatible. Might work yet!!
    p_skip = 3;
    p_type = BOMType::BT_LE_UTF8;
    return BOMOpenResult::BOR_OpenedIncompatible;
  }
  // Check Big-Endian UTF-16
  if(c1 == 0xFE && c2 == 0xFF)
  {
    p_skip = 2;
    p_type = BOMType::BT_BE_UTF16;
    return BOMOpenResult::BOR_Bom;
  }
  // Check Little-Endian UTF-16/UTF32
  if(c1 == 0xFF && c2 == 0xFE)
  {
    if(c3 == 0x0 && c4 == 0x0)
    {
      p_skip = 4;
      p_type = BOMType::BT_LE_UTF32;
      return BOMOpenResult::BOR_OpenedIncompatible;
    }
    p_skip = 2;
    p_type = BOMType::BT_LE_UTF16;
    return BOMOpenResult::BOR_OpenedIncompatible;
  }
  // Check Big-Endian UTF-32
  if(c1 == 0 && c2 == 0 && c3 == 0xFE && c4 == 0xFF)
  {
    p_skip = 4;
    p_type = BOMType::BT_BE_UTF32;
    return BOMOpenResult::BOR_OpenedIncompatible;
  }
  // Check for UTF-7 special case
  if(c1 == 0x2B && c2 == 0x2F && c3 == 0x76)
  {
    if(c4 == 0x38 || c4 == 39 || c4 == 0x2B || c4 == 0x2F)
    {
      // Application still has to process lower 2 bits 
      // of the 4th character. Re-spool to that char.
      p_skip = 3;
      p_type = BOMType::BT_BE_UTF7;
      return BOMOpenResult::BOR_OpenedIncompatible;
    }
  }
  // Check for UTF-1 special case
  if(c1 == 0xF7 && c2 == 0x64 && c3 == 0x4C)
  {
    p_skip = 3;
    p_type = BOMType::BT_BE_UTF1;
    return BOMOpenResult::BOR_OpenedIncompatible;
  }
  // Check for UTF-EBCDIC IBM set
  if(c1 == 0xDD && c2 == 0x73 && c3 == 0x66 && c4 == 0x73)
  {
    p_skip = 4;
    p_type = BOMType::BT_UTF_EBCDIC;
    return BOMOpenResult::BOR_OpenedIncompatible;
  }
  // Check for CSCU 
  if(c1 == 0x0E && c2 == 0xFE && c3 == 0xFF)
  {
    p_skip = 3;
    p_type = BOMType::BT_BE_CSCU;
    return BOMOpenResult::BOR_OpenedIncompatible;
  }
  // Check for BOCU-1
  if(c1 == 0xFB && c2 == 0xEE && c3 == 0x28)
  {
    p_skip = 3;
    p_type = BOMType::BT_BOCU_1;
    return BOMOpenResult::BOR_OpenedIncompatible;
  }
  // Check GB-18030
  if(c1 == 0x84 && c2 == 0x31 && c3 == 0x95 && c4 == 0x33)
  {
    p_skip = 4;
    p_type = BOMType::BT_GB_18030;
    return BOMOpenResult::BOR_OpenedIncompatible;
  }
  // NOT A BOM !!
  return BOMOpenResult::BOR_NoBom;
}

