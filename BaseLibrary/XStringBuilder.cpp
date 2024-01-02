/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XStringBuilder.cpp
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
#include "pch.h"
#include "XStringBuilder.h"
#include <algorithm>
#include <iterator>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef _DEBUG
#endif

//////////////////////////////////////////////////////////////////////////

class XAppender
{
  PTCHAR m_dst;

public:

  explicit XAppender(PTCHAR dst): m_dst(dst)
  {
  }

  void operator () (XString const& src)
  {
    int len = src.GetLength();
    _tcsncpy_s(m_dst,len + 1,src,len);
    m_dst += len;
  }
};

//////////////////////////////////////////////////////////////////////////

XStringBuilder::XStringBuilder()
               :m_length(0)
{
}

XStringBuilder::XStringBuilder(XStringBuilder const& rhs)
               :m_length(0)
{
  Append(rhs);
}

XStringBuilder::XStringBuilder(XString const& rhs)
               :m_length(0)
{
  Append(rhs);
}

XStringBuilder const&
XStringBuilder::operator = (XStringBuilder const& rhs)
{
  if(this != &rhs)
  {
    m_segments = rhs.m_segments;
    m_length = rhs.m_length;
  }
  return *this;
}

XStringBuilder const&
XStringBuilder::operator = (XString const& rhs)
{
  Clear();
  Append(rhs);
  return *this;
}

void
XStringBuilder::Clear()
{
  m_segments.clear();
  m_length = 0;
}

bool
XStringBuilder::IsEmpty() const
{
  return m_length == 0;
}

int
XStringBuilder::GetLength() const
{
  return m_length;
}

XStringBuilder&
XStringBuilder::Append(XStringBuilder const& sb)
{
  if(!sb.IsEmpty())
  {
    std::copy(sb.m_segments.begin(),sb.m_segments.end(),std::back_inserter(m_segments));
    m_length += sb.m_length;
  }
  return *this;
}

XStringBuilder&
XStringBuilder::Append(XString const& segment)
{
  if(int len = segment.GetLength())
  {
    m_segments.push_back(segment);
    m_length += len;
  }
  return *this;
}

XStringBuilder&
XStringBuilder::Prepend(XStringBuilder const& sb)
{
  if(!sb.IsEmpty())
  {
    std::copy(sb.m_segments.begin(),sb.m_segments.end(),std::front_inserter(m_segments));
    m_length += sb.m_length;
  }
  return *this;
}

XStringBuilder&
XStringBuilder::Prepend(XString const& segment)
{
  if(int len = segment.GetLength())
  {
    m_segments.push_front(segment);
    m_length += len;
  }
  return *this;
}

XStringBuilder&
XStringBuilder::operator += (XStringBuilder const& rhs)
{
  return Append(rhs);
}

XStringBuilder&
XStringBuilder::operator += (XString const& rhs)
{
  return Append(rhs);
}

XStringBuilder&
XStringBuilder::operator += (int rhs)
{
  XString number;
  number.Format(_T("%d"),rhs);
  return Append(number);
}

int
XStringBuilder::Compare(PCTSTR psz) const
{
  return ToString().Compare(psz);
}

XStringBuilder::operator XString const& () const
{
  return ToString();
}

XStringBuilder::operator PCTSTR () const
{
  return ToString();
}

// COMPACT the XString list to one single string!
// THIS IS OUR MAIN PURPOSE OF EXISTENCE!

XString const&
XStringBuilder::ToString() const
{
  // Empty the XStringBuilder
  if(m_length == 0)
  {
    static XString dummyString;
    return dummyString;
  }

  // Already a single segment
  if(m_segments.size() == 1)
  {
    return *m_segments.begin();
  }

  // Remember size of first segment
  int firstlen = m_segments.begin()->GetLength();

  // Resize first segment to make space for segments 1...n
  PTCHAR ptr = m_segments.begin()->GetBufferSetLength(m_length);

  // Offset pointer by length of first segment
  ptr += firstlen;

  // Append segments 1...n to the first segment
  std::for_each(++m_segments.begin(),m_segments.end(),XAppender(ptr));

  // Remove segments 1...n from the list
  m_segments.erase(++m_segments.begin(),m_segments.end());

  // Return a reference to the newly merged string
  return *m_segments.begin();
}
