/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XPath.h
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

//////////////////////////////////////////////////////////////////////////
//
// XPATH
//
// name                           -> Do a regular recursive 'FindElement'
// /name/other                    -> Selects the 'other' node under the 'name' root
// /name/other/house[4]           -> Selects the 4th 'house' node under the 'other'
// /myRoot/Houses//garage         -> Selects the first 'garage' node somewhere below the 'Houses' node
// //[@name]                      -> Selects all the nodes with the 'name' attribute
// /myRoot/Houses//[@type='loft'] -> Selects all the house nodes where the attribute 'type' has the value 'loft'
// /Bookstore/book[price>35.00]   -> Selects all the books with a price larger than 35 (dollar/euros)
// /Bookstore/book[price<100]     -> Selects all the books with a price lower than 100 (dollar/euros)
// /name/other/house[last()]      -> Selects the last 'house' node under 'other'
// /bookstore/book[contains(title,'Start')]    -> Selects books with 'Start' in the title
// /bookstore/book[begins-with(title,'Start')] -> Selects books where title begins with 'Start'
//
// List of functions (More to be implemented)
// last()             -> Last of equal children
// contains(element,string)
// starts-with(element,string)
//
// List of operators (More to be implemented)
// ='string'    -> Equals a string
// =12345       -> Equals a number
// >number      -> Larger than
// <number      -> Smaller than
// 
#pragma once
#include "XMLMessage.h"

// Indexing into child members is one-based (1 is the first child)
#define XPATH_ONE_BASED   1

enum class XPStatus
{
  XP_None       // Not parsed yet
 ,XP_Root       // Root (whole document) matches
 ,XP_Nodes      // Matching node(s) found
 ,XP_Invalid    // XPath does not evaluate to a valid node
};

class XPath
{
public:
  XPath() = default;
  XPath(XMLMessage* p_message,XString p_path);
 ~XPath();

  bool Evaluate() noexcept;

  // SETTERS
  bool  SetMessage(XMLMessage* p_message) noexcept;
  bool  SetPath(XString p_path) noexcept;

  // GETTERS
  XString      GetPath() const;
  XMLMessage*  GetXMLMessage() const;
  XPStatus     GetStatus() const;
  // Results from the path evaluation
  unsigned     GetNumberOfMatches() const;
  XMLElement*  GetFirstResult() const;
  XMLElement*  GetResult(int p_index) const;
  XString      GetErrorMessage() const;

private:
  // PARSING OF THE XPATH
  void    Reset();
  bool    ParseLevel(XString& p_parsing);
  XString GetToken  (XString& p_parsing);
  bool    GetNumber (XString& p_parsing,XString& p_token);
  void    NeedToken (XString& p_parsing,TCHAR p_token);
  bool    FindRecursivly(XMLElement* p_elem,XString token);
  bool    FindRecursivlyAttribute(XMLElement* p_elem,XString token,bool p_recursivly);

  // ParseLevel helpers (parsing got split up)
  bool    ParseLevelFindIndex (XString p_token);
  bool    ParseLevelFindAttrib(XString p_token,bool p_recurse);
  bool    ParseLevelFindNodes (XString p_token,bool p_recurse);
  bool    ParseLevelNameReduce(XString p_token,bool p_recurse);
  bool    ParseLevelAttrReduce(XString p_token);

  // Filter expressions
  bool    IsOperator(XString p_token);
  bool    IsFunction(XString p_token);
  bool    ParseLevelFunction(XString& p_parsing,XString p_function);
  bool    ParseLevelOperator(XString& p_parsing,XString p_operator,XString p_element,XString p_attribute);

  // DATA
  XString       m_path;
  XMLMessage*   m_message  { nullptr };
  XPStatus      m_status   { XPStatus::XP_None };
  bool          m_isString { false };     // Current token is a string
  XString       m_errorInfo;              // Latest error info

  XMLElement*   m_element  { nullptr };   // Where we are currently scanning
  XmlElementMap m_results;                // All the search results
};
