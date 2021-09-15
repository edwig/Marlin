/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: JSONPointer.cpp
//
// Marlin Server: Internet server/client
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
#include "JSONPointer.h"
#include "CrackURL.h"

// Global value to reference non-existing array or object members
static char g_invalid_jp_value[] = "-";

JSONPointer::JSONPointer()
{
}

JSONPointer::JSONPointer(JSONMessage* p_message,CString p_pointer)
            :m_message(p_message)
            ,m_pointer(p_pointer)
{
  if(m_message)
  {
    m_message->AddReference();
  }
  Evaluate();
}

JSONPointer::~JSONPointer()
{
  if(m_message)
  {
    m_message->DropReference();
    m_message = nullptr;
  }
}

// Our main purpose: evaluate the pointer in the message
bool 
JSONPointer::Evaluate()
{
  // Start all-over
  Reset();

  // Check that we have work to do
  if(m_message == nullptr)
  {
    m_status = JPStatus::JP_INVALID;
    return false;
  }

  // Preset the first pointer
  m_value = &(m_message->GetValue());

  if(m_pointer.IsEmpty())
  {
    PresetFromJsonValue();
    m_status = JPStatus::JP_Match_wholedoc;
    return true;
  }
  // Parse the pointer (left-to-right)
  CString parsing(m_pointer);
  while(ParseLevel(parsing));

  // See if we found something
  if(m_status != JPStatus::JP_INVALID)
  {
    PresetFromJsonValue();
    return true;
  }
  // Not in a valid state!
  return false;
}

// SETTERS
bool
JSONPointer::SetPointer(CString p_pointer)
{
  m_pointer = p_pointer;
  if(m_message)
  {
    return Evaluate();
  }
  return false;
}

bool
JSONPointer::SetMessage(JSONMessage* p_message)
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
    if(!m_pointer.IsEmpty())
    {
      return Evaluate();
    }
  }
  return false;
}

// GETTERS
CString
JSONPointer::GetPointer()
{
  return m_pointer;
}

JSONMessage*
JSONPointer::GetJSONMessage()
{
  return m_message;
}

JPStatus
JSONPointer::GetStatus()
{
  return m_status;
}

JsonType
JSONPointer::GetType()
{
  return m_type;
}

// Result independent of the type!
JSONvalue*
JSONPointer::GetResultJSONValue()
{
  return m_value;
}

// Depending on type, return use of these!
CString
JSONPointer::GetResultString()
{
  if(m_string)
  {
    return *m_string;
  }
  return g_invalid_jp_value;
}

CString
JSONPointer::GetResultForceToString(bool p_whitespace /*=false*/)
{
  CString result(g_invalid_jp_value);
  JSONvalue* value = nullptr;

  // TBD
  if(m_status == JPStatus::JP_Match_wholedoc)
  {
    if(m_message)
    {
      value = &(m_message->GetValue());
    }
  }
  if(m_status != JPStatus::JP_INVALID &&
     m_status != JPStatus::JP_None)
  {
    value = m_value;
  }
  if(value)
  {
    result = value->GetAsJsonString(p_whitespace,JsonEncoding::JENC_Plain,0);
  }
  return result;
}

int
JSONPointer::GetResultInteger()
{
  if(m_number_int)
  {
    return *m_number_int;
  }
  return 0;
}

bcd
JSONPointer::GetResultBCD()
{
  if(m_number_bcd)
  {
    return *m_number_bcd;
  }
  return bcd();
}

JsonConst
JSONPointer::GetResultConstant()
{
  if(m_constant)
  {
    return *m_constant;
  }
  return JsonConst::JSON_NONE;
}

JSONobject*
JSONPointer::GetResultObject()
{
  return m_object;
}

JSONarray*
JSONPointer::GetResultArray()
{
  return m_array;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

void 
JSONPointer::Reset()
{
  // Reset the status
  m_status      = JPStatus::JP_None;
  // RESULT pointers
  m_type        = JsonType::JDT_const;
  m_value       = nullptr;
  m_constant    = nullptr;
  m_string      = nullptr;
  m_number_int  = nullptr;
  m_number_bcd  = nullptr;
  m_object      = nullptr;
  m_array       = nullptr;
}

// Preset pointers from JSON value
// Use the internal m_value.
void 
JSONPointer::PresetFromJsonValue()
{
  m_type = m_value->GetDataType();
  switch (m_type)
  {
    case JsonType::JDT_string:      m_string     = &(m_value->m_string);
                                    m_status     = JPStatus::JP_Match_string;
                                    break;
    case JsonType::JDT_const:       m_constant   = &(m_value->m_constant);
                                    m_status     = JPStatus::JP_Match_constant;
                                    break;
    case JsonType::JDT_number_int:  m_number_int = &(m_value->m_intNumber);
                                    m_status     = JPStatus::JP_Match_number_int;
                                    break;
    case JsonType::JDT_number_bcd:  m_number_bcd = &(m_value->m_bcdNumber);
                                    m_status     = JPStatus::JP_Match_number_bcd;
                                    break;
    case JsonType::JDT_object:      m_object     = &(m_value->m_object);
                                    m_status     = JPStatus::JP_Match_object;
                                    break;
    case JsonType::JDT_array:       m_array      = &(m_value->m_array);
                                    m_status     = JPStatus::JP_Match_array;
                                    break;
  }
}

void
JSONPointer::Unescape(CString& p_token)
{
  p_token.Replace("~1", "/");
  p_token.Replace("~0", "~");

  // Decode the URI hex chars
  p_token = CrackedURL::DecodeURLChars(p_token);
}

// Parse one extra level
// 1) Pointer is non-empty
// 2) Pointer and message and value are filled-in
bool
JSONPointer::ParseLevel(CString& p_parsing)
{
  // See if we are ready
  if(p_parsing.IsEmpty())
  {
    return false;
  }

  // Anything left must start with a '/'
  if(p_parsing.GetAt(0) != '/')
  {
    m_status = JPStatus::JP_INVALID;
    return false;
  }
  p_parsing = p_parsing.Mid(1);

  // Finding the token
  CString token;
  int nextpos = p_parsing.Find('/');
  if(nextpos > 0)
  {
    token = p_parsing.Left(nextpos);
    p_parsing = p_parsing.Mid(nextpos);
  }
  else
  {
    token = p_parsing;
    p_parsing.Empty();
  }

  // Unescape the token
  Unescape(token);

  // Check for value == array and token is number
  if(m_value->GetDataType() == JsonType::JDT_array && !token.IsEmpty() && isdigit(token.GetAt(0)))
  {
    unsigned index = atoi(token);
    if(index < (int)m_value->GetArray().size())
    {
      m_value = &(m_value->GetArray()[index]);
      return true;
    }
    // Index-out-of-bounds error
  }
  // Check for value == object and token is identifier
  else if(m_value->GetDataType() == JsonType::JDT_object)
  {
    for(auto& pair : m_value->GetObject())
    {
      if(pair.m_name.Compare(token) == 0)
      {
        m_value = &(pair.m_value);
        return true;
      }
    }
    // Token-not-found-in-object error
  }
  // Else error:
  // No more recursive JSON content found and still a token
  // so token cannot be found in the JSON
  m_status = JPStatus::JP_INVALID;
  return false;
}
