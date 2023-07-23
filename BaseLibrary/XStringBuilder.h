/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XStringBuilder.h
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
#include <list>

class XStringBuilder
{
public:

  // Default XTOR
  XStringBuilder();

  // Copy XTOR
  explicit XStringBuilder(XStringBuilder const& rhs);

  // XTOR from XString
  //
  explicit XStringBuilder(XString const& rhs);

  // Assignment from XStringBuilder
  XStringBuilder const& operator = (XStringBuilder const& rhs);

  // Assignment from XString
  XStringBuilder const& operator = (XString const& rhs);

  // Clear the current contents
  void Clear();

  // Is there any text in here?
  bool IsEmpty() const;

  // Retrieve the length of the whole string
  int GetLength() const;

  // Convert the currently stored segments to a single segment
  // and return a reference to the newly formed string. This
  // is considered a const operation since the representation
  // changes but not the actual value.
  //
  XString const& ToString() const;

  // Append the segments of a XStringBuilder
  XStringBuilder& Append(XStringBuilder const& sb);

  // Append a string
  XStringBuilder& Append(XString const& segment);

  // Prepend the segments of a XStringBuilder
  XStringBuilder& Prepend(XStringBuilder const& sb);

  // Prepend a string
  XStringBuilder& Prepend(XString const& segment);

  // Append operators
  XStringBuilder& operator += (XStringBuilder const& rhs);
  XStringBuilder& operator += (XString const& rhs);
  XStringBuilder& operator += (int rhs);

  // Compare to other string
  int Compare(char const* psz) const;

  // Implicit conversion to string
  operator XString const& () const;

  // Implicit conversion to char pointer
  operator char const* () const;

private:

  // Segment list
  typedef std::list<XString> ListType;

  // Member data
  mutable ListType  m_segments;
  int               m_length;
};

inline bool
operator == (XStringBuilder const& lhs,XStringBuilder const& rhs)
{
  return lhs.Compare(rhs) == 0;
}

inline bool
operator != (XStringBuilder const& lhs,XStringBuilder const& rhs)
{
  return lhs.Compare(rhs) != 0;
}

inline bool
operator == (XStringBuilder const& lhs,XString const& rhs)
{
  return lhs.Compare(rhs) == 0;
}

inline bool
operator != (XStringBuilder const& lhs,XString const& rhs)
{
  return lhs.Compare(rhs) != 0;
}

inline bool
operator == (XString const& lhs,XStringBuilder const& rhs)
{
  return lhs.Compare(rhs) == 0;
}

inline bool
operator != (XString const& lhs,XStringBuilder const& rhs)
{
  return lhs.Compare(rhs) != 0;
}

inline bool
operator == (XStringBuilder const& lhs,char const* rhs)
{
  return lhs.Compare(rhs) == 0;
}

inline bool
operator != (XStringBuilder const& lhs,char const* rhs)
{
  return lhs.Compare(rhs) != 0;
}

inline bool
operator == (char const* lhs,XStringBuilder const& rhs)
{
  return rhs.Compare(lhs) == 0;
}

inline bool
operator != (char const* lhs,XStringBuilder const& rhs)
{
  return rhs.Compare(lhs) != 0;
}
