/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLParser.cpp
//
// Copyright (c) 2014-2021 ir. W.E. Huisman
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
#include "XMLParser.h"
#include "DefuseBOM.h"
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
  { "&amp;", 5, '&' },
  { "&lt;",  4, '<' },
  { "&gt;",  4, '>' },
  { "&quot;",6, '\"'},
  { "&apos;",6, '\''},
};

// Static function to be called from the outside
// Print string with entities and optionally as UTF8 again
CString 
XMLParser::PrintXmlString(const CString& p_string,bool p_utf8 /*=false*/)
{
  CString result;
  CString uncoded(p_string);

  if(p_utf8)
  {
    // Now encode MBCS to UTF-8 without a BOM
    uncoded = EncodeStringForTheWire(uncoded,"utf-8");
  }

  unsigned char* pointer = (unsigned char*)uncoded.GetString();
  while(*pointer)
  {
    switch(*pointer)
    {
      // Chars with special XML meaning (entities)
      case '&': result += "&amp;";  break;
      case '<': result += "&lt;";   break;
      case '>': result += "&gt;";   break;
      case '\'':result += "&apos;"; break;
      case '\"':result += "&quot;"; break;
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
                  CString temp;
                  temp.Format("&#%02d;",(int)*pointer);
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

CString
XMLParser::PrintJsonString(const CString& p_string,JsonEncoding p_encoding)
{
  unsigned char* buffer  = new unsigned char[2 * p_string.GetLength() + 4];
  unsigned char* pointer = buffer;

  *pointer++ = '\"';

  for(int ind = 0; ind < p_string.GetLength(); ++ind)
  {
    char ch = p_string.GetAt(ind);

    if(ch < 0x80)
    {
      switch(ch = p_string.GetAt(ind))
      {
        case '\"': *pointer++ = '\\';
                   *pointer++ = '\"';
                   break;
        case '\\': *pointer++ = '\\';
                   *pointer++ = '\\';
                   break;
        case '\b': *pointer++ = '\\';
                   *pointer++ = 'b';
                   break;
        case '\f': *pointer++ = '\\';
                   *pointer++ = 'f';
                   break;
        case '\n': *pointer++ = '\\';
                   *pointer++ = 'n';
                   break;
        case '\r': *pointer++ = '\\';
                   *pointer++ = 'r';
                   break;
        case '\t': *pointer++ = '\\';
                   *pointer++ = 't';
                   break;
        default:   *pointer++ = ch;
                   break;
      }
    }
    else
    {
      // Plainly add the character
      // Windows-1252 encoding or UTF-8 encoding
      *pointer++ = ch;
    }
  }
  // Closing
  *pointer++ = '\"';
  *pointer   = 0;

  CString result(buffer);
  delete [] buffer;
  switch(p_encoding)
  {
    case JsonEncoding::JENC_UTF8:     result = EncodeStringForTheWire(result,"utf-8");    break;
    case JsonEncoding::JENC_UTF16:    result = EncodeStringForTheWire(result,"utf-16");   break;
    case JsonEncoding::JENC_ISO88591: result = EncodeStringForTheWire(result,"iso88591"); break;
    case JsonEncoding::JENC_Plain:    // Fall through
    default:                          break;
  }
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
XMLParser::ParseMessage(CString& p_message,WhiteSpace p_whiteSpace /*=PRESERVE_WHITESPACE*/)
{
  // Remember parsing mode
  m_whiteSpace = p_whiteSpace;

  // Check if we have something to do
  if(m_message == nullptr || p_message.IsEmpty())
  {
    SetError(XmlError::XE_EmptyXML,(uchar*)"Empty message",false);
    return;
  }
  // Initialize the parsing pointer
  m_pointer = (uchar*) p_message.GetString();

  // Check for Byte-Order-Mark first
  BOMType bomType = BOMType::BT_NO_BOM;
  unsigned int skip = 0;
  BOMOpenResult bomResult = CheckForBOM(m_pointer,bomType,skip);

  if(bomResult != BOMOpenResult::BOR_NoBom)
  {
    if(bomType != BOMType::BT_BE_UTF8)
    {
      // cannot process these strings
      SetError(XmlError::XE_IncompatibleEncoding,(uchar*)"Incompatible Byte-Order-Mark encoding",false);
      return;
    }
    m_message->m_encoding = XMLEncoding::ENC_UTF8;
    m_message->m_sendBOM  = true;
    m_utf8 = true;
    // Skip past BOM
    m_pointer += skip;
  }

  // MAIN PARSING LOOP
  try
  {
    // Parse an XML level
    ParseLevel();

    // Checks after parsing
    if(m_message->m_root->GetName().IsEmpty())
    {
      // Minimum requirement of an XML message
      SetError(XmlError::XE_NoRootElement,(uchar*)"Missing root element of XML message");
    }
    if(*m_pointer)
    {
      SetError(XmlError::XE_ExtraText,m_pointer);
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
XMLParser::ParseForNode(XMLElement* p_node,CString& p_message,WhiteSpace p_whiteSpace /*=PRESERVE_WHITESPACE*/)
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
      SetError(XmlError::XE_NotAnXMLMessage,m_pointer);
      return;
    }
    // STEP 4: One of five node types
    if(strncmp((const char*)m_pointer,"<?xml",5) == 0)
    {
      ParseDeclaration();
    }
    else if(strncmp((const char*)m_pointer,"<?xml-stylesheet",16) == 0)
    {
      ParseStylesheet();
    }
    else if(strncmp((const char*)m_pointer,"<!--",4) == 0)
    {
      ParseComment();
    }
    else if(strncmp((const char*)m_pointer,"<![CDATA[",9) == 0)
    {
      ParseCDATA();
      // Scan for recursively restartable CDATA sections
      while(strncmp((const char*)m_pointer,"<![CDATA[",9) == 0)
      {
        ParseCDATA(true);
      }
      return;
    }
    else if(strncmp((const char*)m_pointer,"<!",2) == 0)
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
XMLParser::SetError(XmlError p_error,const uchar* p_text,bool p_throw /*=true*/)
{
  if(m_message == nullptr)
  {
    return;
  }
  CString errorString("ERROR parsing XML: ");
  if(m_element)
  {
    errorString.AppendFormat(" Element [%s] : ",m_element->GetName().GetString());
  }
  // Setting the error
  errorString += (const char*) p_text;
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
  CString attributeName;

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
    CString value = GetQuotedString();
    SkipWhiteSpace();

    // Remove possibly "xml" namespace
    CString namesp = SplitNamespace(attributeName);

    if(attributeName.Compare("version") == 0)
    {
      if(value.Compare("1.0") == 0)
      {
        m_message->m_version = value;
      }
    }
    else if(attributeName.Compare("encoding") == 0)
    {
      m_message->m_encoding = XMLEncoding::ENC_Plain;
      if(value.CompareNoCase("utf-8") == 0)
      {
        m_utf8 = true; 
        m_message->m_encoding = XMLEncoding::ENC_UTF8;
      }
      else if(value.Left(6).CompareNoCase("utf-16") == 0)
      {
        // Works for "UTF-16", "UTF-16LE" and "UTF-16BE" as of RFC 2781
        // Although one should only specify "utf-16" without the endianess
        m_message->m_encoding = XMLEncoding::ENC_UTF16;
      }
      else if(value.CompareNoCase("iso-8859-1") == 0)
      {
        m_message->m_encoding = XMLEncoding::ENC_ISO88591;
      }
      else
      {
        // If it was send in the current codepage, use that
        if(GetACP() == (UINT) CharsetToCodepage(value))
        {
          m_message->m_encoding = XMLEncoding::ENC_Plain;
        }
        else
        {
          CString message;
          message.Format("Unknown XML character encoding: %s",value.GetString());
          SetError(XmlError::XE_UnknownEncoding,(uchar*)message.GetString());
        }
      }
    }
    else if(attributeName.Compare("standalone") == 0)
    {
      if(value.CompareNoCase("yes") == 0 || value.CompareNoCase("no") == 0)
      {
        m_message->m_standalone = value;
      }
    }
    else
    {
      CString message;
      message.Format("Unknown header attributes [%s=%s]",attributeName.GetString(),value.GetString());
      SetError(XmlError::XE_HeaderAttribs,(uchar*)message.GetString());
    }
    if(!namesp.IsEmpty() && namesp.Compare("xml"))
    {
      CString message;
      message.Format("Unknown root namespace [%s] for attribute [%s]",namesp.GetString(),attributeName.GetString());
      SetError(XmlError::XE_HeaderAttribs,(uchar*)message.GetString());
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

  CString attributeName;

  SkipWhiteSpace();
  while(GetIdentifier(attributeName))
  {
    SkipWhiteSpace();
    NeedToken('=');
    CString value = GetQuotedString();
    SkipWhiteSpace();

    if(attributeName.Compare("type") == 0)
    {
      m_message->m_stylesheetType = value;
    }
    else if(attributeName.Compare("href") == 0)
    {
      m_message->m_stylesheet = value;
    }
    else
    {
      CString message;
      message.Format("Unknown stylesheet attributes [%s=%s]",attributeName.GetString(),value.GetString());
      SetError(XmlError::XE_HeaderAttribs,(uchar*)message.GetString());
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
  while(*m_pointer && strncmp((const char*)m_pointer,"-->",3))
  {
    m_pointer++;
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
  CString value;

  // Skip past "<![CDATA["
  m_pointer += 9;

  // Read everything until we find "]]>"
  // XML says a ']]' can occur in a CDATA section
  while(*m_pointer && strncmp((const char*)m_pointer,"]]>",3))
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
  CString value;

  while(*m_pointer && *m_pointer != '<')
  {
    unsigned char ch = ValueChar();
    if(m_whiteSpace == WhiteSpace::COLLAPSE_WHITESPACE && isspace(ch))
    {
      value += ' '; // Add exactly one whitespace char
      while(*m_pointer && isspace(*m_pointer)) ++m_pointer;
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
    if(m_utf8)
    {
      value = DecodeStringFromTheWire(value);
    }
    m_lastElement->SetValue(value);
  }
}

void
XMLParser::ParseDTD()
{
  // Setting the error, but we do not throw from here.
  SetError(XmlError::XE_DTDNotSupported,(uchar*)"DTD's are unsupported",false);

  // Skip past a DTD part
  while(*m_pointer && *m_pointer != '>')
  {
    m_pointer++;
  }
  NeedToken('>');
  SkipWhiteSpace();
}

// Parse element node
// Returns true if parsing level complete
bool
XMLParser::ParseElement()
{
  CString elementName;
  CString attributeName;
  WhiteSpace elemspace = m_whiteSpace;
  // Skip leading '<'
  m_pointer++;

  if(GetIdentifier(elementName))
  {
    // Creating an identifier
    CString namesp = SplitNamespace(elementName);
    MakeElement(namesp,elementName);

    if(isspace(*m_pointer))
    {
      SkipWhiteSpace();
      // See if we must get attributes
      while(GetIdentifier(attributeName))
      {
        SkipWhiteSpace();
        NeedToken('=');
        CString value = GetQuotedString();
        SkipWhiteSpace();

        // Adding an attribute
        m_message->SetAttribute(m_lastElement,attributeName,value);

        // In special case "[xml:]space", we must change whitespace preserving
        if(attributeName.Compare("space") == 0)
        {
          elemspace = value.Compare("preserve") == 0 ? WhiteSpace::PRESERVE_WHITESPACE 
                                                     : WhiteSpace::COLLAPSE_WHITESPACE;
        }
      }
    }
    if(*m_pointer && strncmp((const char*)m_pointer,"/>",2) == 0)
    {
      m_pointer += 2;
      return false;
    }
    if(*m_pointer == '>')
    {
      // Closing of the element
      m_pointer++;

      SkipOuterWhiteSpace();
      if(*m_pointer && *m_pointer == '<')
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
    CString closing;
    NeedToken('<');
    NeedToken('/');
    if(GetIdentifier(closing))
    {
      CString closingNS = SplitNamespace(closing);
      if(elementName.Compare(closing) == 0)
      {
        if(namesp.Compare(closingNS))
        {
          CString error;
          error.Format("Element [%s] has closing tag with different namespace.",elementName.GetString());
          SetError(XmlError::XE_MissingEndTag,(uchar*)error.GetString());
        }
        NeedToken('>');
        SkipOuterWhiteSpace();
        // Parsing level complete
        return strncmp((const char*)m_pointer,"</",2) == 0;
      }
      CString error;
      error.Format("Element [%s] has incorrect closing tag [%s]",elementName.GetString(),closing.GetString());
      SetError(XmlError::XE_MissingEndTag,(uchar*)error.GetString());
    }
    else
    {
      CString error;
      error.Format("Missing end tag for element: %s",elementName.GetString());
      SetError(XmlError::XE_MissingEndTag,(uchar*)error.GetString());
    }
  }
  // End of a list of elements
  if(*m_pointer == '/')
  {
    m_pointer--;
    return true;
  }
  CString error;
  error.Format("Missing ending of XML element: %s",elementName.GetString());
  SetError(XmlError::XE_MissingElement,(uchar*)error.GetString());
  return false;
}

// To do after successfully finding an element
void
XMLParser::ParseAfterElement()
{
  // Not used in the base clase
}

// Getting an identifier
bool
XMLParser::GetIdentifier(CString& p_identifier)
{
  bool result = false;
  // Reset result
  p_identifier.Empty();

  // Identifiers starting with alpha, underscore or colon (no numbers!)
  char ch = *m_pointer;
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
    if(m_utf8)
    {
      p_identifier = DecodeStringFromTheWire(p_identifier);
    }
  }
  return result;
}

// Get quoted string from message
CString
XMLParser::GetQuotedString()
{
  CString result;

  SkipWhiteSpace();
  if(*m_pointer && (*m_pointer == '\"' || *m_pointer == '\''))
  {
    // Save the delimiter
    char delim = *m_pointer;

    m_pointer++;
    while(*m_pointer && *m_pointer != delim)  
    {
      result += ValueChar();
    }
    NeedToken(delim);

    if(m_utf8)
    {
      result = DecodeStringFromTheWire(result);
    }
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

// Get a character from message including '& translation'
unsigned char
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
      return (unsigned char)(number);
    }
    // Search for known entities such as '&amp;' or '&quot;'
    for(unsigned ind = 0;ind < NUM_ENTITY; ++ind)
    {
      if(strncmp((const char*)m_pointer,g_entity[ind].m_entity,g_entity[ind].m_length) == 0)
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
XMLParser::IsAlpha(unsigned char p_char)
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
XMLParser::IsAlphaNummeric(unsigned char p_char)
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
XMLParser::NeedToken(char p_token)
{
  if(*m_pointer && *m_pointer == p_token)
  {
    m_pointer++;
    return;
  }
  CString error;
  error.Format("Missing token [%c:%02X]",p_token,(unsigned char)p_token);
  SetError(XmlError::XE_MissingToken,(uchar*)error.GetString());
}

// Make an element
void
XMLParser::MakeElement(CString& p_namespace,CString& p_name)
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
  m_lastElement = m_message->AddElement(m_element,p_name,XDT_String,"");
  if(m_lastElement == nullptr)
  {
    SetError(XmlError::XE_OutOfMemory,(uchar*)"OUT OF MEMORY");
  }
  m_lastElement->SetNamespace(p_namespace);
}
