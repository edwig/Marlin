/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLParserJSON.h
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
#include "XMLParser.h"

class SOAPMessage;
class JSONMessage;
class JSONvalue;

class XMLParserJSON : public XMLParser
{
public:
  XMLParserJSON();
  XMLParserJSON(XMLMessage* p_xml,JSONMessage* p_json);
 ~XMLParserJSON() = default;

 void  ParseMain(XMLElement* p_element,JSONvalue& p_value);
private:
  void ParseMainSOAP (XMLElement* p_element,JSONvalue& p_value);
  void ParseLevel    (XMLElement* p_element,JSONvalue& p_value,XString p_arrayName = "");
private:
  SOAPMessage* m_soap      { nullptr };
  bool         m_rootFound { false   };
};


