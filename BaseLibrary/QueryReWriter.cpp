/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: QueryReWriter.cpp
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
#include "pch.h"
#include "QueryReWriter.h"
#include "WiNFile.h"

// All tokens that must be recognized
// for the logic to work correctly
static const PTCHAR all_tokens[] =
{
   _T("")
  ,_T("")
  ,_T("")
  ,_T("'")
  ,_T("\"")
  ,_T(".")
  ,_T(",")
  ,_T("-")
  ,_T("/")
  ,_T("--")
  ,_T("/*")
  ,_T("//")
  ,_T("(")
  ,_T(")")
  ,_T("(+)")
  ,_T("+")
  ,_T("||")
  ,_T(" ")
  ,_T("\t")
  ,_T("\r")
  ,_T("\n")
  // Complete SQL words
  ,_T("SELECT")
  ,_T("INSERT")
  ,_T("UPDATE")
  ,_T("DELETE")
  ,_T("FROM")
  ,_T("JOIN")
  ,_T("WHERE")
  ,_T("GROUP")
  ,_T("ORDER")
  ,_T("HAVING")
  ,_T("INTO")
  ,_T("UNION")
  ,_T("STATISTICS")
  ,_T("FOR")
};

// All registered SQL words including tokens and special registrations
static SQLWords g_allWords;

// Remove all words at the closing of the process
void QueryRewriterRemoveAllWords()
{
  g_allWords.clear();
}

///////////////////////////////////////////////////////////////////////////

QueryReWriter::QueryReWriter(const XString& p_schema)
               :m_schema(p_schema)
{
}

XString 
QueryReWriter::Parse(const XString& p_input)
{
  Reset();
  m_input = p_input;

  ParseStatement();

  if(m_level != 0)
  {
    throw StdException(_T("Odd number of '(' and ')' tokens in the statement"));
  }
  return m_output;
}

bool
QueryReWriter::SetOption(SROption p_option)
{
  if(p_option > SROption::SRO_NO_OPTION &&
     p_option < SROption::SRO_LAST_OPTION)
  {
    m_options |= (int) p_option;
    return true;
  }
  return false;
}

bool
QueryReWriter::AddSQLWord(const XString& p_word
                         ,const XString& p_replacement
                         ,const XString  p_schema /*= ""*/
                         ,      Token    p_token  /*= Token::TK_EOS*/
                         ,      OdbcEsc  p_odbc   /*= OdbcEsc::ODBCESC_None*/)
{
  // Must be sure we have the tokens beforehand
  Initialization();

  SQLWord word;
  word.m_word        = p_word;
  word.m_replacement = p_replacement;
  word.m_schema      = p_schema;
  word.m_token       = p_token;
  word.m_odbcEscape  = p_odbc;

  if(g_allWords.find(p_word) != g_allWords.end())
  {
    return false;
  }
  g_allWords.insert(std::make_pair(p_word,word));
  return true;
}

bool
QueryReWriter::AddSQLWord(const SQLWord& p_word)
{
  // Must be sure we have the tokens beforehand
  Initialization();

  if(g_allWords.find(p_word.m_word) != g_allWords.end())
  {
    return false;
  }
  g_allWords.insert(std::make_pair(p_word.m_word,p_word));
  return true;
}

// Returns true if ALL words in parameter are added successfully
bool
QueryReWriter::AddSQLWords(const SQLWords& p_words)
{
  // Must be sure we have the tokens beforehand
  Initialization();

  bool status(true);
  for(auto& word : p_words)
  {
    if(g_allWords.find(word.first) != g_allWords.end())
    {
      status = false;
    }
    g_allWords.insert(std::make_pair(word.first,word.second));
  }
  return status;
}

bool
QueryReWriter::AddSQLWordsFromFile(const XString& p_filename)
{
  // Must be sure we have the tokens beforehand
  Initialization();

  bool result(true);
  WinFile file(p_filename);
  if(file.Open(winfile_read))
  {
    XString line;
    while(file.Read(line))
    {
      line = line.TrimRight(_T("\n"));
      if(!line.GetLength() || line.GetAt(0) == '#')
      {
        // Empty line or comment line
        continue;
      }
      // Syntax example
      // Word,[schema],replacement,[fn]
      int pos1 = line.Find(',');
      int pos2 = line.Find(',',pos1 + 1);
      int pos3 = line.Find(',',pos2 + 1);
      // All three delimiters must be present
      if(pos1 > 0 && pos2 > pos1 && pos3 > pos2)
      {
        SQLWord word;
        word.m_word        = line.Left(pos1).Trim();
        word.m_schema      = line.Mid(pos1 + 1,pos2 - pos1 - 1).Trim();
        word.m_replacement = line.Mid(pos2 + 1,pos3 - pos2 - 1).Trim();
        XString odbc       = line.Mid(pos3 + 1).Trim();
        if(!odbc.IsEmpty())
        {
          if(odbc.CompareNoCase(_T("function"))  == 0) word.m_odbcEscape = OdbcEsc::Function;
          if(odbc.CompareNoCase(_T("procedure")) == 0) word.m_odbcEscape = OdbcEsc::Procedure;
          if(odbc.CompareNoCase(_T("date"))      == 0) word.m_odbcEscape = OdbcEsc::Date;
          if(odbc.CompareNoCase(_T("time"))      == 0) word.m_odbcEscape = OdbcEsc::Time;
          if(odbc.CompareNoCase(_T("timestamp")) == 0) word.m_odbcEscape = OdbcEsc::Timestamp;
          if(odbc.CompareNoCase(_T("guid"))      == 0) word.m_odbcEscape = OdbcEsc::Guid;
          if(odbc.CompareNoCase(_T("like"))      == 0) word.m_odbcEscape = OdbcEsc::LikeEsc;
          if(odbc.CompareNoCase(_T("interval"))  == 0) word.m_odbcEscape = OdbcEsc::Interval;
          if(odbc.CompareNoCase(_T("outerjoin")) == 0) word.m_odbcEscape = OdbcEsc::OuterJoin;
        }
        // Remember this word
        if(!AddSQLWord(word))
        {
          result = false;
        }
      }
    }
    file.Close();
  }
  return result;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

void
QueryReWriter::Reset()
{
  // Primary in- and output
  m_input.Empty();
  m_output.Empty();
  // Processing data
  m_tokenString.Empty();
  m_token       = Token::TK_EOS;
  m_inStatement = Token::TK_EOS;
  m_inFrom      = false;
  m_nextTable   = false;
  m_position    = 0;
  m_level       = 0;
  m_ungetch     = 0;
  m_replaced    = 0;

  // Leave the following members untouched!
  // m_schema
  // m_options
  // m_codes / m_odbcfuncs / m_specials
  Initialization();
}

// Be carefully when called from outside that you call it 
// only once with the 'force' option
void
QueryReWriter::Initialization()
{
  // Only do the initialization once
  if(!g_allWords.empty())
  {
    return;
  }

  // At a minimum, we need all tokens
  for(int ind = 0; ind < sizeof(all_tokens) / sizeof(const TCHAR*); ++ind)
  {
    SQLWord word;
    word.m_word = all_tokens[ind];
    word.m_token = (Token) ind;
    word.m_odbcEscape = OdbcEsc::None;

    g_allWords.insert(std::make_pair(all_tokens[ind],word));
  }

  // Remove when process dies
  atexit(QueryRewriterRemoveAllWords);
}

// Completely parsing the statement
// possibly recurse for parenthesis
void
QueryReWriter::ParseStatement(bool p_closingEscape /*= false*/)
{
  bool begin = true;
  bool odbc  = false;

  ++m_level;
  while(true)
  {
    m_token = GetToken();
    if(m_token == Token::TK_EOS)
    {
      break;
    }

    SkipSpaceAndComment();

    // First token in the statement
    if(begin && (m_token == Token::TK_SELECT || 
                 m_token == Token::TK_INSERT || 
                 m_token == Token::TK_UPDATE || 
                 m_token == Token::TK_DELETE))
    {
      m_inStatement = m_token;
      begin = false;
    }

    // Next level statement
    if(m_token == Token::TK_PAR_OPEN)
    {
      PrintToken();
      Token oldStatement = m_inStatement;
      bool  oldFrom      = m_inFrom;
      m_inStatement      = Token::TK_EOS;
      ParseStatement(odbc);
      m_inStatement      = oldStatement;
      m_inFrom           = oldFrom;
      continue;
    }
    else if(m_token == Token::TK_PLAIN_ODBC)
    {
      odbc = true;
    }

    if(m_token == Token::TK_STATISTICS)
    {
      m_inStatement = m_token;
      PrintToken();
      continue;
    }
    if(m_token == Token::TK_FOR)
    {
      m_nextTable = false;
    }

    // Append schema
    if(m_nextTable)
    {
      AppendSchema();
    }

    // Find next table for appending a schema
    if(m_inStatement == Token::TK_SELECT && (m_token == Token::TK_FROM || m_token == Token::TK_JOIN))
    {
      m_nextTable = true;
      if(m_token == Token::TK_FROM)
      {
        m_inFrom = true;
      }
    }
    if(m_inStatement == Token::TK_STATISTICS && m_token == Token::TK_FOR)
    {
      m_nextTable = true;
    }
    if(m_inStatement == Token::TK_SELECT && m_inFrom && m_token == Token::TK_COMMA)
    {
      m_nextTable = true;
    }
    if(m_inStatement == Token::TK_INSERT && m_token == Token::TK_INTO)
    {
      m_nextTable = true;
    }
    if(m_inStatement == Token::TK_UPDATE)
    {
      m_nextTable = true;
    }
    if(m_inStatement == Token::TK_DELETE && m_token == Token::TK_FROM)
    {
      m_nextTable = true;
    }

    // End of next table
    if(m_token == Token::TK_WHERE || m_token == Token::TK_GROUP || 
       m_token == Token::TK_ORDER || m_token == Token::TK_HAVING )
    {
      m_inFrom    = false;
      m_nextTable = false;
    }

    PrintToken();

    // Check reset on next UNION
    if(m_token == Token::TK_UNION)
    {
      begin         = true;
      m_inFrom      = false;
      m_nextTable   = false;
      m_inStatement = Token::TK_EOS;
    }

    // End of level
    if(m_token == Token::TK_PAR_CLOSE)
    {
      if(p_closingEscape)
      {
        m_output += _T("}");
      }
      break;
    }
  }
  --m_level;
}

// Printing a token to the output buffer
void
QueryReWriter::PrintToken()
{
  switch(m_token)
  {
    case Token::TK_PLAIN:     [[fallthrough]];
    case Token::TK_PLAIN_ODBC:m_output += m_tokenString;
                              break;
    case Token::TK_SQUOTE:    m_output += '\'';
                              m_output += m_tokenString;
                              m_output += '\'';
                              break;
    case Token::TK_DQUOTE:    m_output += '\"';
                              m_output += m_tokenString;
                              m_output += '\"';
                              break;
    case Token::TK_COMM_SQL:  m_output += _T("--");
                              m_output += m_tokenString;
                              m_output += '\n';
                              break;
    case Token::TK_COMM_C:    m_output += _T("/*");
                              m_output += m_tokenString;
                              m_output += _T("/");
                              break;
    case Token::TK_COMM_CPP:  m_output += _T("//");
                              m_output += m_tokenString;
                              m_output += '\n';
                              break;
    case Token::TK_PAR_ADD:   m_output += (m_options & (int)SROption::SRO_ADD_TO_CONCAT) ? _T("||") : _T("+");
                              break;
    case Token::TK_PAR_CONCAT:m_output += (m_options & (int)SROption::SRO_CONCAT_TO_ADD) ? _T("+") : _T("||");
                              break;
    case Token::TK_PAR_OUTER: PrintOuterJoin();
                              break;
    case Token::TK_POINT:     [[fallthrough]];
    case Token::TK_COMMA:     [[fallthrough]];
    case Token::TK_MINUS:     [[fallthrough]];
    case Token::TK_DIVIDE:    [[fallthrough]];
    case Token::TK_PAR_OPEN:  [[fallthrough]];
    case Token::TK_PAR_CLOSE: [[fallthrough]];
    case Token::TK_SPACE:     [[fallthrough]];
    case Token::TK_TAB:       [[fallthrough]];
    case Token::TK_CR:        [[fallthrough]];
    case Token::TK_NEWLINE:   [[fallthrough]];
    case Token::TK_SELECT:    [[fallthrough]];
    case Token::TK_INSERT:    [[fallthrough]];
    case Token::TK_UPDATE:    [[fallthrough]];
    case Token::TK_DELETE:    [[fallthrough]];
    case Token::TK_FROM:      [[fallthrough]];
    case Token::TK_JOIN:      [[fallthrough]];
    case Token::TK_WHERE:     [[fallthrough]];
    case Token::TK_GROUP:     [[fallthrough]];
    case Token::TK_ORDER:     [[fallthrough]];
    case Token::TK_HAVING:    [[fallthrough]];
    case Token::TK_INTO:      [[fallthrough]];
    case Token::TK_STATISTICS:[[fallthrough]];
    case Token::TK_FOR:       [[fallthrough]];
    case Token::TK_UNION:     m_output += all_tokens[(int)m_token];
                              break;
    case Token::TK_EOS:       break;
    default:                  m_output += _T("\nINTERNAL ERROR: Unknown SQL token!\n");
                              break;
  }
}

void
QueryReWriter::PrintOuterJoin()
{
  m_output += all_tokens[(int) Token::TK_PAR_OUTER];
  if(m_options & (int) SROption::SRO_WARN_OUTER)
  {
    m_output += _T("\n");
    m_output += _T("-- BEWARE: Oracle old style (+). Rewrite the SQL query with LEFT OUTER JOIN syntaxis!");
    m_output += _T("\n");
  }
}

// THIS IS WHY WE ARE HERE IN THIS CLASS!
void
QueryReWriter::AppendSchema()
{
  SkipSpaceAndComment();

  int ch = GetChar();

  if(ch == '.')
  {
    // There already was a schema name
    m_output += m_tokenString;
    m_output += '.';
    m_token   = GetToken();
  }
  else
  {
    // Reset token
    UnGetChar(ch);

    if(!m_schema.IsEmpty())
    {
      ++m_replaced;
      m_output += m_schema;
      m_output += '.';
    }
  }

  m_nextTable = false;

  // Reset for update
  if(m_inStatement == Token::TK_UPDATE)
  {
    m_inStatement = Token::TK_EOS;
  }
}

// Skipping spaces and comments
void
QueryReWriter::SkipSpaceAndComment()
{
  while(true)
  {
    if(m_token == Token::TK_SPACE || m_token == Token::TK_TAB || m_token == Token::TK_CR || m_token == Token::TK_NEWLINE)
    {
      PrintToken();
      m_token = GetToken();
      continue;
    }
    if(m_token == Token::TK_COMM_SQL || m_token == Token::TK_COMM_C || m_token == Token::TK_COMM_CPP)
    {
      PrintToken();
      m_token = GetToken();
      continue;
    }
    return;
  }
}

// Getting the next token/tokenstring
Token
QueryReWriter::GetToken()
{
  m_tokenString.Empty();
  int ch = 0;

  if((ch = GetChar()) != (int)Token::TK_EOS)
  {
    switch(ch)
    {
      case '\'':  QuoteString(ch);
                  return Token::TK_SQUOTE;
      case '\"':  QuoteString(ch);
                  return Token::TK_DQUOTE;
      case '-':   return CommentSQL();
      case '/':   return CommentCPP();
      case '|':   return StringConcatenate();
      case '.':   return Token::TK_POINT;
      case ',':   return Token::TK_COMMA;
      case '(':   return Parenthesis();
      case ')':   return Token::TK_PAR_CLOSE;
      case '+':   return Token::TK_PAR_ADD;
      case ' ':   return Token::TK_SPACE;
      case '\t':  return Token::TK_TAB;
      case '\r':  return Token::TK_CR;
      case '\n':  return Token::TK_NEWLINE;
    }
    if(isdigit(ch))
    {
      m_tokenString = XString((TCHAR)ch,1);
      return Token::TK_PLAIN;
    }
    if(isalpha(ch))
    {
      m_tokenString = XString((TCHAR) ch,1);
      ch = GetChar();
      while(isalnum(ch) || ch == '_')
      {
        m_tokenString += (TCHAR) ch;
        ch = GetChar();
      }
      if(ch)
      {
        UnGetChar(ch);
      }
      return FindToken();
    }
    m_tokenString = XString((TCHAR) ch,1);
    return Token::TK_PLAIN;
  }
  return Token::TK_EOS;
}

// Discover the next tokenstring to be a 'real'token
Token
QueryReWriter::FindToken()
{
  // The one-and-only lookup for a token
  SQLWords::iterator tok = g_allWords.find(m_tokenString);
  if(tok == g_allWords.end())
  {
    return Token::TK_PLAIN;
  }

  if(tok->second.m_odbcEscape != OdbcEsc::None)
  {
    switch(tok->second.m_odbcEscape)
    {
      case OdbcEsc::Function:   m_tokenString = XString(_T("{fn ")) + tok->second.m_replacement;
                                break;
      case OdbcEsc::Procedure:  m_tokenString = XString(_T("{[?=]call ")) + tok->second.m_replacement;
                                break;
      case OdbcEsc::Date:       m_tokenString = _T("{d ");
                                break;
      case OdbcEsc::Time:       m_tokenString = _T("{t "); 
                                break;
      case OdbcEsc::Timestamp:  m_tokenString = _T("{ts ");
                                break;
      case OdbcEsc::Guid:       m_tokenString = _T("{guid ");
                                break;
      case OdbcEsc::LikeEsc:    m_tokenString = _T("{");
                                break;
      case OdbcEsc::Interval:   m_tokenString = _T("{INTERVAL ");
                                break;
      case OdbcEsc::OuterJoin:  m_tokenString = _T("{oj ");
                                break;
    }
    // TK_PLAIN_ODBC will provide for the closing '}' escape sequence!
    return Token::TK_PLAIN_ODBC;
  }
  else
  {
    // Possibly a replacement token and schema
    if(!tok->second.m_replacement.IsEmpty())
    {
      m_tokenString = tok->second.m_replacement;
    }
    if(!tok->second.m_schema.IsEmpty())
    {
      m_tokenString = tok->second.m_schema + _T(".") + m_tokenString;
    }
  }

  // Return as a specific token or just a plain word
  if(tok->second.m_token != Token::TK_EOS)
  {
    return tok->second.m_token;
  }
  return Token::TK_PLAIN;
}

// Parse standard SQL comment in the -- like way
Token
QueryReWriter::CommentSQL()
{
  int ch = GetChar();
  if(ch == '-')
  {
    while(true)
    {
      ch = GetChar();
      if(ch == 0 || ch == '\n')
      {
        break;
      }
      m_tokenString += (TCHAR) ch;
    }
    return Token::TK_COMM_SQL;
  }
  UnGetChar(ch);
  return Token::TK_MINUS;
}

// Found a forward slash. Is it any form of comment?
Token
QueryReWriter::CommentCPP()
{
  int ch = GetChar();
  if(ch == '/')
  {
    while(true)
    {
      ch = GetChar();
      if(ch == 0 || ch == '\n')
      {
        break;
      }
      m_tokenString += (TCHAR) ch;
    }
    return Token::TK_COMM_CPP;
  }
  else if(ch == '*')
  {
    int lastchar = 0;
    while(true)
    {
      ch = GetChar();
      if(ch == 0 || (ch == '/' && lastchar == '*'))
      {
        break;
      }
      m_tokenString += (TCHAR) ch;
      lastchar = ch;
    }
    return Token::TK_COMM_C;
  }
  else
  {
    UnGetChar(ch);
    return Token::TK_DIVIDE;
  }
}

// Found a '|'. Is it a SQL string concatenation?
Token
QueryReWriter::StringConcatenate()
{
  int ch = GetChar();
  if(ch == '|')
  {
    return Token::TK_PAR_CONCAT;
  }
  UnGetChar(ch);
  return Token::TK_PLAIN;
}

// Found a parenthesis. Is it a Oracle left-outer-join?
Token 
QueryReWriter::Parenthesis()
{
  int ch = GetChar();
  if(ch == '+')
  {
    // One extra look-ahead
    if(m_input.GetAt(m_position) == ')')
    {
      GetChar();
      return Token::TK_PAR_OUTER;
    }
  }
  UnGetChar(ch);
  return Token::TK_PAR_OPEN;
}

// Parse single or double quoted string
void
QueryReWriter::QuoteString(int p_ending)
{
  while(true)
  {
    int ch = GetChar();
    if(ch == p_ending || ch == 0)
    {
      return;
    }
    m_tokenString += (TCHAR) ch;
  } 
}

// Restore last character
void
QueryReWriter::UnGetChar(int p_char)
{
  if(m_ungetch == 0)
  {
    m_ungetch = p_char;
  }
}

// Getting next character and possible the restored unget
int
QueryReWriter::GetChar()
{
  if(m_ungetch)
  {
    int ch = m_ungetch;
    m_ungetch = 0;
    return ch;
  }
  if(m_position < m_input.GetLength())
  {
    return m_input.GetAt(m_position++);
  }
  return 0;
}
