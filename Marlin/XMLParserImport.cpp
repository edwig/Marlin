/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLParserImport.cpp
//
// Marlin Server: Internet server/client
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
#include "stdafx.h"
#include "FileBuffer.h"
#include "HTTPClient.h"
#include "XMLParserImport.h"
#include "XMLMessage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

XMLParserImport::XMLParserImport(XMLMessage* p_message)
                :XMLParser(p_message)
{
}

// After parsing a primary node in the XML document
// see if we just did the <import> node of an XSD scheme
void
XMLParserImport::ParseAfterElement()
{
  if(m_lastElement->GetName().CompareNoCase("import") == 0)
  {
    // Getting the attributes
    CString namesp   = m_message->GetAttribute(m_lastElement,"namespace");
    CString location = m_message->GetAttribute(m_lastElement,"schemaLocation");

    // Remove original <import> element
    m_message->DeleteElement(m_element,m_lastElement);

    if(!location.IsEmpty())
    {
      // Save current parser state on the stack
      XMLElement* element = m_element;
      uchar*      pointer = m_pointer;
      // replace this element by the import from the schema location
      ParseSchemaImport(location);
      // Return to this point in the parsing state
      m_element = element;
      m_pointer = pointer;
    }
  }
}

void
XMLParserImport::ParseSchemaImport(CString p_location)
{
  bool result = false;

  CString filename(p_location);
  CString protocol1 = filename.Left(8);
  CString protocol2 = filename.Left(7);
  protocol1.MakeLower();
  protocol2.MakeLower();
  if(protocol1 == "https://" || protocol2 == "http://")
  {
    result = ReadXSDFileFromURL(filename);
  }
  else
  {
    result = ReadXSDLocalFile(filename);
  }
  // Reset last element of the parser
  m_lastElement = nullptr;
}

bool
XMLParserImport::ReadXSDFileFromURL(CString p_url)
{
  bool result = false;

  HTTPClient client;
  FileBuffer buffer;

  if(client.Send(p_url,&buffer))
  {
    uchar* contents = nullptr;
    size_t size = 0;
    if(buffer.GetBufferCopy(contents,size))
    {
      CString message((LPCTSTR)contents);
      result = ReadXSD(message);
    }
    delete [] contents;
  }
  return result;
}

bool
XMLParserImport::ReadXSDLocalFile(CString p_filename)
{
  bool result = false;
  FileBuffer buf(p_filename);
  
  if(buf.ReadFile())
  {
    uchar* contents = nullptr;
    size_t size = 0;
    if(buf.GetBufferCopy(contents,size))
    {
      CString message((LPCTSTR)contents);
      result = ReadXSD(message);
    }
    delete [] contents;
  }
  return result;
}

bool 
XMLParserImport::ReadXSD(CString p_message)
{
  XMLMessage xsd;
  XMLParserImport parser(m_message);
  xsd.ParseMessage(&parser,p_message);
  
  // Check if XSD is legal XML
  if(xsd.GetInternalError() != XmlError::XE_NoError)
  {
    return false;
  }
  XMLElement* schema = xsd.FindElement("schema");
  if(schema == nullptr)
  {
    return false;
  }
  XMLElement* contents = xsd.GetElementFirstChild(schema);
  if(contents == nullptr)
  {
    return false;
  }
  CString toParse = xsd.PrintElements(contents);

  // Restart parsing from here
  // Adding this part of the XSD to the original XML document
  m_pointer = (uchar*) toParse.GetString();

  // MAIN PARSING LOOP
  try
  {
    // Parse an XML level
    ParseLevel();

    // Checks after parsing
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
  return (m_message->m_internalError == XmlError::XE_NoError);
}
