/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLParser.h
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
#include "XMLMessage.h"
#include "ConvertWideString.h"

// To be translated entities in an XML string
class Entity
{
public:
  const char* m_entity;
  int         m_length;
  const char  m_char;
};

// Pointer type for processing
using uchar = unsigned char;

// Current number of entities recognized
constexpr auto NUM_ENTITY = 5;


class XMLParser
{
public:
  XMLParser() = default;
  XMLParser(XMLMessage* p_message);

  // Parse a complete XML message string
  void          ParseMessage(XString& p_message,WhiteSpace p_whiteSpace = WhiteSpace::PRESERVE_WHITESPACE);
  // Parse from a beginning node
  void          ParseForNode(XMLElement* p_node,XString& p_message,WhiteSpace p_whiteSpace = WhiteSpace::PRESERVE_WHITESPACE);
  // Setting message to UTF-8 encryption
  void          SetUTF8();

  // Print string with entities and optionally as UTF8 again
  static XString PrintXmlString (const XString& p_string, bool p_utf8 = false);
  static XString PrintJsonString(const XString& p_string, StringEncoding p_encoding = StringEncoding::ENC_Plain);

protected:
  // Set the internal error
  void          SetError(XmlError p_error,const uchar* p_text,bool p_throw = true);
  // Skipping whitespace
  void          SkipWhiteSpace();
  // Skip whitespace outside tags
  void          SkipOuterWhiteSpace();
  // Parsers
  void          ParseLevel();
  void          ParseDeclaration();
  void          ParseStylesheet();
  void          ParseComment();
  void          ParseDTD();
  void          ParseCDATA(bool p_append = false);
  void          ParseText();
  bool          ParseElement();
  virtual void  ParseAfterElement();
  // Getting an identifier
  bool          GetIdentifier(XString& p_identifier);
  // Get quoted string from message
  XString       GetQuotedString();
  // Get a character from message including '& translation'
  unsigned char ValueChar();
  // Conversion of xdigit to a numeric value
  int           XDigitToValue(int ch);
    // Is an alphanumeric char
  int           IsAlpha        (unsigned char p_char);
  int           IsAlphaNummeric(unsigned char p_char);
  // Need special token
  void          NeedToken(char p_token);
  // Create a new element
  void          MakeElement(XString& p_namespace,XString& p_name);

  // Message being parsed
  XMLMessage*   m_message    { nullptr };
  bool          m_utf8       { false   };
  unsigned      m_spaces     { 0 };
  unsigned      m_elements   { 0 };
  WhiteSpace    m_whiteSpace { WhiteSpace::PRESERVE_WHITESPACE };
  // Parsing was advanced to here
  uchar*        m_pointer    { nullptr };
  XMLElement*   m_element    { nullptr };
  XMLElement*   m_lastElement{ nullptr };
};

inline void 
XMLParser::SetUTF8()
{
  m_utf8 = true;
}
