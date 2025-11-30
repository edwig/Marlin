/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: IsUnicodeUTF8.cpp
//
// BaseLibrary: Indispensable general objects and functions
// 
// Created: 2014-2025 ir. W.E. Huisman
// MIT License
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
#include "IsUnicodeUTF8.h"

// https://en.wikipedia.org/wiki/UTF-8
//
//              Byte 1     Byte 2     Byte 3     Byte 4
// -----------|----------|----------|----------|----------|
// 1 Byte     | 0xxxxxxx |          |          |          |
// 2 Bytes    | 110xxxxx | 10xxxxxx |          |          |
// 3 Bytes    | 1110xxxx | 10xxxxxx | 10xxxxxx |          |
// 4 Bytes    | 11110xxx | 10xxxxxx | 10xxxxxx | 10xxxxxx |

bool
IsTextUnicodeUTF8(LPCTSTR p_pointer,size_t p_length)
{
  int blocklen = 1;
  bool ascii = true;

  while(*p_pointer && p_length--)
  {
    TCHAR ch = *p_pointer++;

    if(ascii)
    {
      if(ch & 0x80)
      {
             if ((ch >> 3) == 0x1E) blocklen = 3;
        else if ((ch >> 4) == 0x0E) blocklen = 2;
        else if ((ch >> 5) == 0x06) blocklen = 1;
        else
        {
          // No UTF-8 Escape code
          return false;
        }
        ascii = false;
      }
    }
    else
    {
      if ((ch >> 6) == 0x2)
      {
        if (--blocklen == 0)
        {
          ascii = true;
        }
      }
      else
      {
        // Can only for 0x10xxxxxxx
        return false;
      }
    }
  }
  return true;
}
