/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Encoding.h
//
// BaseLibrary: Indispensable general objects and functions
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

// Result of the defusion process
enum class BOMOpenResult
{
  NoString      // Empty input
 ,NoEncoding    // No encoding Byte-order-mark found
 ,BOM           // Byte-Order-Mark
 ,Incompatible  // Incompatible (not supported) encoding
};

// Type of BOM found to defuse
// As found on: https://en.wikipedia.org/wiki/Byte_order_mark
enum class Encoding
{
  Default      // No BOM found
 ,UTF8         // UTF 8 bits (General WEB standard)
 ,LE_UTF16     // Little-Endian UTF 16 bits (Intel & MS-Windows!)
 ,LE_UTF32     // Little-Endian UTF 32 bits
 ,BE_UTF16     // Big-Endian UTF 16 bits (Motorola & Apple MacIntosh)
 ,BE_UTF32     // Big-Endian UTF 32 bits
 ,UTF7         // UTF 7 bits (Obsolete variable length encoding)
 ,UTF1         // UTF-1 (Variable width encoding, not searchable!)
 ,UTF_EBCDIC   // IBM EBCDIC Unicode
 ,SCSU         // Standard Compression Code for Unicode
 ,BOCU_1       // Binary Ordered Compressed Unicode
 ,GB_18030     // Chinese government: Guojia Biazhun coding standard 18030
};

// Default encoding (opening files etc)
#ifdef UNICODE
#define EncodingDefault Encoding::LE_UTF16
#else
#define EncodingDefault Encoding::Default
#endif
