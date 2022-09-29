/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: JSONPath.h
//
// BaseLibrary: Indispensable general objects and functions
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
#include "pch.h"
#include "JSONPath.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

JSONPath::JSONPath(bool p_originOne /*= false*/)
{
  m_origin = p_originOne ? 1 : 0;
}

JSONPath::JSONPath(JSONMessage* p_message,XString p_path,bool p_originOne /*= false*/)
         :m_message(p_message)
         ,m_path(p_path)
{
  m_origin = p_originOne ? 1 : 0;

  if(m_message)
  {
    m_message->AddReference();
  }
  Evaluate();
}

JSONPath::JSONPath(JSONMessage& p_message,XString p_path,bool p_originOne /*= false*/)
         :m_message(&p_message)
         ,m_path(p_path)
{
  m_origin = p_originOne?1:0;

  if(m_message)
  {
    m_message->AddReference();
  }
  Evaluate();
}

JSONPath::~JSONPath()
{
  if(m_message)
  {
    m_message->DropReference();
    m_message = nullptr;
  }
}

bool 
JSONPath::SetPath(XString p_path) noexcept
{
  m_path = p_path;
  return Evaluate();
}

bool 
JSONPath::SetMessage(JSONMessage* p_message) noexcept
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
JSONPath::Evaluate() noexcept
{
  // Start all-over
  Reset();

  // Check that we have work to do
  // message and path must be filled
  // path MUST start with the '$' symbol !!
  if(m_message == nullptr || m_path.IsEmpty() || m_path.GetAt(0) != '$')
  {
    m_status = JPStatus::JP_INVALID;
    m_errorInfo = "No message, path or the path does not start with a '$'";
    return false;
  }

  // Preset the start of searching
  XString parsing(m_path);
  m_searching = &m_message->GetValue();
  // Check for 'whole-document'
  if(parsing == "$")
  {
    m_results.push_back(m_searching);
    m_status = JPStatus::JP_Match_wholedoc;
    return true;
  }
  // Parse the pointer (left-to-right)
  while(ParseLevel(parsing));

  // See if we found something
  if(m_status != JPStatus::JP_None &&
     m_status != JPStatus::JP_INVALID)
  {
    return true;
  }
  // Not in a valid state!
  return false;
}

XString
JSONPath::GetPath() const
{
  return m_path;
}

JSONMessage* 
JSONPath::GetJSONMessage() const
{
  return m_message;
}

JPStatus
JSONPath::GetStatus() const
{
  return m_status;
}

unsigned
JSONPath::GetNumberOfMatches() const
{
  return (unsigned) m_results.size();
}

JSONvalue*
JSONPath::GetFirstResult() const
{
  if(!m_results.empty())
  {
    return m_results.front();
  }
  return nullptr;
}

JSONvalue*
JSONPath::GetResult(int p_index) const
{
  if(0 <= p_index && p_index < (int)m_results.size())
  {
    return m_results[p_index];
  }
  return nullptr;
}

XString
JSONPath::GetErrorMessage() const
{
  return m_errorInfo;
}

XString
JSONPath::GetFirstResultForceToString(bool p_whitespace /*=false*/) const
{
  JSONvalue* value = nullptr;

  if(m_status == JPStatus::JP_Match_wholedoc)
  {
    if(m_message)
    {
      value = &(m_message->GetValue());
    }
  }
  if(!m_results.empty())
  {
    if(m_status != JPStatus::JP_INVALID &&
       m_status != JPStatus::JP_None)
    {
      value = m_results.front();
    }
  }
  if(value)
  {
    return value->GetAsJsonString(p_whitespace,StringEncoding::ENC_Plain,0);
  }
  return XString();
}

//////////////////////////////////////////////////////////////////////////
// 
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

void  
JSONPath::Reset()
{
  m_status    = JPStatus::JP_None;
  m_delimiter = '/';
  m_recursive = false;
  m_searching = nullptr;
  m_errorInfo.Empty();
  m_results.clear();
}

// Last found value set the status!!!!
void
JSONPath::PresetStatus()
{
  if(!m_searching)
  {
    return;
  }
  switch(m_searching->GetDataType())
  {
    case JsonType::JDT_string:     m_status = JPStatus::JP_Match_string;     break;
    case JsonType::JDT_number_int: m_status = JPStatus::JP_Match_number_int; break;
    case JsonType::JDT_number_bcd: m_status = JPStatus::JP_Match_number_bcd; break;
    case JsonType::JDT_const:      m_status = JPStatus::JP_Match_constant;   break;
  }
}

bool
JSONPath::FindDelimiterType(XString& p_parsing)
{
  char ch = p_parsing.GetAt(0);
  if(ch == '.' || ch == '[')
  {
    m_delimiter = ch;
    return true;
  }
  return false;
}

// Split of the first token 
// -> ".word.one.two"
// -> "['word']['one']['two']
// -> ".word[2].three[5][6]
// -> ".."
// -> ".book.*"
// -> ".*"
bool
JSONPath::GetNextToken(XString& p_parsing,XString& p_token,bool& p_isIndex)
{
  char firstChar  = p_parsing.GetAt(0);
  char secondChar = p_parsing.GetAt(1);

  // Reset
  p_token.Empty();
  p_isIndex = false;

  // Special case: recursive
  if(firstChar == '.' && secondChar == '.')
  {
    m_recursive = true;
    p_parsing = p_parsing.Mid(1);
    firstChar = secondChar;
    secondChar = p_parsing.GetAt(1);
  }

  // Special case: Wildcard
  if(firstChar == '.' && secondChar == '*')
  {
    p_token = "*";
    p_parsing = p_parsing.Mid(2);
    return true;
  }

  // Find a word
  if(firstChar == '.' && secondChar != '[')
  {
    int pos = p_parsing.Find('.',1);
    int ind = p_parsing.Find('[');

    if ((ind > 0 && pos > 0 && ind < pos) ||
        (ind > 0 && pos < 0))
    {
      // This is a ".word[34].etc"
      // This is a ".word[34]"
      p_token   = p_parsing.Mid(1,ind - 1);
      p_parsing = p_parsing.Mid(ind);
    }
    else if(pos >= 2)
    {
      // This is a ".word.etc...."
      p_token   = p_parsing.Mid(1,pos - 1);
      p_parsing = p_parsing.Mid(pos);
    }
    else
    {
      // This is a ".word"
      p_token = p_parsing.Mid(1);
      p_parsing.Empty();
    }
    return true;
  }

  // Parse away a residue ".['something']"
  if(firstChar == '.' && secondChar == '[')
  {
    firstChar = secondChar;
    p_parsing = p_parsing.Mid(1);
  }

  // Find an index subscription
  if(firstChar == '[')
  {
    int pos = p_parsing.Find(']');
    if(pos >= 2)
    {
      p_token = p_parsing.Mid(1,pos - 1);
      p_parsing = p_parsing.Mid(pos + 1);
    }
    else
    {
      p_token = p_parsing.Mid(1);
      p_parsing.Empty();
    }
    p_token = p_token.Trim();
    if(p_token.GetAt(0) != '\'')
    {
      p_isIndex = true;
    }
    else
    {
      p_token = p_token.Trim('\'');
    }
    return true;
  }

  // Not a valid path expression
  return false;
}

void
JSONPath::ProcessWildcard()
{
  if(m_searching->GetDataType() == JsonType::JDT_array)
  {
    // All array elements are matched
    for(auto& val : m_searching->GetArray())
    {
      m_results.push_back(&val);
    }
    m_status = JPStatus::JP_Match_array;
    return;
  }
  if(m_searching->GetDataType() == JsonType::JDT_object)
  {
    // All object elements are matched (names are NOT included in the results!!)
    for(auto& pair : m_searching->GetObject())
    {
      m_results.push_back(&pair.m_value);
    }
    m_status = JPStatus::JP_Match_object;
    return;
  }
  // Ordinal values are matched
  m_results.push_back(m_searching);
  PresetStatus();
}

// Process token BNF: [start][[:end][:step]]
// Token has at lease one (1) ':' seperator
void
JSONPath::ProcessSlice(XString p_token)
{
  int starting = 0;
  int ending   = (int) m_searching->GetArray().size();
  int step     = 1;

  int firstColon = p_token.Find(':');
  if(firstColon > 0)
  {
    starting = atoi(p_token) - m_origin;
    p_token = p_token.Mid(firstColon + 1);
  }
  int secondColon = p_token.Find(':');
  if(secondColon > 0)
  {
    ending = atoi(p_token) - m_origin;
    step = atoi(p_token.Mid(secondColon + 1));
  }
  else if(secondColon == 0)
  {
    // Ending is still at array end.
    step = atoi(p_token.Mid(1)) - m_origin;
  }
  else // secondColon < 0
  {
    ending = atoi(p_token) - m_origin;
  }

  // Check array bounds and step direction
  if((step == 0) || (starting > ending))
  {
    // Invalid index subscription
    m_status = JPStatus::JP_INVALID;
    m_errorInfo = "Slice indexing is invalid step = 0 or start and ending are switched.";
    return;
  }

  // Reverse direction?
  if(step < 0)
  {
    --ending;
  }

  // Getting our slice result from the array
  for(int index = (step > 0) ? starting : ending;
                  (step > 0) ? (index < ending) : (index >= starting);
                   index += step)
  {
    if(0 <= index && index < (int)m_searching->GetArray().size())
    {
      m_results.push_back(&m_searching->GetArray()[index]);
    }
    else if(m_results.empty())
    {
      // Invalid index subscription
      m_status = JPStatus::JP_INVALID;
      m_errorInfo.Format("Invalid index subscription for array: %d",(int)index);
      return;
    }
  }
  m_status = JPStatus::JP_Match_array;
}

// Proces token BNF: num,num,num
// Tokan has least one ',' seperator
void 
JSONPath::ProcessUnion(XString p_token)
{
  while(!p_token.IsEmpty())
  {
    size_t index = atoi(p_token) - (size_t)m_origin;
    int pos = p_token.Find(',');
    if(pos > 0)
    {
      p_token = p_token.Mid(pos + 1);
      p_token = p_token.Trim();
    }
    else
    {
      // Last union member
      p_token.Empty();
    }

    if(0 <= index && index < m_searching->GetArray().size())
    {
      m_results.push_back(&m_searching->GetArray()[index]);
    }
    else if(m_results.empty())
    {
      // Invalid index subscription
      m_status = JPStatus::JP_INVALID;
      m_errorInfo.Format("Invalid index subscription in union operator: %d",(int) index);
      return;
    }
  }
  m_status = JPStatus::JP_Match_array;
}

bool
JSONPath::ParseLevel(XString& p_parsing)
{
  // Check if we are done parsing
  if(p_parsing.IsEmpty())
  {
    if(m_searching)
    {
      m_results.push_back(m_searching);
    }
    return false;
  }

  // Finding the first delimiter after the '$'
  // Must be an '.' or a '[' delimiter!
  if(p_parsing.GetAt(0) == '$')
  {
    p_parsing = p_parsing.Mid(1);
    if(!FindDelimiterType(p_parsing))
    {
      m_status = JPStatus::JP_INVALID;
      m_errorInfo = "Missing delimiter after the '$'. Must be '.' or '['";
      return false;
    }
  }

  XString token;
  bool isIndex = false;

  if(GetNextToken(p_parsing,token,isIndex))
  {
    // Check for special recursive operator
    if(token.IsEmpty() && m_recursive)
    {
      return true;
    }

    // Check for wildcard '*' (all)
    if(token == "*")
    {
      ProcessWildcard();
      return false;
    }

    if(isIndex)
    {
      // Do an array indexing action
      if(m_searching->GetDataType() == JsonType::JDT_array)
      {
        bool isSlice = token.Find(':') > 0 && p_parsing.IsEmpty();
        bool isUnion = token.Find(',') > 0 && p_parsing.IsEmpty();

        if(isSlice)
        {
          ProcessSlice(token);
          return false;
        }
        if(isUnion)
        {
          ProcessUnion(token);
          return false;
        }

        // Search on through this array
        size_t index = atoi(token) - (size_t) m_origin;
        if(index < 0)
        {
          // Negative index, take it from the end
          index = m_searching->GetArray().size() + index;
        }
        if(0 <= index && index < m_searching->GetArray().size())
        {
          m_searching = &m_searching->GetArray()[index];
          PresetStatus();
          return true;
        }
        // ERROR Index-out-of-bounds
        m_errorInfo.Format("Array index out of bounds: %d",(int)index);
      }
    }
    else
    {
      // Token is an object member
      m_searching = m_message->FindValue(m_searching,token,m_recursive);
      m_recursive = false; // Reset, use only once
      if(m_searching)
      {
        PresetStatus();
        return true;
      }
      // ERROR object-name-not-found
      m_errorInfo.Format("Object pair name [%s] not found",token.GetString());
    }
  }
  // No next token found. Incomplete path expression
  m_status = JPStatus::JP_INVALID;
  return false;
}

