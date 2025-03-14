/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: JSONPath.h
//
// BaseLibrary: Indispensable general objects and functions
// 
// Copyright (c) 2014-2025 ir. W.E. Huisman
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

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
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
    m_errorInfo = _T("No message, path or the path does not start with a '$'");
    return false;
  }

  // Preset the start of searching
  XString parsing(m_path);
  m_searching = &m_message->GetValue();
  // Check for 'whole-document'
  if(parsing == _T("$"))
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
    return value->GetAsJsonString(p_whitespace,0);
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
JSONPath::FindDelimiterType(const XString& p_parsing)
{
  TCHAR ch = (TCHAR) p_parsing.GetAt(0);
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
JSONPath::GetNextToken(XString& p_parsing,XString& p_token,bool& p_isIndex,bool& p_isFilter)
{
  TCHAR firstChar  = (TCHAR) p_parsing.GetAt(0);
  TCHAR secondChar = (TCHAR) p_parsing.GetAt(1);

  // Reset
  p_token.Empty();
  p_isIndex = false;
  p_isFilter = false;

  // Special case: recursive
  if(firstChar == '.' && secondChar == '.')
  {
    m_rootWord = m_rootWord + firstChar + secondChar;
    m_recursive = true;
    p_parsing = p_parsing.Mid(1);
    firstChar = secondChar;
    secondChar = (TCHAR) p_parsing.GetAt(1);
  }

  // Special case: Wildcard
  if(firstChar == '.' && secondChar == '*')
  {
    m_rootWord += m_rootWord + firstChar + secondChar;
    p_token = _T("*");
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
    m_rootWord += p_token;
    return true;
  }

  // Parse away a residue ".['something']"
  if(firstChar == '.' && secondChar == '[')
  {
    firstChar = secondChar;
    p_parsing = p_parsing.Mid(1);
  }

  // Find an index subscription
  if(firstChar == '[' && secondChar != '?')
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

  // Find a filter expression
  if(firstChar == '[' && secondChar == '?')
  {
    int pos = FindMatchingBracket(p_parsing,0);
    if(pos >= 3)
    {
      p_token    = p_parsing.Mid(2,pos - 2);
      p_parsing  = p_parsing.Mid(pos + 3);
      p_token    = p_token.Trim();
      p_isFilter = true;
      return true;
    }
  }

  // Not a valid path expression
  return false;
}

void
JSONPath::ProcessWildcard(XString& p_parsing)
{
  XString parsing(p_parsing);
  if(m_searching->GetDataType() == JsonType::JDT_array)
  {
    // All array elements are matched
    for(auto& val : m_searching->GetArray())
    {
      if(p_parsing.IsEmpty())
      {
        m_results.push_back(&val);
        m_status = JPStatus::JP_Match_array;
      }
      else
      {
        m_searching = &val;
        parsing = p_parsing;
        while(ParseLevel(parsing));
      }
    }
    p_parsing = parsing;
    return;
  }
  if(m_searching->GetDataType() == JsonType::JDT_object)
  {
    // All object elements are matched (names are NOT included in the results!!)
    for(auto& pair : m_searching->GetObject())
    {
      if(p_parsing.IsEmpty())
      {
        m_results.push_back(&pair.m_value);
        m_status = JPStatus::JP_Match_object;
      }
      else
      {
        m_searching = &pair.m_value;
        parsing = p_parsing;
        while(ParseLevel(parsing));
      }
    }
    p_parsing = parsing;
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
    starting = _ttoi(p_token) - m_origin;
    p_token = p_token.Mid(firstColon + 1);
  }
  int secondColon = p_token.Find(':');
  if(secondColon > 0)
  {
    ending = _ttoi(p_token) - m_origin;
    step   = _ttoi(p_token.Mid(secondColon + 1));
  }
  else if(secondColon == 0)
  {
    // Ending is still at array end.
    step = _ttoi(p_token.Mid(1)) - m_origin;
  }
  else // secondColon < 0
  {
    ending = _ttoi(p_token) - m_origin;
  }

  // Check array bounds and step direction
  if((step == 0) || (starting > ending))
  {
    // Invalid index subscription
    m_status = JPStatus::JP_INVALID;
    m_errorInfo = _T("Slice indexing is invalid step = 0 or start and ending are switched.");
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
      m_errorInfo.Format(_T("Invalid index subscription for array: %d"),(int)index);
      return;
    }
  }
  m_status = JPStatus::JP_Match_array;
}

// Proces token BNF: num,num,num
// Token has least one ',' seperator
void 
JSONPath::ProcessUnion(XString p_token)
{
  while(!p_token.IsEmpty())
  {
    size_t index = _ttoi(p_token) - m_origin;
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

    if(index < m_searching->GetArray().size())
    {
      m_results.push_back(&m_searching->GetArray()[index]);
    }
    else if(m_results.empty())
    {
      // Invalid index subscription
      m_status = JPStatus::JP_INVALID;
      m_errorInfo.Format(_T("Invalid index subscription in union operator: %d"),(int) index);
      return;
    }
  }
  m_status = JPStatus::JP_Match_array;
}

void
JSONPath::ProcessFilter(XString p_token)
{  
  // Remove all spaces if not within single quotes
  XString token;
  int opening = -1;
  for(int i = 0; i < p_token.GetLength(); i++)
  {
    if(opening >= 0 && p_token.GetAt(i) != '\'')
    {
      token += (TCHAR) p_token.GetAt(i);
    } 
    else if(p_token.GetAt(i) == '\'')
    {
      token += (TCHAR) p_token.GetAt(i);
      opening = i;
    }
    else if(!isspace(p_token.GetAt(i)))
    {
      token += (TCHAR) p_token.GetAt(i);
    }
  }
  ProcessFilterTokenCharacters(token);
}

void 
JSONPath::ProcessFilterTokenCharacters(XString p_token)
{
  int pos = 0;
  while(pos < p_token.GetLength())
  {
    HandleLogicalNot(p_token,pos);
    pos++;
  }
}

int 
JSONPath::GetCurrentCharacter(XString p_token,int& p_pos)
{
  for(;;)
  {
    if(isspace(p_token.GetAt(p_pos)))
    {
      p_pos++;
    }
    else
    {
      return p_token.GetAt(p_pos);
    }
  }
}

int 
JSONPath::GetNextCharacter(XString p_token,const int& p_pos)
{
  int ch = -1;
  if(p_pos + 1 <= p_token.GetLength())
  {
    ch = p_token.GetAt(p_pos + 1);
  }
  return ch;
}

int 
JSONPath::GetEndOfPart(XString p_token,const int& p_pos)
{
  // End can either be ')', '&&', '||', ' ' or end of string
  int parenthesisPos = p_token.Find(')',p_pos);
  int andPos   = p_token.Find(_T("&&"),p_pos);
  int orPos    = p_token.Find(_T("||"),p_pos);

  int tempPos = p_token.GetLength();
  if(parenthesisPos > 0 &&
    (parenthesisPos < andPos   || andPos   < 0) &&
    (parenthesisPos < orPos    || orPos    < 0))
  {
    tempPos = parenthesisPos;
  }
  else if(andPos > 0 &&
    (andPos < parenthesisPos || parenthesisPos < 0) &&
    (andPos < orPos    || orPos    < 0))
  {
    tempPos = andPos;
  }
  else if(orPos > 0 &&
    (orPos < parenthesisPos || parenthesisPos < 0) &&
    (orPos < andPos   || andPos   < 0))
  {
    tempPos = orPos;
  }

  // return end of part encased in quotes if applicable
  if(WithinQuotes(p_token,p_pos,tempPos))
  {
    tempPos = p_token.GetLength();
  }

  return tempPos;
}

void
JSONPath::EvaluateFilter(Relation relation)
{
  if(m_results.empty())
  {
    if(m_searching->GetDataType() == JsonType::JDT_array)
    {
      for(int index = 0; index < (int)m_searching->GetArray().size(); index++)
      {
        bool contains(false);
        for(JSONpair pair : m_searching->GetArray().at(index).GetObject())
        {
          if(pair.m_name.Compare(relation.leftSide) == 0)
          {
            if(EvaluateFilterClause(relation,pair.m_value))
            {
              m_results.push_back(&m_searching->GetArray().at(index));
              m_status = JPStatus::JP_Match_array;
            };
          }
          else if(relation.leftSide.IsEmpty() && !relation.rightSide.IsEmpty())
          {
            if(relation.clause.Compare(_T("!")) == 0 || relation.clause.Compare(_T("~")) == 0)
            {
              if(pair.m_name.Compare(relation.rightSide) == 0)
              {
                contains = true;
              }
            }
          }
        }
        // If contains is false here, the current right side is not in the object
        if(!contains && relation.clause.Compare(_T("!")) == 0)
        {
          m_results.push_back(&m_searching->GetArray().at(index));
          m_status = JPStatus::JP_Match_array;
        }
        else if(contains && relation.clause.Compare(_T("~")) == 0)
        {
          m_results.push_back(&m_searching->GetArray().at(index));
          m_status = JPStatus::JP_Match_array;
        }
      }
    }
  }
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
      m_errorInfo = _T("Missing delimiter after the '$'. Must be '.' or '['");
      return false;
    }
  }

  XString token;
  bool isIndex = false;
  bool isFilter = false;

  if(GetNextToken(p_parsing,token,isIndex,isFilter))
  {
    // Check for special recursive operator
    if(token.IsEmpty() && m_recursive)
    {
      return true;
    }

    // Check for wildcard '*' (all)
    if(token == _T("*"))
    {
      ProcessWildcard(p_parsing);
      return false;
    }

    if(isIndex)
    {
      // Do an array indexing action
      if(m_searching->GetDataType() == JsonType::JDT_array)
      {
        bool isSlice = token.Find(':') > 0 && p_parsing.IsEmpty();
        bool isUnion = token.Find(',') > 0 && p_parsing.IsEmpty();

        // Check correct use of brackets
        int found = -1;
        for(int i = 0; i < token.GetLength(); i++)
        {
          if(token.GetAt(i) == '[')
          {
            found = FindMatchingBracket(token,i);
            if(found < 0)
            {
              m_errorInfo = _T("Could not parse index filter in JsonPath: ") + m_path;
              return false;
            }
          }
        }

        // We do allow trailing brackets. So remove trailing ']'
        if(found >= 0)
        {
          p_parsing.Remove(']');
        }
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
        int index = _ttoi(token) - m_origin;
        if(index < 0)
        {
          // Negative index, take it from the end
          index = (int)(m_searching->GetArray().size()) + index;
        }
        if(0 <= index && index < (int)m_searching->GetArray().size())
        {
          m_searching = &m_searching->GetArray()[index];
          PresetStatus();
          return true;
        }
        // ERROR Index-out-of-bounds
        m_errorInfo.Format(_T("Array index out of bounds: %d"),(int)index);
      }
    }
    else if(isFilter)
    {
      if(m_searching->GetDataType() == JsonType::JDT_array)
      {
        // Remove outer brackets
        if(token.GetAt(0) == '(' && token.GetAt(token.GetLength() - 1) == ')')
        {
          token = token.Mid(1,token.GetLength() - 2);
        }
        ProcessFilter(token);
        if(m_bracketStack.size() > 0)
        {
          m_errorInfo.Format(_T("Missing ')' in filter: ") + token);
        }
        return false;
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
      m_errorInfo.Format(_T("Object pair name [%s] not found"),token.GetString());
    }
  }
  // No next token found. Incomplete path expression
  m_status = JPStatus::JP_INVALID;
  return false;
}

void 
JSONPath::HandleLogicalNot(XString p_token,int& p_pos)
{
  HandleRelationOperators(p_token,p_pos);
  // Handle logical Not
  if(GetCurrentCharacter(p_token,p_pos) == '!' && GetNextCharacter(p_token,p_pos) != '=')
  {
    int tempPos = GetEndOfPart(p_token,p_pos);

    // Looks like !@.isbn
    XString rightSide = p_token.Mid(p_pos,tempPos - p_pos);
    p_pos = tempPos - 1;
    rightSide.Replace(_T("!@."),_T(""));
    rightSide = rightSide.Trim();

    Relation relation;
    relation.clause = _T("!");
    relation.leftSide.Empty();
    relation.rightSide = rightSide;

    // Now evaluate
    EvaluateFilter(relation);
  }
  else if(GetCurrentCharacter(p_token,p_pos) == '@' && GetNextCharacter(p_token,p_pos) == '.')
  {
    // Check for @.isbn
    int tempPos = GetEndOfPart(p_token,p_pos);

    XString rightSide = p_token.Mid(0,tempPos);
    // If the rightSide does not contain an operator, it looks like @.isbn
    bool containsOperator{ false };
    for(int i = 0; i < rightSide.GetLength(); i++)
    {
      XString op = DetermineRelationalOperator(rightSide,i);
      if(!op.IsEmpty())
      {
        containsOperator = true;
        break;
      }
    }

    if(!containsOperator)
    {
      p_pos = tempPos - 1;
      rightSide.Replace(_T("@."),_T(""));
      rightSide = rightSide.Trim();

      Relation relation;
      relation.clause    = _T("~");
      relation.leftSide  = _T("");
      relation.rightSide = rightSide;

      // Now evaluate
      EvaluateFilter(relation);
    }
  };
}

void 
JSONPath::HandleRelationOperators(XString p_token,int& p_pos)
{
  HandleLogicalAnd(p_token,p_pos);
  // Handle relational operators
  XString op = DetermineRelationalOperator(p_token,p_pos);
  if(!op.IsEmpty())
  {
    p_pos++;

    // Look for the end of the current filterpart
    int tempPos = GetEndOfPart(p_token,p_pos);

    // Determine left side
    XString leftSide = p_token.Mid(0,p_pos);
    leftSide.Replace(op,_T(""));
    leftSide = leftSide.Trim();
    leftSide.Replace(_T("@."),_T(""));

    // Determine right side
    XString rightSide = p_token.Mid(p_pos,tempPos - p_pos).Trim();
    p_pos = tempPos - 1;

    rightSide.Replace(_T("@."),_T(""));
    rightSide.Replace(_T("'"), _T(""));

    Relation relation;
    relation.clause    = op;
    relation.leftSide  = leftSide;
    relation.rightSide = rightSide;

    // Now evaluate
    EvaluateFilter(relation);
  };
}

void 
JSONPath::HandleLogicalAnd(XString p_token,int& p_pos)
{
  HandleLogicalOr(p_token,p_pos);
  // Processing logical AND
  if(GetCurrentCharacter(p_token,p_pos) == '&')
  {
    if(GetNextCharacter(p_token,p_pos) == '&')
    {
      p_pos += 2;
      XString rightSide;
      rightSide = p_token.Mid(p_pos,p_token.GetLength()).Trim();

      // Evaluate the part after "&&" separately
      JSONPath path(m_message,XString(_T("$")) + m_rootWord + _T("[?(") + rightSide + _T(")]"));
      JPResults newResults;

      bool exist = false;
      // We add the result from path.m_results only if it was present
      if(path.GetNumberOfMatches() == 1)
      {
        for(const JSONvalue* val : m_results)
        {
          if(val == path.GetFirstResult())
          {
            exist = true;
            break;
          }
        }
        if(exist)
        {
          newResults.push_back(path.GetFirstResult());
        }
      }
      else if(path.GetNumberOfMatches() > 1)
      {
        for(int index = 0; index < (int)path.GetNumberOfMatches(); ++index)
        {
          exist = false;
          for(const JSONvalue* val : m_results)
          {
            if(val == path.GetResult(index))
            {
              exist = true;
              break;
            }
          }
          if(exist)
          {
            newResults.push_back(path.GetResult(index));
          }
        }
      }
      m_results = newResults;
      p_pos += rightSide.GetLength();
    }
  }
}

void 
JSONPath::HandleLogicalOr(XString p_token,int& p_pos)
{
  HandleBrackets(p_token,p_pos);
  // Handle the logical "or"
  if(GetCurrentCharacter(p_token,p_pos) == '|')
  {
    if(GetNextCharacter(p_token,p_pos) == '|')
    {
      p_pos += 2;
      XString rightSide;
      rightSide = p_token.Mid(p_pos,p_token.GetLength()).Trim();

      // Evaluate the part after the "||"
      JSONPath path(m_message,_T("$") + m_rootWord + _T("[?(") + rightSide + _T(")]"));

      // Result from path.m_reuslts only to be appended if not already present
      bool exist = false;
      if(path.GetNumberOfMatches() == 1)
      {
        for(const JSONvalue* val : m_results)
        {
          if(val == path.GetFirstResult())
          {
            exist = true;
            break;
          }
        }
        if(!exist)
        {
          m_results.push_back(path.GetFirstResult());
        }
      }
      else if(path.GetNumberOfMatches() > 1)
      {
        for(int index = 0; index < (int)path.GetNumberOfMatches(); ++index)
        {
          exist = false;
          for(const JSONvalue* val : m_results)
          {
            if(val == path.GetResult(index))
            {
              exist = true;
              break;
            }
          }
          if(!exist)
          {
            m_results.push_back(path.GetResult(index));
          }
        }
      }
      p_pos += rightSide.GetLength();
    }
  }
}

void 
JSONPath::HandleBrackets(XString p_token,int& p_pos)
{
  if(GetCurrentCharacter(p_token,p_pos) == '(')
  {
    m_bracketStack.push((TCHAR)GetCurrentCharacter(p_token,p_pos));
    if(GetNextCharacter(p_token,p_pos) != '(')
    {
      int closingBracket = FindMatchingBracket(p_token,p_pos);
      if(closingBracket > p_pos)
      {
        p_pos++;
        XString token = p_token.Mid(p_pos,closingBracket - 1);
        ProcessFilter(token);
        p_pos += token.GetLength() - 1;
      }
    }
  }
  else if(GetCurrentCharacter(p_token,p_pos) == ')')
  {
    if(m_bracketStack.size() > 0)
    {
    m_bracketStack.pop();
  }
    else
    {
      m_errorInfo = _T("Unexpected token ')'");
    }
  }
}

// Check if a given char is within quotes
bool 
JSONPath::WithinQuotes(XString p_token,int p_pos,int p_charPos)
{
  int openingSingleQoute = p_token.Find('\'',p_pos);
  int endingSingleQoute  = openingSingleQoute >= 0 ? p_token.Find('\'',openingSingleQoute + 1) : -1;
  if(endingSingleQoute > openingSingleQoute)
  {
    return p_charPos > openingSingleQoute && p_charPos < endingSingleQoute;
  }
  return false;
}

int
JSONPath::FindMatchingBracket(const XString& p_string, int p_bracketPos)
{
  TCHAR bracket = p_string[p_bracketPos];
  TCHAR match   = 0;
  bool  reverse = false;

  switch (bracket)
  {
    case '(':    match = ')';    break;
    case '{':    match = '}';    break;
    case '[':    match = ']';    break;
    case '<':    match = '>';    break;
    case ')':    reverse = true;    
                 match = '(';
                 break;  
    case '}':    reverse = true;
                 match = '{';
                 break;
    case ']':    reverse = true;
                 match = '[';
                 break;
    case '>':    reverse = true;
                 match = '<';
                 break;
    default:     // Nothing to match
                 return -1;
  }

  if (reverse)
  {
    for (int pos = p_bracketPos - 1, nest = 1; pos >= 0; --pos)
    {
      TCHAR c = p_string[pos];
      if (c == bracket)
      {
        ++nest;
      }
      else if (c == match)
      {
        if (--nest == 0)
        {
          return pos;
        }
      }
    }
  }
  else
  {
    for (int pos = p_bracketPos + 1, nest = 1, len = p_string.GetLength(); pos < len; ++pos)
    {
      TCHAR c = p_string[pos];

      // skip finding matching bracket if encased in single quotes
      if(c == '\'')
      {
        pos = p_string.Find('\'',pos + 1);
        continue;
      }
      if (c == bracket)
      {
        ++nest;
      }
      else if (c == match)
      {
        if (--nest == 0)
        {
          return pos;
        }
      }
    }
  }
  return -1;
}

XString
JSONPath::DetermineRelationalOperator(XString p_token,int& p_pos)
{
  switch(GetCurrentCharacter(p_token,p_pos))
  {
    case '=': if(GetNextCharacter(p_token,p_pos) == '=')
              {
                p_pos++;
                return _T("==");
              }
              return _T("");
    case '!': if(GetNextCharacter(p_token,p_pos) == '=')
              {
                p_pos++;
                return _T("!=");
              }
              return _T("");
    case '<': if(GetNextCharacter(p_token,p_pos) == '=')
              {
                p_pos++;
                return _T("<=");
              }
              return _T("<");
    case '>': if(GetNextCharacter(p_token,p_pos) == '=')
              {
                p_pos++;
                return _T(">=");
              }
              return _T(">");
    default:  return _T("");
  }
}

bool 
JSONPath::EvaluateFilterClause(Relation p_filter,const JSONvalue& p_value)
{
  JsonType type = p_value.GetDataType();
  if(p_filter.clause.Compare(_T("==")) == 0)
  {
    switch(type)
    {
      case JsonType::JDT_string:     return p_value.GetString()    == p_filter.rightSide;
      case JsonType::JDT_number_int: return p_value.GetNumberInt() == _ttoi(p_filter.rightSide);
      case JsonType::JDT_number_bcd: return p_value.GetNumberBcd() == _ttof(p_filter.rightSide);
    }
  }
  else if(p_filter.clause.Compare(_T("!=")) == 0)
  {
    switch(type)
    {
      case JsonType::JDT_string:     return p_value.GetString()    != p_filter.rightSide;
      case JsonType::JDT_number_int: return p_value.GetNumberInt() != _ttoi(p_filter.rightSide);
      case JsonType::JDT_number_bcd: return p_value.GetNumberBcd() != _ttof(p_filter.rightSide);
    }
  }
  else if(p_filter.clause.Compare(_T("<")) == 0)
  {
    switch(type)
    {
      case JsonType::JDT_string:     return p_value.GetString()    < p_filter.rightSide;
      case JsonType::JDT_number_int: return p_value.GetNumberInt() < _ttoi(p_filter.rightSide);
      case JsonType::JDT_number_bcd: return p_value.GetNumberBcd() < _ttof(p_filter.rightSide);
    }
  }
  else if(p_filter.clause.Compare(_T("<=")) == 0)
  {
    switch(type)
    {
      case JsonType::JDT_string:     return p_value.GetString()    <= p_filter.rightSide;
      case JsonType::JDT_number_int: return p_value.GetNumberInt() <= _ttoi(p_filter.rightSide);
      case JsonType::JDT_number_bcd: return p_value.GetNumberBcd() <= _ttof(p_filter.rightSide);
    }
  }
  else if(p_filter.clause.Compare(_T(">")) == 0)
  {
    switch(type)
    {
      case JsonType::JDT_string:     return p_value.GetString()    > p_filter.rightSide;
      case JsonType::JDT_number_int: return p_value.GetNumberInt() > _ttoi(p_filter.rightSide);
      case JsonType::JDT_number_bcd: return p_value.GetNumberBcd() > _ttof(p_filter.rightSide);
    }
  }
  else if(p_filter.clause.Compare(_T(">=")) == 0)
  {
    switch(type)
    {
      case JsonType::JDT_string:     return p_value.GetString()    >= p_filter.rightSide;
      case JsonType::JDT_number_int: return p_value.GetNumberInt() >= _ttoi(p_filter.rightSide);
      case JsonType::JDT_number_bcd: return p_value.GetNumberBcd() >= _ttof(p_filter.rightSide);
    }
  }
  else if(p_filter.clause.IsEmpty() && p_filter.leftSide.IsEmpty() && !p_filter.rightSide.IsEmpty())
  {
    switch(type)
    {
      case JsonType::JDT_string:     return p_value.GetString()    >= p_filter.rightSide;
      case JsonType::JDT_number_int: return p_value.GetNumberInt() >= _ttoi(p_filter.rightSide);
      case JsonType::JDT_number_bcd: return p_value.GetNumberBcd() >= _ttof(p_filter.rightSide);
    }
  }
  return false;
}
