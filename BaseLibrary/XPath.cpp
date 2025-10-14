/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XPath.cpp
//
// Copyright (c) 2014-2025 ir. W.E. Huisman
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
#include "XPath.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

XPath::XPath(XMLMessage* p_message,XString p_path)
      :m_message(p_message)
      ,m_path(p_path)
{
  m_message->AddReference();

  Evaluate();
}

XPath::~XPath()
{
  if(m_message)
  {
    m_message->DropReference();
  }
}

bool
XPath::SetMessage(XMLMessage* p_message) noexcept
{
  // Replace message pointer
  if(m_message)
  {
    m_message->DropReference();
    m_message = nullptr;
  }
  m_message = p_message;

  // Use this message
  if(m_message)
  {
    m_message->AddReference();

    // Optionally evaluate the message
    if(!m_path.IsEmpty())
    {
      return Evaluate();
    }
  }
  return false;
}

bool
XPath::SetPath(XString p_path) noexcept
{
  m_path = p_path;
  if(m_message)
  {
    return Evaluate();
  }
  return false;
}

bool
XPath::Evaluate() noexcept
{
  // Start all-over
  Reset();

  // Check that we have work to do
  // message and path must be filled
  // path MUST start with the '/' symbol !!
  if (m_message == nullptr || m_path.IsEmpty())
  {
    m_status = XPStatus::XP_Invalid;
    m_errorInfo = _T("Message and path must both be filled in.");
    return false;
  }

  // Special case: Not a path, but a node to find
  if(m_path.GetAt(0) != '/')
  {
    m_results.push_back(m_message->FindElement(m_path));
    m_status = m_results.empty() ? XPStatus::XP_Invalid : XPStatus::XP_Nodes;
    return true;
  }

  // Preset the start of searching
  XString parsing(m_path);
  m_element = m_message->GetRoot();
  if (!m_element)
  {
    m_status = XPStatus::XP_Invalid;
    m_errorInfo = _T("Empty XML document: no root element!");
    return false;
  }

  // Special case: finding the whole document
  if(m_path.GetLength() == 1 && m_element)
  {
    m_status = XPStatus::XP_Root;
    return true;
  }
  // Parse the pointer (left-to-right)
  while (ParseLevel(parsing));

  if(m_element && m_results.empty())
  {
    m_results.push_back(m_element);
    m_element = nullptr;
  }

  // Did we find something?
  if(m_results.empty())
  {
    m_status = XPStatus::XP_Invalid;
    if(m_errorInfo.IsEmpty())
    {
      m_errorInfo = _T("Path not found in the XML document!");
    }
    return false;
  }
  m_status = XPStatus::XP_Nodes;
  return true;
}

XString
XPath::GetPath() const
{
  return m_path;
}

XMLMessage*
XPath::GetXMLMessage() const
{
  return m_message;
}

XPStatus
XPath::GetStatus() const
{
  return m_status;
}

unsigned
XPath::GetNumberOfMatches() const
{
  return (unsigned) m_results.size();
}

XMLElement*
XPath::GetFirstResult() const
{
  if(!m_results.empty())
  {
    return m_results.front();
  }
  return nullptr;
}

XMLElement*
XPath::GetResult(int p_index) const
{
  if((size_t)p_index >= 0 && (size_t) p_index < m_results.size())
  {
    return m_results[p_index];
  }
  return nullptr;
}

XString
XPath::GetErrorMessage() const
{
  return m_errorInfo;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

void
XPath::Reset()
{
  m_status = XPStatus::XP_None;
  m_errorInfo.Empty();
  m_results.clear();
  m_element  = nullptr;
  m_isString = false;
}

XString
XPath::GetToken(XString& p_parsing)
{
  XString token;

  if(p_parsing.GetAt(0) == '/')
  {
    p_parsing = p_parsing.Mid(1);
  }

  // Nothing found. XPath ends on a '/'
  if (p_parsing.IsEmpty())
  {
    return token;
  }

  // Possibly find a ' delimited string
  m_isString = false;
  if(p_parsing.GetAt(0) == '\'')
  {
    int pos = p_parsing.Find('\'',1);
    if(pos > 0)
    {
      token      = p_parsing.Mid(1,pos - 1);
      p_parsing  = p_parsing.Mid(pos + 1);
      m_isString = true;
      return token;
    }
  }

  // One time match
  int ch = p_parsing.GetAt(0);

  // Try parsing a number
  if(ch == '+' || ch == '-' || isdigit(ch))
  {
    if(GetNumber(p_parsing,token))
    {
      return token;
    }
  }

  // NOT an identifier
  if(ch != '_' && !isalpha(ch))
  {
    token = XString((TCHAR)ch,1);
    p_parsing = p_parsing.Mid(1);
    return token;
  }

  // Parse identifier
  int index = 1;
  while(isalnum(p_parsing.GetAt(index)) || p_parsing.GetAt(index) == '_' || p_parsing.GetAt(index) == '-')
  {
    ++index;
  }
  token     = p_parsing.Left(index);
  p_parsing = p_parsing.Mid(index);

  return token;
}

void
XPath::NeedToken(XString& p_parsing,TCHAR p_token)
{
  XString token = GetToken(p_parsing);
  if(token.GetLength() != 1 || token.GetAt(0) != p_token)
  {
    m_element = nullptr;
    m_results.clear();
    if(m_errorInfo.IsEmpty())
    {
      m_errorInfo.Format(_T("Expected a [%c] token!"), p_token);
    }
  }
}

bool
XPath::GetNumber(XString& p_parsing,XString& p_token)
{
  int ch = p_parsing.GetAt(0);
  if(ch == '-' || ch == '+')
  {
    if(!isdigit(p_parsing.GetAt(1)))
    {
      return false;
    }
    p_token.Empty();
    p_parsing = p_parsing.Mid(1);
    if(ch == '-')
    {
      p_token = XString((TCHAR)ch,1);
    }
  }

  ch = p_parsing.GetAt(0);
  while(isdigit(ch) || ch == '.')
  {
    p_token  += (TCHAR) p_parsing.GetAt(0);
    p_parsing = p_parsing.Mid(1);
    ch = p_parsing.GetAt(0);
  }

  if(toupper(p_parsing.GetAt(0)) == 'E')
  {
    // Getting the exponent
    p_token  += (TCHAR) p_parsing.GetAt(0);
    p_parsing = p_parsing.Mid(1);
    // Exponent sign (optional)
    ch = p_parsing.GetAt(0);
    if(ch == '-' || ch == '+')
    {
      p_token  += (TCHAR) p_parsing.GetAt(0);
      p_parsing = p_parsing.Mid(1);
    }
    // Exponent
    while(isdigit(p_parsing.GetAt(0)))
    {
      p_token  += (TCHAR) p_parsing.GetAt(0);
      p_parsing = p_parsing.Mid(1);
    }
  }
  return true;
}

bool
XPath::FindRecursivly(XMLElement* p_elem,XString token,bool* p_store)
{
  bool found = false;

  if(p_elem->GetName().Compare(token) == 0)
  {
    m_results.push_back(p_elem);
    found = true;
    if(*p_store)
    {
      m_element = p_elem;
      *p_store  = false;
    }
  }
  else
  {
    for(auto& elem : p_elem->GetChildren())
    {
      if(FindRecursivly(elem,token,p_store))
      {
        found = true;
      }
    }
  }
  return found;
}

bool
XPath::FindRecursivlyAttribute(XMLElement* p_elem,XString token,bool p_recursively)
{
  bool found = false;
  for(auto& elem : p_elem->GetChildren())
  {
    if(m_message->FindAttribute(elem,token))
    {
      m_results.push_back(elem);
    }
    if(p_recursively && !elem->GetChildren().empty())
    {
      if(FindRecursivlyAttribute(elem,token,true))
      {
        found = true;
      }
    }
  }
  return found;
}

bool
XPath::ParseLevel(XString& p_parsing)
{
  bool result = false;

  // Ready parsing?
  if(p_parsing.IsEmpty() || m_element == nullptr)
  {
    return result;
  }

  bool recursive = false;
  XString token = GetToken(p_parsing);
  if(!token.IsEmpty())
  {
    // See if we must continue with a 'free' search
    if(token == _T("/"))
    {
      recursive = true;
      token = GetToken(p_parsing);
    }
  }

  if(!token.IsEmpty())
  {
    XString element;
    XString attribute;

    // Do an index
    if(token == _T("["))
    {
      token = GetToken(p_parsing);
      if(isdigit(token.GetAt(0)))
      {
        // Perform indexing
        result = ParseLevelFindIndex(token);
        NeedToken(p_parsing,']');
        return result;
      }
      else if(token.GetAt(0) == '@')
      {
        // Get an attribute
        attribute = GetToken(p_parsing);
        result = ParseLevelFindAttrib(attribute,recursive);
      }
      else if(IsFunction(token))
      {
        result = ParseLevelFunction(p_parsing,token);
      }
      else
      {
        // Perform an element search
        result = ParseLevelNameReduce(element = token,false);

      }
      token = GetToken(p_parsing);
      if(token == _T("]"))
      {
        return result;
      }
      if(IsOperator(token))
      {
        // Last resort: expression reduce
        result = ParseLevelOperator(p_parsing,token,element,attribute);
      }
      NeedToken(p_parsing,']');
      return result;
    }
  }

  if(!token.IsEmpty())
  {
    // Find next node
    if(m_results.empty())
    {
      // Find the next node in the message
      result = ParseLevelFindNodes(token,recursive);
    }
    else
    {
      // Use the name to reduce the result set
      result = ParseLevelNameReduce(token,recursive);
    }
  }

  if(!result)
  {
    m_status = XPStatus::XP_Invalid;
    m_element = nullptr;
  }
  return result;
}

bool
XPath::ParseLevelFindIndex(XString p_token)
{
  int index = _ttoi(p_token) - XPATH_ONE_BASED;
  XMLElement* parent = m_element->GetParent();

  if(parent && (index >= 0 && index < (int)parent->GetChildren().size()))
  {
    m_element = parent->GetChildren()[index];
    m_results.clear();
    m_results.push_back(m_element);
    return true;
  }
  m_errorInfo = _T("Index out of bounds!");
  m_element = nullptr;
  m_results.clear();
  m_status = XPStatus::XP_Invalid;
  return false;
}

bool
XPath::ParseLevelFindAttrib(XString p_token,bool p_recurse)
{
  bool found = false;
  if(m_results.empty())
  {
    found = FindRecursivlyAttribute(m_element,p_token,p_recurse);
  }
  else
  {
    found = ParseLevelAttrReduce(p_token);
  }
  if(!found)
  {
    m_element = nullptr;
    m_results.clear();
    m_status = XPStatus::XP_Invalid;
    m_errorInfo.Format(_T("No elements with attribute [%s] found!"),p_token.GetString());
  }
  return found;
}

// Finding the next node '/nodename/'
bool
XPath::ParseLevelFindNodes(XString p_token,bool p_recurse)
{
  bool found = false;

  if(p_recurse)
  {
    bool store = true;
    found = FindRecursivly(m_element,p_token,&store);
    if(!found)
    {
      m_element = nullptr;
      m_results.clear();
      m_errorInfo = _T("Unsuccessful recursive find of: ") + p_token;
      m_status = XPStatus::XP_Invalid;
    }
  }
  else
  {
    // Straight path -> Find the next child element only
    m_element = m_message->FindElement(m_element,p_token,false);
    found = m_element != nullptr;
  }
  return found;
}

// Already results, next node finding '/nodename/' reduces
// the already found set of results
bool
XPath::ParseLevelNameReduce(XString p_token, bool p_recurse)
{
  XmlElementMap found;
  XmlElementMap::iterator it = m_results.begin();
  while(it != m_results.end())
  {
    XMLElement* child = m_message->FindElement(*it,p_token,p_recurse);
    if(child)
    {
      found.push_back(child);
    }
    it = m_results.erase(it);
  }
  if(!found.empty())
  {
    m_results.insert(m_results.begin(),found.begin(),found.end());
  }
  return !m_results.empty();
}

bool
XPath::ParseLevelAttrReduce(XString p_token)
{
  XmlElementMap::iterator it = m_results.begin();
  while(it != m_results.end())
  {
    if(m_message->FindAttribute(*it,p_token) == nullptr)
    {
      it = m_results.erase(it);
      continue;
    }
    ++it;
  }
  return !m_results.empty();
}

//////////////////////////////////////////////////////////////////////////
//
// Filter expressions
//
//////////////////////////////////////////////////////////////////////////

bool
XPath::IsOperator(XString p_token)
{
  return 
  p_token == _T("=") || 
  p_token == _T("<") || 
  p_token == _T(">") ;
}

bool 
XPath::IsFunction(XString p_token)
{
  return
  p_token.Compare(_T("last"))           == 0 ||
  p_token.Compare(_T("contains"))       == 0 ||
  p_token.Compare(_T("starts-with"))    == 0 ;
}

bool
XPath::ParseLevelFunction(XString& p_parsing,XString p_function)
{
  XMLElement* parent = m_element->GetParent();

  if(p_function.Compare(_T("last")) == 0)
  {
    NeedToken(p_parsing,'(');
    NeedToken(p_parsing,')');
    size_t total = parent->GetChildren().size();
    m_element = parent->GetChildren()[total - 1];
    return true;
  }
  else
  {
    // parse (element,text)
    NeedToken(p_parsing,'(');
    XString element = GetToken(p_parsing);
    NeedToken(p_parsing,',');
    XString text    = GetToken(p_parsing);
    NeedToken(p_parsing,')');

    for(auto& elem : parent->GetChildren())
    {
      XMLElement* compare = m_message->FindElement(elem,element,true);
      if(compare)
      {
        XString value = compare->GetValue();
        if(p_function.Compare(_T("contains")) == 0)
        {
          if(value.Find(text) >= 0)
          {
            m_results.push_back(elem);
          }
        }
        else if(p_function.Compare(_T("starts-with")) == 0)
        {
          if(value.Find(text) == 0)
          {
            m_results.push_back(elem);
          }
        }
      }
    }
    return !m_results.empty();
  }
  return false;
}

// Reduce the result set by applying a simple condition
bool
XPath::ParseLevelOperator(XString&  p_parsing
                         ,XString   p_operator
                         ,XString   p_element
                         ,XString   p_attribute)
{
  XString rightSide = GetToken(p_parsing);

  XmlElementMap::iterator it = m_results.begin();
  while(it != m_results.end())
  {
    XString leftValue;
    bool doErase(false);

    XMLElement* compare = m_message->FindElement(*it,p_element);

    if(!p_attribute.IsEmpty())
    {
      leftValue = m_message->GetAttribute(compare,p_attribute);
    }
    else
    {
      leftValue = compare->GetValue();
    }
    if(m_isString)
    {
      switch(p_operator.GetAt(0))
      {
        case '=': doErase = leftValue.Compare(rightSide) != 0; break;
        case '<': doErase = leftValue.Compare(rightSide) >= 0; break;
        case '>': doErase = leftValue.Compare(rightSide) <= 0; break;
        default:  return false;
      }
    }
    else
    {
      bcd numberRight(rightSide);
      bcd numberLeft (leftValue);

      switch(p_operator.GetAt(0))
      {
        case '=': doErase = numberLeft != numberRight; break;
        case '<': doErase = numberLeft >= numberRight; break;
        case '>': doErase = numberLeft <= numberRight; break;
        default:  return false;
      }
    }
    if(doErase)
    {
      it = m_results.erase(it);
      continue;
    }
    ++it;
  }
  return true;
}
