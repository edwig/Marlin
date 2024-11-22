/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Namespace.cpp
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
#include "Namespace.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

// Compares two namespaces. Returns standard compare value
// "http://Name.Test.lower\something" equals "https://NAME.test.LOWER/SomeThing/"
//
int CompareNamespaces(XString p_namespace1, XString p_namespace2)
{
  // Make same case
  p_namespace1.MakeLower();
  p_namespace2.MakeLower();
  // Make all separators equal
  p_namespace1.Replace('\\','/');
  p_namespace2.Replace('\\','/');
  // Remove last closing separator
  p_namespace1.TrimRight('/');
  p_namespace2.TrimRight('/');
  // Removes different protocols
  if(p_namespace1.Left(5).Compare(_T("http:"))  == 0) p_namespace1 = p_namespace1.Mid(5);
  if(p_namespace2.Left(5).Compare(_T("http:"))  == 0) p_namespace2 = p_namespace2.Mid(5);
  if(p_namespace1.Left(6).Compare(_T("https:")) == 0) p_namespace1 = p_namespace1.Mid(6);
  if(p_namespace2.Left(6).Compare(_T("https:")) == 0) p_namespace2 = p_namespace2.Mid(6);
  
  // Return comparison
  return p_namespace1.Compare(p_namespace2);
}

// Can be used to split SOAPAction from the HTTP protocol
// or from the Soap envelope <Action>  node
//
// http://server/uri/command       -> "http://server/uri"       + "command"
// http://server/uri/some#command" -> "http://server/uri/some#" + "command"
// command                         -> ""                        + "command"
// 
bool SplitNamespaceAndAction(XString p_soapAction,XString& p_namespace,XString& p_action,bool p_nmsp_ends_in_slash /*= false*/)
{
  // Quick check whether it's filled
  if(p_soapAction.IsEmpty())
  {
    // No result returned
    return false;
  }
  // Remove leading and trailing quotes (can come from HTTP protocol!)
  p_soapAction.Trim('\"');
  // Change all separators and removed trailing '/' (see namespace standard)
  p_soapAction.Replace('\\','/');
  p_soapAction.TrimRight('/');

  // Namespace earlier on found. Just chop of the part after the namespace
  // .NET does this for SOAP 1.2. So snap that anus shut and continue on!
  if(!p_namespace.IsEmpty() && p_soapAction.Left(p_namespace.GetLength()) == p_namespace)
  {
    p_action = p_soapAction.Mid(p_namespace.GetLength());
    p_action.TrimLeft('/');
    return true;
  }

  // Check if it's part of an URI
  int pos = p_soapAction.ReverseFind('/');
  if (pos > 0)
  {
    int apos = pos + 1;
    // Check if it's an anchor to a namespace
    int hpos = p_soapAction.Find('#', pos);
    if (hpos >= 0)
    {
      pos = apos = ++hpos;
    }
    // Split namespace and action command name
    p_namespace = p_soapAction.Left(apos - (p_nmsp_ends_in_slash ? 0 : 1));
    p_action    = p_soapAction.Mid(apos);
  }
  else
  {
    // Clearly no namespace
    // Very ancient: SOAPAction is just the action name
    p_action = p_soapAction;
    return false;
  }
  return true;
}

// Concatenate namespace and action to a soapaction entry
// Can be used in HTTP and in SOAP messages
XString CreateSoapAction(XString p_namespace, XString p_action)
{
  // Quick check for POS
  if(p_namespace.IsEmpty())
  {
    return p_action;
  }
  // Sanitize namespace
  XString soapAction(p_namespace);
  soapAction.Replace('\\','/');
  if(soapAction.Right(1) != _T("/"))
  {
    soapAction += _T("/");
  }
  soapAction += p_action;

  return soapAction;
}

// Split namespace from an identifier
XString SplitNamespace(XString& p_name)
{
  XString namesp;
  int pos = p_name.Find(':');
  if(pos > 0)
  {
    namesp = p_name.Left(pos);
    p_name = p_name.Mid(pos + 1);
  }
  return namesp;
}
