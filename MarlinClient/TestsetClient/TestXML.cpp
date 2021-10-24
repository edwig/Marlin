/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestXML.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
#include "stdafx.h"
#include "XMLMessage.h"
#include "XPath.h"
#include "bcd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int TestXPath()
{
  CString xml;
  XMLMessage msg;
  int errors = 0;

  xml = "<bookstore>\n"
        "\n"
        "  <book category=\"cooking\">\n"
        "    <title lang=\"en\">Everyday Italian</title>\n"
        "    <author>Giada De Laurentiis</author>\n"
        "    <year>2005</year>\n"
        "    <price>30.00</price>\n"
        "  </book>\n"
        "\n"
        "  <book category=\"children\">\n"
        "    <title lang=\"en\">Harry Potter</title>\n"
        "    <author>J K. Rowling</author>\n"
        "    <year>2005</year>\n"
        "    <price>29.99</price>\n"
        "  </book>\n"
        "\n"
        "  <book category=\"web\">\n"
        "    <title lang=\"EN-US\">XQuery Kick Start</title>\n"
        "    <author>James McGovern</author>\n"
        "    <author>Per Bothner</author>\n"
        "    <author>Kurt Cagle</author>\n"
        "    <author>James Linn</author>\n"
        "    <author>Vaidyanathan Nagarajan</author>\n"
        "    <year>2003</year>\n"
        "    <price>49.99</price>\n"
        "  </book>\n"
        "\n"
        "  <book category=\"web\">\n"
        "    <title>Start Learning XML</title>\n"
        "    <author>Erik T. Ray</author>\n"
        "    <year>2003</year>\n"
        "    <price>39.95</price>\n"
        "  </book>\n"
        "\n"
        "</bookstore>\n";

  msg.ParseMessage(xml);
  if(msg.GetInternalError() != XmlError::XE_NoError)
  {
    ++errors;
  }

  // Test array indexing (1-based!)
  XPath path(&msg,"/bookstore/book[2]");
  if(path.GetNumberOfMatches() == 1)
  {
    if(path.GetFirstResult()->GetAttributes().front().m_value != "children")
    {
      ++errors;
    }
  }
  else ++errors;

  // Check regular node value
  path.SetPath("/bookstore/book[4]/price");
  if(path.GetNumberOfMatches() == 1)
  {
    bcd price(path.GetFirstResult()->GetValue());
    if(price.AsString() != "39.95")
    {
      ++errors;
    }
  }
  else ++errors;

  // Test finding all the attributes
  path.SetPath("//title[@lang]");
  if(path.GetNumberOfMatches() != 3)
  {
    ++errors;
  }

  // Test getting the last of an array
  path.SetPath("/bookstore/book[last()]");
  if(path.GetNumberOfMatches() == 1)
  {
    bcd price(path.GetFirstResult()->GetChildren()[3]->GetValue());
    if(price.AsString() != "39.95")
    {
      ++errors;
    }
  }
  else ++errors;

  // Test number expression
  path.SetPath("/bookstore//book[price<35]");
  if(path.GetNumberOfMatches() != 2)
  {
    ++errors;
  }

  path.SetPath("/bookstore/book[contains(title,'Start')]");
  if(path.GetNumberOfMatches() != 2)
  {
    ++errors;
  }

  path.SetPath("/bookstore/book[starts-with(title,'Start')]");
  if(path.GetNumberOfMatches() != 1)
  {
    ++errors;
  }

  // --- "---------------------------------------------- - ------
  printf("Testing XPath/XPointer expressions             : %s\n", errors ? "ERROR" : "OK");
  return errors;
}

int TestXML(void)
{
  int errors = 0;

  errors += TestXPath();

  return errors;
}