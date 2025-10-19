/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: JSONParser.h
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
#pragma once
#include "JSONMessage.h"
#include "SOAPMessage.h"
#include "Encoding.h"

enum class JsonError
{
  JE_Empty = 1
 ,JE_ExtraText
 ,JE_UnknownString
 ,JE_IllString
 ,JE_ArrayElement
 ,JE_NoString
 ,JE_StringEnding
 ,JE_ObjNameSep
 ,JE_ObjectElement
 ,JE_IncompatibleEncoding
 ,JE_Unicode4Chars
 ,JE_StreamingLimit
};

// Parsing a string to a JSON message
//
class JSONParser
{
public:
  explicit JSONParser(JSONMessage* p_message);
 ~JSONParser();

  // Parse a complete JSON message string
  void    ParseMessage(const XString& p_message,bool& p_whitespace);
private:
  void    SetError(JsonError p_error,LPCTSTR p_text,bool p_throw = true);
  void    SkipWhitespace();
  XString GetString();
  // Get a character from message including '& translation'
  _TUCHAR ValueChar();
  _TUCHAR XDigitToValue(int ch);
  _TUCHAR UnicodeChar();
  _TUCHAR UTF8Char();

  void    ParseLevel();
  bool    ParseConstant();
  bool    ParseString();
  bool    ParseNumber();
  bool    ParseArray();
  bool    ParseObject();

protected:
  JSONMessage* m_message    { nullptr };  // Receiving the errors for the parse
  _TUCHAR*     m_pointer    { nullptr };  // Pointer in string to parse
  JSONvalue*   m_valPointer { nullptr };  // Currently parsing value
  unsigned     m_lines      { 0 };        // Lines parsed
  unsigned     m_objects    { 0 };        // Objects/arrays parsed
  _TUCHAR*     m_scanString { nullptr };  // Temporary buffer to scan one string
  int          m_scanLength { 0 };        // Max length of a string to scan
};

// Parsing a SOAPMessage to a JSON Message
//
class JSONParserSOAP : public JSONParser
{
public:
  explicit JSONParserSOAP(JSONMessage* p_message);
  explicit JSONParserSOAP(JSONMessage* p_message,const SOAPMessage* p_soap);
  explicit JSONParserSOAP(JSONMessage* p_message,const XMLMessage*  p_xml);

  void Parse(XMLElement* p_element,bool p_forceerArray = false);
private:
  // Parse the message
  void ParseMain   (JSONvalue& p_valPointer,XMLElement& p_element);
  void ParseLevel  (JSONvalue& p_valPointer,XMLElement& p_element);
  // Create various parts of the JSON message
  void CreatePair  (JSONvalue& p_valPointer,XMLElement& p_element);
  void CreateObject(JSONvalue& p_valPointer,XMLElement& p_element);
  void CreateArray (JSONvalue& p_valPointer,XMLElement& p_element,XString p_arrayName);
  // Forward scanning
  int  ScanForArray(XMLElement& p_element);
  bool ScanForArray(XMLElement& p_element,XString& p_arrayName);
  void Trim(XString& p_value);

  bool m_forceerArray { false };
};

