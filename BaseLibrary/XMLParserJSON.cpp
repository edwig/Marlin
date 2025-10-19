/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLParserJSON.cpp
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
#include "pch.h"
#include "XMLParserJSON.h"
#include "SOAPMessage.h"
#include "JSONMessage.h"

//////////////////////////////////////////////////////////////////////////
//
// XMLParserJSON XML(SOAP) -> JSON
//
//////////////////////////////////////////////////////////////////////////

XMLParserJSON::XMLParserJSON()
{
}

XMLParserJSON::XMLParserJSON(XMLMessage* p_xml,const JSONMessage* p_json)
              :XMLParser(p_xml)
{
  m_soap = reinterpret_cast<SOAPMessage*>(p_xml);
  XMLElement* element = m_soap->GetParameterObjectNode();
  JSONvalue&  value   = p_json->GetValue();

  ParseMainSOAP(element,value);
}

void
XMLParserJSON::ParseMainSOAP(XMLElement* p_element,JSONvalue& p_value)
{
  // finding the main node in the JSON
  if(p_value.GetDataType() != JsonType::JDT_object)
  {
    return;
  }
  JSONpair&  pair  = p_value.GetObject()[0];
  JSONvalue& value = pair.m_value;

  // Detect the SOAP Envelope
  if(pair.m_name == _T("Envelope"))
  {
    if(value.GetDataType() != JsonType::JDT_object)
    {
      return;
    }
    pair = value.GetObject()[0];
    value = pair.m_value;
  
  }

  // Detect the SOAP Body
  if(pair.m_name == _T("Body"))
  {
    if(value.GetDataType() != JsonType::JDT_object)
    {
      return;
    }
    pair = value.GetObject()[0];
    value = pair.m_value;
  }

  // Remember the action name
  m_soap->SetParameterObject(pair.m_name);
  m_soap->SetSoapAction(pair.m_name);

  // Parse the message
  ParseLevel(p_element,value);
}

void
XMLParserJSON::ParseMain(XMLElement* p_element,JSONvalue& p_value)
{
  ParseLevel(p_element,p_value);
}

void
XMLParserJSON::ParseLevel(XMLElement* p_element,JSONvalue& p_value,const XString& p_arrayName /*=""*/)
{
  JSONobject* object  = nullptr;
  JSONarray*  array   = nullptr;
  XMLElement* element = nullptr;
  XString arrayName;
  XString value;

  switch(p_value.GetDataType())
  {
    case JsonType::JDT_object:      object = &p_value.GetObject();
                                    for(auto& pair : *object)
                                    {
                                      if(pair.m_value.GetDataType() == JsonType::JDT_array)
                                      {
                                        ParseLevel(p_element,pair.m_value,pair.m_name);
                                      }
                                      else
                                      {
                                        if(m_soap || m_rootFound)
                                        {
                                          element = m_soap->AddElement(p_element,pair.m_name,XDT_String,_T(""));
                                        }
                                        else
                                        {
                                          p_element->SetName(pair.m_name);
                                          element = p_element;
                                          m_rootFound = true;
                                        }
                                        ParseLevel(element,pair.m_value);
                                      }
                                    }
                                    break;
    case JsonType::JDT_array:       array = &p_value.GetArray();
                                    for(auto& val : *array)
                                    {
                                      element = m_soap->AddElement(p_element,p_arrayName,XDT_String,_T(""));
                                      ParseLevel(element,val);
                                    }
                                    break;
    case JsonType::JDT_string:      p_element->SetValue(p_value.GetString());
                                    break;
    case JsonType::JDT_number_int:  value.Format(_T("%d"),p_value.GetNumberInt());
                                    p_element->SetValue(value);
                                    break;
    case JsonType::JDT_number_bcd:  value = p_value.GetNumberBcd().AsString(bcd::Format::Bookkeeping,false,0);
                                    p_element->SetValue(value);
                                    break;
    case JsonType::JDT_const:       switch(p_value.GetConstant())
                                    {
                                      case JsonConst::JSON_NONE:  break; // Do nothing: empty string!!
                                      case JsonConst::JSON_NULL:  p_element->SetValue(_T(""));      break;
                                      case JsonConst::JSON_FALSE: p_element->SetValue(_T("false")); break;
                                      case JsonConst::JSON_TRUE:  p_element->SetValue(_T("true"));  break;
                                    }
                                    break;
  }
}
