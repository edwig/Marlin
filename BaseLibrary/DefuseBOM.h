/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: DefuseBOM.h
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

// Result of the defusion process
enum class BOMOpenResult
{
  BOR_NoString
 ,BOR_NoBom
 ,BOR_Bom
 ,BOR_OpenedIncompatible
};

// Type of BOM found to defuse
// As found on: https://en.wikipedia.org/wiki/Byte_order_mark
enum class BOMType
{
  BT_NO_BOM
 ,BT_BE_UTF1
 ,BT_BE_UTF7
 ,BT_BE_UTF8
 ,BT_BE_UTF16
 ,BT_BE_UTF32
 ,BT_BE_CSCU
 ,BT_LE_UTF8
 ,BT_LE_UTF16
 ,BT_LE_UTF32
 ,BT_UTF_EBCDIC
 ,BT_BOCU_1
 ,BT_GB_18030
};

// Check for a Byte-Order-Mark (BOM)
BOMOpenResult CheckForBOM(const unsigned char* p_pointer
                         ,BOMType&             p_type
                         ,unsigned int&        p_skip);

