/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: JSONParser.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
#include "JSONParser.h"
#include "XMLParser.h"
#include "DefuseBOM.h"
#include "ConvertWideString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

JSONParser::JSONParser(JSONMessage* p_message)
           :m_message(p_message)
{
}

void
JSONParser::SetError(JsonError p_error,const char* p_text)
{
  if(m_message)
  {
    m_message->m_errorstate = true;
    m_message->m_lastError.Format("ERROR [%d] on line [%d] ",p_error,m_lines);
    m_message->m_lastError += p_text;
  }
  throw p_error;
}

void
JSONParser::ParseMessage(CString& p_message,bool& p_whitespace,JsonEncoding p_encoding /*=JsonEncoding::JENC_UTF8*/)
{
  // Check if we have something to do
  if(m_message == nullptr)
  {
    SetError(JsonError::JE_Empty,"Empty message");
    return;
  }

  // Initializing the parser
  m_pointer    = (uchar*) p_message.GetString();
  m_valPointer = m_message->m_value;
  m_lines      = 1;
  m_objects    = 0;
  m_utf8       = p_encoding == JsonEncoding::JENC_UTF8;

  // Check for Byte-Order-Mark first
  BOMType bomType = BOMType::BT_NO_BOM;
  unsigned int skip = 0;
  BOMOpenResult bomResult = CheckForBOM(m_pointer,bomType,skip);

  if(bomResult != BOMOpenResult::BOR_NoBom)
  {
    if(bomType != BOMType::BT_BE_UTF8)
    {
      // cannot process these strings
      SetError(JsonError::JE_IncompatibleEncoding,"Incompatible Byte-Order-Mark encoding");
      return;
    }
    m_message->m_encoding = JsonEncoding::JENC_UTF8;
    m_message->m_sendBOM  = true;
    m_utf8 = true;
    // Skip past BOM
    m_pointer += skip;
  }

  // See if we have an empty message string
  SkipWhitespace();
  if(p_message.IsEmpty())
  {
    // Regular empty message.
    // Nothing here but whitespace
    return;
  }

  // MAIN PARSING LOOP
  try
  {
    // Parse an XML level
    ParseLevel();

    if(m_pointer && *m_pointer)
    {
      SetError(JsonError::JE_ExtraText,(const char*)m_pointer);
    }
  }
  catch(JsonError& /*error*/)
  {
    // Error message text already set
  }
  
  // Preserving whitespace setting
  p_whitespace = (m_lines > m_objects);
}

void
JSONParser::SkipWhitespace()
{
  while(isspace(*m_pointer))
  {
    if(*m_pointer == '\n')
    {
      ++m_lines;
    }
    ++m_pointer;
  }
}

void
JSONParser::ParseLevel()
{
  SkipWhitespace();

  if(!ParseString())
  {
    if(!ParseNumber())
    {
      if(!ParseArray())
      {
        if(!ParseObject())
        {
          if(!ParseConstant())
          {
            if(m_pointer && *m_pointer)
            {
              SetError(JsonError::JE_UnknownString,"Non conforming JSON message text");
            }
          }
        }
      }
    }
  }

  SkipWhitespace();
}

// Parse constants
bool
JSONParser::ParseConstant()
{
  if(_strnicmp((const char*)m_pointer,"null",4) == 0)
  {
    m_valPointer->SetValue(JsonConst::JSON_NULL);
    m_pointer += 4;
  }
  else if(_strnicmp((const char*)m_pointer,"true",4) == 0)
  {
    m_valPointer->SetValue(JsonConst::JSON_TRUE);
    m_pointer += 4;
  }
  else if(_strnicmp((const char*)m_pointer,"false",5) == 0)
  {
    m_valPointer->SetValue(JsonConst::JSON_FALSE);
    m_pointer += 5;
  }
  else
  {
    return false;
  }
  return true;
}

// Gets me a string
CString
JSONParser::GetString()
{
  CString result;

  // Check that we have a string now
  if(*m_pointer != '\"')
  {
    SetError(JsonError::JE_NoString,"String expected but not found!");
  }
  ++m_pointer;

  while(*m_pointer && *m_pointer != '\"')
  {
    // See if we must do an escape
    if(*m_pointer == '\\')
    {
      ++m_pointer;
      int ch = *m_pointer++;
      switch(ch)
      {
        case '\"': [[fallthrough]];
        case '\\': [[fallthrough]];
        case '/':  result += (char)ch; break;
        case 'b':  result += '\b'; break;
        case 'f':  result += '\f'; break;
        case 'n':  result += '\n'; break;
        case 'r':  result += '\r'; break;
        case 't':  result += '\t'; break;
        case 'u':  result += UnicodeChar();
                   break;
        default:   SetError(JsonError::JE_IllString,"Ill formed string. Illegal escape sequence.");
                   break;
      }
    }
    else
    {
      result += ValueChar();
    }
  }
  // Skip past string's ending
  if(*m_pointer && *m_pointer == '\"')
  {
    ++m_pointer;
  }
  else
  {
    SetError(JsonError::JE_StringEnding,"String found without an ending quote!");
  }

  // Eventually decode UTF-8 encodings
  if(m_utf8)
  {
    result = DecodeStringFromTheWire(result);
  }
  return result;
}

// Conversion of xdigit to a numeric value
uchar
JSONParser::XDigitToValue(int ch)
{
  if(ch >= '0' && ch <= '9')
  {
    return (uchar) (ch - '0');
  }
  if(ch >= 'A' && ch <= 'F')
  {
    return (uchar) (ch - 'A' + 10);
  }
  if(ch >= 'a' && ch <= 'f')
  {
    return (uchar) (ch - 'a' + 10);
  }
  return 0;
}

// Get a character from message including UTF-8 translation
uchar
JSONParser::ValueChar()
{
  if (*m_pointer == '\n')
  {
    ++m_lines;
  }
  return *m_pointer++;
}

uchar
JSONParser::UnicodeChar()
{
  int  ind = 4;
  uchar ch = 0;
  while(*m_pointer && isxdigit(*m_pointer) && ind > 0)
  {
    ch *= 16; // Always radix 16 for JSON
    ch += XDigitToValue(*m_pointer);
    // Next char
    --ind;
    ++m_pointer;
  }
  if(ind)
  {
    SetError(JsonError::JE_Unicode4Chars, "Unicode escape consists of 4 hex characters");
  }
  return ch;
}

bool
JSONParser::ParseString()
{
  if(*m_pointer != '\"')
  {
    return false;
  }
  CString result = GetString();

  // Found our string
  m_valPointer->SetValue(result);
  return true;
}

// Try to parse an array
bool
JSONParser::ParseArray()
{
  // Must we do an array?
  if(*m_pointer != '[')
  {
    return false;
  }
  ++m_objects;
  // Go to begin and set value to array
  ++m_pointer;
  SkipWhitespace();
  m_valPointer->SetDatatype(JsonType::JDT_array);

  // Loop through all array values
  int elements = 0;
  while(*m_pointer)
  {
    // Put array element extra in the array
    JSONvalue value;
    m_valPointer->GetArray().push_back(value);

    // Check for an empty array
    if(*m_pointer == ']' && elements == 0)
    {
      ++m_pointer;
      return true;
    }
    ++elements;

    // Put value pointer on the stack and parse an array value
    JSONvalue* workPointer = m_valPointer;
    m_valPointer = &m_valPointer->GetArray().back();
    ParseLevel();
    m_valPointer = workPointer;

    // End of the array found?
    if(*m_pointer == ']')
    {
      ++m_pointer;
      break;
    }
    // Must now find ',' for next array value
    if(*m_pointer != ',')
    {
      SetError(JsonError::JE_ArrayElement,"Array element separator ',' expected!");
    }
    // Skip past array separator
    ++m_pointer;
  }
  return true;
}

// Try to parse an object
bool
JSONParser::ParseObject()
{
  // Must we do an object?
  if(*m_pointer != '{')
  {
    return false;
  }
  ++m_objects;
  // Go to begin and set value in the object
  ++m_pointer;
  SkipWhitespace();
  m_valPointer->SetDatatype(JsonType::JDT_object);

  // Loop through all object values
  int elements = 0;
  while(*m_pointer)
  {
    JSONpair value;
    m_valPointer->GetObject().push_back(value);
    JSONpair& property = m_valPointer->GetObject().back();

    // Check for an empty object
    if(*m_pointer == '}' && elements == 0)
    {
      ++m_pointer;
      return true;
    }
    ++elements;

    // Parse the name string;
    CString name = GetString();
    property.m_name = name;

    SkipWhitespace();
    if(*m_pointer != ':')
    {
      SetError(JsonError::JE_ObjNameSep,"Object's name-value separator ':' is missing!");
    }
    ++m_pointer;
    SkipWhitespace();

    // Put pointer on the stack and parse a value
    JSONvalue* workPointer = m_valPointer;
    m_valPointer = &property.m_value;
    ParseLevel();
    m_valPointer = workPointer;

    // End of the object found?
    if(*m_pointer == '}')
    {
      ++m_pointer;
      break;
    }
    // Must now find ',' for next object value
    if(*m_pointer != ',')
    {
      SetError(JsonError::JE_ObjectElement,"Object element separator ',' expected!");
    }
    // Skip past object separator
    ++m_pointer;
    SkipWhitespace();
  }
  return true;
}

// Parse a number
bool
JSONParser::ParseNumber()
{
  // See if we must parse a number
  if(*m_pointer != '-' && !isdigit(*m_pointer))
  {
    return false;
  }

  // Locals
  int     factor    = 1;
  __int64 number    = 0;
  int     intNumber = 0;
  double  dblNumber = 0.0;
  JsonType type = JsonType::JDT_number_int;

  // See if we find a negative number
  if(*m_pointer == '-')
  {
    factor = -1;
    ++m_pointer;
  }

  // Finding the integer part
  while(*m_pointer && isdigit(*m_pointer))
  {
    number *= 10; // JSON is always in radix 10!
    number += (*m_pointer - '0');
    ++m_pointer;
  }

  // Finding a broken number
  if(*m_pointer == '.' || tolower(*m_pointer) == 'e')
  {
    // Prepare
    if(*m_pointer == '.')
    {
      ++m_pointer;
      type = JsonType::JDT_number_dbl;
      dblNumber = (double)number;
      double decimPart = 1;

      while(*m_pointer && isdigit(*m_pointer))
      {
        decimPart *= 10;
        dblNumber += (*m_pointer - '0') / decimPart;
        ++m_pointer;
      }
    }
    // Do the exponential?
    if(tolower(*m_pointer) == 'e')
    {
      // Prepare
      ++m_pointer;
      int exp = 0;
      int fac = 1;

      // Negative exponential?
      if(*m_pointer == '-')
      {
        fac = -1;
        ++m_pointer;
      }

      // Find all exponential digits
      while(*m_pointer && isdigit(*m_pointer))
      {
        exp *= 10;
        exp += (*m_pointer - '0');
        ++m_pointer;
      }
      dblNumber *= pow(10.0,exp * fac);
    }

    // Do not forget the sign
    dblNumber *= factor;
  }
  else
  {
    number *= factor;
    if((number > MAXINT32) || (number < MININT32))
    {
      type = JsonType::JDT_number_dbl;
      dblNumber = (double)number;
    }
    else
    {
      intNumber = (int)number;
    }
  }

  // Preserving our findings
  if(type == JsonType::JDT_number_int)
  {
    m_valPointer->SetValue(intNumber);
  }
  else // if(type == JDT_number_dbl)
  {
    m_valPointer->SetValue(dblNumber);
  }
  return true;
}

//////////////////////////////////////////////////////////////////////////
//
// PARSER SOAPMessage -> JSONMessage
//
//////////////////////////////////////////////////////////////////////////

JSONParserSOAP::JSONParserSOAP(JSONMessage* p_message,SOAPMessage* p_soap)
               :JSONParser(p_message)
               ,m_soap(p_soap)
{
  // Construct the correct contents!!
  p_soap->GetSoapMessage();

  JSONvalue&  valPointer = *m_message->m_value;
  XMLElement& element    = *m_soap->m_paramObject;

  ParseMain(valPointer,element);
}

void
JSONParserSOAP::ParseMain(JSONvalue& p_valPointer,XMLElement& p_element)
{
  p_valPointer.SetDatatype(JsonType::JDT_object);
  JSONobject object;
  JSONpair pair;
  pair.m_name = p_element.GetName();
  pair.m_value.SetValue(JsonConst::JSON_NULL);
  object.push_back(pair);
  p_valPointer.SetValue(object);

  JSONpair& npair = p_valPointer.GetObject().back();
  JSONvalue& value = npair.m_value;

  if(!p_element.GetChildren().empty())
  {
    ParseLevel(value,p_element);
  }
}

void
JSONParserSOAP::ParseLevel(JSONvalue& p_valPointer,XMLElement& p_element)
{
  if(p_element.GetChildren().size())
  {
    CString arrayName;
    bool makeArray = ScanForArray(p_element,arrayName);

    if(makeArray)
    {
      CreateArray(p_valPointer,p_element,arrayName);
    }
    else
    {
      CreateObject(p_valPointer,p_element);
    }
  }
  else
  {
    // Create a pair
    CreatePair(p_valPointer,p_element);
  }
}

bool 
JSONParserSOAP::ScanForArray(XMLElement& p_element,CString& p_arrayName)
{
  // If the WSDL type tells us that there is an array af same elements
  // Under this element, we will convert to an array in JSON
  if(p_element.GetType() & XDT_Array)
  {
    return true;
  }

  // See if we must scan
  if(p_element.GetChildren().size() < 2)
  {
    return false;
  }

  CString sameName;
  bool first = true;

  // Scan the underlying elements
  for(auto& element : p_element.GetChildren())
  {
    if(first)
    {
      first = false;
      sameName = element->GetName();
    }
    else if(sameName.Compare(element->GetName()))
    {
      return false;
    }
  }
  // Yip, it's an array
  p_arrayName = sameName;
  return true;
}

void 
JSONParserSOAP::CreateArray(JSONvalue& p_valPointer,XMLElement& p_element,CString p_arrayName)
{
  p_valPointer.SetDatatype(JsonType::JDT_object);
  JSONarray jarray;
  JSONpair  pair;
  pair.m_name = p_arrayName;
  pair.m_value.SetValue(jarray);
  p_valPointer.GetObject().push_back(pair);

  JSONpair&  jpair = p_valPointer.GetObject().back();
  JSONarray& jar   = jpair.m_value.GetArray();

  for(auto& element : p_element.GetChildren())
  {
    // Put an object in the array and parse it
    JSONvalue value;
    jar.push_back(value);
    JSONvalue& val = jar.back();

    ParseLevel(val,*element);
  }
}

void 
JSONParserSOAP::CreateObject(JSONvalue& p_valPointer,XMLElement& p_element)
{
  p_valPointer.SetDatatype(JsonType::JDT_object);
  for(auto& element : p_element.GetChildren())
  {
    if(element->GetChildren().size())
    {
      JSONobject object;
      JSONpair   pair;
      pair.m_name = element->GetName();
      pair.m_value.SetValue(object);
      p_valPointer.GetObject().push_back(pair);

      JSONpair& npair = p_valPointer.GetObject().back();
      JSONvalue& value = npair.m_value;
      ParseLevel(value,*element);
    }
    else
    {
      ParseLevel(p_valPointer,*element);
    }
  }
}

void 
JSONParserSOAP::CreatePair(JSONvalue& p_valPointer,XMLElement& p_element)
{
  if(p_valPointer.GetDataType() == JsonType::JDT_object)
  {
    JSONpair pair;
    pair.m_name = p_element.GetName();
    CString value = p_element.GetValue();
    if(value.IsEmpty())
    {
      pair.m_value.SetValue(JsonConst::JSON_NULL);
    }
    else
    {
      pair.m_value.SetValue(value);
    }
    // Put in the surrounding object
    p_valPointer.GetObject().push_back(pair);
  }
  else
  {
    // Simple string in an array
    p_valPointer.SetDatatype(JsonType::JDT_string);
    CString value = p_element.GetValue();
    p_valPointer.SetValue(value);
  }
}
