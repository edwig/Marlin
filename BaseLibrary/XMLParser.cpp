/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLParser.cpp
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
#include "XMLParser.h"
#include "WinFile.h"
#include "ConvertWideString.h"
#include "Namespace.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Special entities, so we do not mess with the XML structures
Entity g_entity[NUM_ENTITY] =
{
  { _T("&amp;"),  5, '&' },
  { _T("&lt;"),   4, '<' },
  { _T("&gt;"),   4, '>' },
  { _T("&quot;"), 6, '\"'},
  { _T("&apos;"), 6, '\''}
};

// Static function to be called from the outside
// Print string with entities and optionally as UTF8 again
XString 
XMLParser::PrintXmlString(const XString& p_string,bool p_utf8 /*=false*/)
{
  XString result;
  XString uncoded(p_string);

  if(p_utf8)
  {
    // Now encode MBCS to UTF-8 without a BOM
    uncoded = EncodeStringForTheWire(uncoded,_T("utf-8"));
  }

  _TUCHAR* pointer = (_TUCHAR*)uncoded.GetString();
  while(*pointer)
  {
    switch(*pointer)
    {
      // Chars with special XML meaning (entities)
      case '&': result += _T("&amp;");  break;
      case '<': result += _T("&lt;");   break;
      case '>': result += _T("&gt;");   break;
      case '\'':result += _T("&apos;"); break;
      case '\"':result += _T("&quot;"); break;
      // Performance optimization
      case ' ' :[[fallthrough]];
      case '\t':[[fallthrough]];
      case '\r':[[fallthrough]];
      case '\n':result += *pointer; break;
      // Standard chars here
      default:  if(*pointer < ' ')
                {
                  // All control chars under 0x20
                  // are restricted chars in the XML-standard
                  // And should be printed as entities
                  XString temp;
                  temp.Format(_T("&#%02d;"),(int)*pointer);
                  result += temp;
                }
                else
                {
                  // All ASCII from 0x21 to 0x7F and encoded UTF-8 chars
                  result += *pointer;
                }
                break;
    }
    ++pointer;
  }
  return result;
}

XString
XMLParser::PrintJsonString(const XString& p_string)
{
  _TUCHAR* buffer  = new _TUCHAR[2 * (size_t)p_string.GetLength() + 4];
  _TUCHAR* pointer = buffer;

  *pointer++ = '\"';

  for(int ind = 0; ind < p_string.GetLength(); ++ind)
  {
    TCHAR ch = p_string.GetAt(ind);
    switch(ch)
    {
      case '\"':  *pointer++ = '\\';
                  *pointer++ = '\"';
                  break;
      case '\\':  *pointer++ = '\\';
                  *pointer++ = '\\';
                  break;
      case '\b':  *pointer++ = '\\';
                  *pointer++ = 'b';
                  break;
      case '\f':  *pointer++ = '\\';
                  *pointer++ = 'f';
                  break;
      case '\n':  *pointer++ = '\\';
                  *pointer++ = 'n';
                  break;
      case '\r':  *pointer++ = '\\';
                  *pointer++ = 'r';
                  break;
      case '\t':  *pointer++ = '\\';
                  *pointer++ = 't';
                  break;
      default:    *pointer++ = ch;
                  break;
    }
  }
  // Closing
  *pointer++ = '\"';
  *pointer   = 0;

  XString result(buffer);
  delete [] buffer;
  return result;
}

//////////////////////////////////////////////////////////////////////////
//
// CLASS XMLParser
//
//////////////////////////////////////////////////////////////////////////

XMLParser::XMLParser(XMLMessage* p_message)
          :m_message(p_message)
{
}

void
XMLParser::ParseMessage(XString& p_message,WhiteSpace p_whiteSpace /*=PRESERVE_WHITESPACE*/)
{
  // Remember parsing mode
  m_whiteSpace = p_whiteSpace;

  // Check if we have something to do
  if(m_message == nullptr || p_message.IsEmpty())
  {
    SetError(XmlError::XE_EmptyXML,_T("Empty message"),false);
    return;
  }
  // Initialize the parsing pointer
  m_pointer = (_TUCHAR*) p_message.GetString();

  // MAIN PARSING LOOP
  try
  {
    // Parse an XML level
    ParseLevel();

    // Checks after parsing
    if(m_message->m_root->GetName().IsEmpty())
    {
      // Minimum requirement of an XML message
      SetError(XmlError::XE_NoRootElement,_T("Missing root element of XML message"));
    }
    if(*m_pointer)
    {
      SetError(XmlError::XE_ExtraText,(LPCTSTR)m_pointer);
    }
  }
  catch(XmlError& error)
  {
    // Error message text already set
    m_message->m_internalError = error;
  }
  catch(StdException& ex)
  {
    m_message->m_internalError = XmlError::XE_NotAnXMLMessage;
    if(m_message->m_internalErrorString.IsEmpty())
    {
      m_message->m_internalErrorString = ex.GetErrorMessage();
    }
  }

  // Conclusion of condensed level
  m_message->SetCondensed(m_spaces < m_elements);
}

// Parse from a beginning node
void
XMLParser::ParseForNode(XMLElement* p_node,XString& p_message,WhiteSpace p_whiteSpace /*=PRESERVE_WHITESPACE*/)
{
  m_lastElement = m_element = p_node;
  ParseMessage(p_message,p_whiteSpace);
}

void
XMLParser::ParseLevel()
{
  // MAIN PARSING LOOP
  while(*m_pointer)
  {
    // STEP 1: Skip whitespace
    SkipOuterWhiteSpace();

    // STEP 2: Check for end
    if(!*m_pointer)
    {
      return;
    }
    // STEP 3: Check for a node
    if(*m_pointer != '<')
    {
      SetError(XmlError::XE_NotAnXMLMessage,(LPCTSTR)m_pointer);
      return;
    }
    // STEP 4: One of five node types
    if(_tcsncmp((LPCTSTR)m_pointer,_T("<?xml"),5) == 0)
    {
      ParseDeclaration();
    }
    else if(_tcsncmp((LPCTSTR)m_pointer,_T("<?xml-stylesheet"),16) == 0)
    {
      ParseStylesheet();
    }
    else if(_tcsncmp((LPCTSTR)m_pointer,_T("<!--"),4) == 0)
    {
      ParseComment();
    }
    else if(_tcsncmp((LPCTSTR)m_pointer,_T("<![CDATA["),9) == 0)
    {
      ParseCDATA();
      // Scan for recursively restartable CDATA sections
      while(_tcsncmp((LPCTSTR)m_pointer,_T("<![CDATA["),9) == 0)
      {
        ParseCDATA(true);
      }
      return;
    }
    else if(_tcsncmp((LPCTSTR)m_pointer,_T("<!"),2) == 0)
    {
      ParseDTD();
    }
    else if(ParseElement())
    {
      ParseAfterElement();
      return;
    }
  }
}

// Set the internal error
void
XMLParser::SetError(XmlError p_error,LPCTSTR p_text,bool p_throw /*=true*/)
{
  if(m_message == nullptr)
  {
    return;
  }
  XString errorString(_T("ERROR parsing XML: "));
  if(m_element)
  {
    errorString.AppendFormat(_T(" Element [%s] : "),m_element->GetName().GetString());
  }
  // Setting the error
  errorString += p_text;
  m_message->m_internalErrorString = errorString;
  // Passing it on
  if(p_throw)
  {
    throw StdException((int)p_error);
  }
}

// Go past whitespace
void
XMLParser::SkipWhiteSpace()
{
  while(*m_pointer && *m_pointer < 128 && isspace(*m_pointer))
  {
    ++m_pointer;
  }
}

// Skip whitespace outside tags
void
XMLParser::SkipOuterWhiteSpace()
{
  while(*m_pointer && *m_pointer < 128)
  {
    if(isspace(*m_pointer))
    {
      ++m_spaces;
      ++m_pointer;
    }
    else break;
  }
}

// Parse the declaration of form:
// <?xml version="1.0" encoding="utf-8" space="preserve" standalone="yes"?>
void
XMLParser::ParseDeclaration()
{
  XString attributeName;

  // Initially set to empty.
  // The 'encoding' could have been set by the 
  // HTTP content type 'charset' attribute, so we leave it for now.
  m_message->m_version.Empty();
  m_message->m_standalone.Empty();

  // Skip over "<?xml"
  m_pointer += 5;

  SkipWhiteSpace();
  while(GetIdentifier(attributeName))
  {
    SkipWhiteSpace();
    NeedToken('=');
    XString value = GetQuotedString();
    SkipWhiteSpace();

    // Remove possibly "xml" namespace
    XString namesp = SplitNamespace(attributeName);

    if(attributeName.Compare(_T("version")) == 0)
    {
      if(value.Compare(_T("1.0")) == 0)
      {
        m_message->m_version = value;
      }
    }
    else if(attributeName.Compare(_T("encoding")) == 0)
    {
      m_encoding = value;
      // Most commonly 'UTF-8'
      if(CharsetToCodepage(value) < 0)
      {
        XString message;
        message.Format(_T("Unknown XML character encoding: %s"),value.GetString());
        SetError(XmlError::XE_NoError,message.GetString(),false);
      }
    }
    else if(attributeName.Compare(_T("standalone")) == 0)
    {
      if(value.CompareNoCase(_T("yes")) == 0 || value.CompareNoCase(_T("no")) == 0)
      {
        m_message->m_standalone = value;
      }
    }
    else
    {
      XString message;
      message.Format(_T("Unknown header attributes [%s=%s]"),attributeName.GetString(),value.GetString());
      SetError(XmlError::XE_HeaderAttribs,message.GetString());
    }
    if(!namesp.IsEmpty() && namesp.Compare(_T("xml")))
    {
      XString message;
      message.Format(_T("Unknown root namespace [%s] for attribute [%s]"),namesp.GetString(),attributeName.GetString());
      SetError(XmlError::XE_HeaderAttribs,message.GetString());
    }
  }
  // Skip over end of declaration
  NeedToken('?');
  NeedToken('>');
  SkipWhiteSpace();
}

// For now we only parse the type and href attributes
void
XMLParser::ParseStylesheet()
{
  m_message->m_stylesheetType.Empty();
  m_message->m_stylesheet.Empty();

  // Skip over "<?xml-stylesheet"
  m_pointer += 16;

  XString attributeName;

  SkipWhiteSpace();
  while(GetIdentifier(attributeName))
  {
    SkipWhiteSpace();
    NeedToken('=');
    XString value = GetQuotedString();
    SkipWhiteSpace();

    if(attributeName.Compare(_T("type")) == 0)
    {
      m_message->m_stylesheetType = value;
    }
    else if(attributeName.Compare(_T("href")) == 0)
    {
      m_message->m_stylesheet = value;
    }
    else
    {
      XString message;
      message.Format(_T("Unknown stylesheet attributes [%s=%s]"),attributeName.GetString(),value.GetString());
      SetError(XmlError::XE_HeaderAttribs,message.GetString());
    }
  }
  // Skip over end of the declaration
  NeedToken('?');
  NeedToken('>');
  SkipWhiteSpace();
}

void
XMLParser::ParseComment()
{
  // We throw comment's away real quick
  while(*m_pointer && _tcsncmp((LPCTSTR)m_pointer,_T("-->"),3))
  {
    ++m_pointer;
  }
  // Skip end of comment
  NeedToken('-');
  NeedToken('-');
  NeedToken('>');
  SkipWhiteSpace();
}

void
XMLParser::ParseCDATA(bool p_append /*= false*/)
{
  XString value;

  // Skip past "<![CDATA["
  m_pointer += 9;

  // Read everything until we find "]]>"
  // XML says a ']]' can occur in a CDATA section
  while(*m_pointer && _tcsncmp((LPCTSTR)m_pointer,_T("]]>"),3))
  {
    value += *m_pointer++;
  }
  NeedToken(']');
  NeedToken(']');
  NeedToken('>');
  // Add to current element
  if(m_lastElement)
  {
    if(p_append)
    {
      m_lastElement->SetValue(m_lastElement->GetValue() + value);
    }
    else
    {
      m_lastElement->SetValue(value);
      m_lastElement->SetType(XDT_CDATA);
    }
  }
  SkipWhiteSpace();
}

void
XMLParser::ParseText()
{
  XString value;

  while(*m_pointer && *m_pointer != '<')
  {
    _TUCHAR ch = ValueChar();
    if(m_whiteSpace == WhiteSpace::COLLAPSE_WHITESPACE && isspace(ch))
    {
      value += ' '; // Add exactly one whitespace char
      while(*m_pointer && isspace(*m_pointer))
      {
        ++m_pointer;
      }
    }
    else
    {
      value += ch;
    }
  }
  // Collapse begin and end
  if(m_whiteSpace == WhiteSpace::COLLAPSE_WHITESPACE)
  {
    value.Trim();
  }

  // Add to current element
  if(m_lastElement)
  {
    m_lastElement->SetValue(value);
  }
}

void
XMLParser::ParseDTD()
{
  // Setting the error, but we do not throw from here.
  SetError(XmlError::XE_DTDNotSupported,_T("DTD's are unsupported"),false);

  // Skip past a DTD part
  while(*m_pointer && *m_pointer != '>')
  {
    ++m_pointer;
  }
  NeedToken('>');
  SkipWhiteSpace();
}

// Parse element node
// Returns true if parsing level complete
bool
XMLParser::ParseElement()
{
  XString elementName;
  XString attributeName;
  WhiteSpace elemspace = m_whiteSpace;
  // Skip leading '<'
  m_pointer++;

  if(GetIdentifier(elementName))
  {
    // Creating an identifier
    XString namesp = SplitNamespace(elementName);
    MakeElement(namesp,elementName);

    if(isspace(*m_pointer))
    {
      SkipWhiteSpace();
      // See if we must get attributes
      while(GetIdentifier(attributeName))
      {
        SkipWhiteSpace();
        NeedToken('=');
        XString value = GetQuotedString();
        SkipWhiteSpace();

        // Adding an attribute
        m_message->SetAttribute(m_lastElement,attributeName,value);

        // In special case "[xml:]space", we must change whitespace preserving
        if(attributeName.Compare(_T("space")) == 0)
        {
          elemspace = value.Compare(_T("preserve")) == 0 ? WhiteSpace::PRESERVE_WHITESPACE 
                                                         : WhiteSpace::COLLAPSE_WHITESPACE;
        }
      }
    }
    if(*m_pointer && _tcsncmp((LPCTSTR)m_pointer,_T("/>"),2) == 0)
    {
      m_pointer += 2;
      return false;
    }
    if(*m_pointer == '>')
    {
      // Closing of the element
      m_pointer++;

      SkipOuterWhiteSpace();
      if(*m_pointer == '<')
      {
        // Push element and space-preserving and parse next level
        XMLElement* level = m_element;
        WhiteSpace  space = m_whiteSpace;

        m_whiteSpace = elemspace;
        m_element    = m_lastElement;
        ParseLevel();
        m_element    = level;
        m_whiteSpace = space;
      }
      else
      {
        ParseText();
      }
    }
    // Need to see ending of the element
    XString closing;
    NeedToken('<');
    NeedToken('/');
    if(GetIdentifier(closing))
    {
      XString closingNS = SplitNamespace(closing);
      if(elementName.Compare(closing) == 0)
      {
        if(namesp.Compare(closingNS))
        {
          XString error;
          error.Format(_T("Element [%s] has closing tag with different namespace."),elementName.GetString());
          SetError(XmlError::XE_MissingEndTag,error.GetString());
        }
        NeedToken('>');
        SkipOuterWhiteSpace();
        // Parsing level complete
        return _tcsncmp((LPCTSTR)m_pointer,_T("</"),2) == 0;
      }
      XString error;
      error.Format(_T("Element [%s] has incorrect closing tag [%s]"),elementName.GetString(),closing.GetString());
      SetError(XmlError::XE_MissingEndTag,error.GetString());
    }
    else
    {
      XString error;
      error.Format(_T("Missing end tag for element: %s"),elementName.GetString());
      SetError(XmlError::XE_MissingEndTag,error.GetString());
    }
  }
  // End of a list of elements
  if(*m_pointer == '/')
  {
    m_pointer--;
    return true;
  }
  XString error;
  error.Format(_T("Missing ending of XML element: %s"),elementName.GetString());
  SetError(XmlError::XE_MissingElement,error.GetString());
  return false;
}

// To do after successfully finding an element
void
XMLParser::ParseAfterElement()
{
  // Not used in the base class
}

// Getting an identifier
bool
XMLParser::GetIdentifier(XString& p_identifier)
{
  bool result = false;
  // Reset result
  p_identifier.Empty();

  // Identifiers starting with alpha, underscore or colon (no numbers!)
  TCHAR ch = *m_pointer;
  if(IsAlpha(ch) || ch == '_' || ch == ':')
  {
    result = true;
    p_identifier += ValueChar();
    while(*m_pointer)
    {
      ch = *m_pointer;
      if(IsAlphaNummeric(ch) || ch == '_' || ch == '-' || ch == ':' || ch == '.')
      {
        p_identifier += ValueChar();
      }
      else break;
    }
  }
  return result;
}

// Get quoted string from message
XString
XMLParser::GetQuotedString()
{
  XString result;

  SkipWhiteSpace();
  if(*m_pointer && (*m_pointer == '\"' || *m_pointer == '\''))
  {
    // Save the delimiter
    _TUCHAR delim = *m_pointer;

    m_pointer++;
    while(*m_pointer && *m_pointer != delim)  
    {
      result += ValueChar();
    }
    NeedToken(delim);
  }
  return result;
}

// Conversion of xdigit to a numeric value
int
XMLParser::XDigitToValue(int ch)
{
  if(ch >= '0' && ch <= '9')
  {
    return ch - '0';
  }
  if(ch >= 'A' && ch <= 'F')
  {
    return ch - 'A' + 10;
  }
  if(ch >= 'a' && ch <= 'f')
  {
    return ch - 'a' + 10;
  }
  return 0;
}

// Check special chars not supported in Windows-1252, not supported in ISO8859
int
XMLParser::UnicodeISO8859Check(int p_number)
{
  int number = p_number;

  // Catch special cases for quotes, not supported by ISO8859, but are in Windows-1252
  // If we do not translate these, we cannot store them in SQL-Server and Oracle WEISO8859
  // See: https://stackoverflow.com/questions/19109899/what-is-the-exact-difference-between-windows-1252-and-iso-8859-1
  //
  switch(number)
  {
    case  338: number = _T('O');  break;  // Scandinavian OE
    case  339: number = _T('o');  break;  // Scandinavian oe
    case  352: number = _T('S');  break;  // S-Hacek
    case  353: number = _T('s');  break;  // s-Hacek
    case  368: number = _T('Z');  break;  // Z-Hacek
    case  369: number = _T('z');  break;  // z-Hacek
    case  376: number = _T('Y');  break;  // Y-trema
    case  402: number = _T('f');  break;  // Dutch Florin
    case  710: number = _T('^');  break;  // Larger circonflex
    case  732: number = _T('~');  break;  // Larger tilde
    case 8211: number = _T('-');  break;  // Longer En-Dash (N)
    case 8212: number = _T('-');  break;  // Longer Em-Dash (M)
    case 8216: [[fallthrough]];
    case 8217: [[fallthrough]];
    case 8218: number = _T('\''); break;  // Forward/backward single quote
    case 8220: [[fallthrough]];
    case 8221: [[fallthrough]];
    case 8222: number = _T('\"'); break;  // Forward/backward double quote
    case 8224: number = _T('+');  break;  // Cross
    case 8225: number = _T('+');  break;  // Double Cross
    case 8226: number = _T('o');  break;  // Closed dot
    case 8230: number = _T('.');  break;  // ... Diaeresis
    case 8240: number = _T('%');  break;  // promille
    case 8249: number = _T('<');  break;  // Larger <
    case 8250: number = _T('>');  break;  // Larger >
    case 8364: number = _T('€');  break;  // Euro sign
  }
  return number;
}

// Get a character from message including '& translation'
_TUCHAR
XMLParser::ValueChar()
{
  if(*m_pointer == '&')
  {
    if(*(m_pointer + 1) == '#')
    {
      // Parse a number
      int number = 0;
      int radix  = 10;

      // Eventually a hexadecimal number
      m_pointer += 2;
      if(*m_pointer == 'x')
      {
        ++m_pointer;
        radix = 16;
      }

      // Read number
      while(*m_pointer && isxdigit(*m_pointer) && *m_pointer != ';')
      {
        number *= radix;
        number += XDigitToValue(*m_pointer++);
      }
      // Skip closing ';'
      NeedToken(';');
      // Result

      return (_TUCHAR) UnicodeISO8859Check(number);
    }
    // Search for known entities such as '&amp;' or '&quot;'
    for(unsigned ind = 0;ind < NUM_ENTITY; ++ind)
    {
      if(_tcsncmp((LPCTSTR)m_pointer,g_entity[ind].m_entity,g_entity[ind].m_length) == 0)
      {
        m_pointer += g_entity[ind].m_length;
        return g_entity[ind].m_char;
      }
    }
    // Unrecognized. Keep on going
    // Parse according to garbage-in/garbage-out principle
  }
  return *m_pointer++;
}

// Is an alphanumeric char
int
XMLParser::IsAlpha(_TUCHAR p_char)
{
  if(p_char < 128)
  {
    // Only defined for the 7 BIT ASCII range: 0 - 127
    return isalpha(p_char);
  }
  // Not defined. Assume we have a diacritic char!!
  return 1;
}

int
XMLParser::IsAlphaNummeric(_TUCHAR p_char)
{
  if(p_char < 128)
  {
    // Only defined for the 7 BIT ASCII range: 0 - 127
    return isalnum(p_char);
  }
  // Not defined. Assume we have a diacritic char!!
  return 1;
}

// Need special token
void 
XMLParser::NeedToken(_TUCHAR p_token)
{
  if(*m_pointer && *m_pointer == p_token)
  {
    m_pointer++;
    return;
  }
  XString error;
  error.Format(_T("Missing token [%c:%02X]"),p_token,(unsigned char)p_token);
  SetError(XmlError::XE_MissingToken,error.GetString());
}

// Make an element
void
XMLParser::MakeElement(XString& p_namespace,XString& p_name)
{
  // One element more
  ++m_elements;

  // Start our message at the root
  if(m_message->m_root->GetName().IsEmpty())
  {
    m_message->m_root->SetNamespace(p_namespace);
    m_message->m_root->SetName(p_name);
    m_element = m_message->m_root;
    m_lastElement = m_element;
    return;
  }
  // Adding to an existing element
  m_lastElement = m_message->AddElement(m_element,p_name,XDT_String,_T(""));
  if(m_lastElement == nullptr)
  {
    SetError(XmlError::XE_OutOfMemory,_T("OUT OF MEMORY"));
  }
  m_lastElement->SetNamespace(p_namespace);
}
