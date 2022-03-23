/////////////////////////////////////////////////////////////////////////////
//
// SQL SchemaReWriter parser
// ir. W.E. Huisman
//
#include "pch.h"
#include "SchemaReWriter.h"

const char* tokens[] = 
{
   ""
  ,""
  ,"'"
  ,"\""
  ,"."
  ,","
  ,"--"
  ,"/*"
  ,"//"
  ,"("
  ,")"
  ," "
  ,"\t"
  ,"\r"
  ,"\n"
  ,"SELECT"
  ,"INSERT"
  ,"UPDATE"
  ,"DELETE"
  ,"FROM"
  ,"JOIN"
  ,"WHERE"
  ,"GROUP"
  ,"ORDER"
  ,"HAVING"
  ,"INTO"
  ,"UNION"
};

///////////////////////////////////////////////////////////////////////////

SchemaReWriter::SchemaReWriter(XString p_schema)
               :m_schema(p_schema)
{

}

XString 
SchemaReWriter::Parse(XString p_input)
{
  m_input = p_input;
  ParseStatement();

  if(m_level != 0)
  {
    MessageBox(NULL,"Oneven aantal '(' en ')' tekens in het statement","Waarschuwing",MB_OK|MB_ICONERROR);
  }
  return m_output;
}

void
SchemaReWriter::ParseStatement()
{
  bool begin = true;

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
      m_inStatement = Token::TK_EOS;
      ParseStatement();
      m_inStatement = oldStatement;
      m_inFrom      = oldFrom;
      continue;
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

    // Einde van next table
    if(m_token == Token::TK_WHERE || m_token == Token::TK_GROUP || m_token == Token::TK_ORDER || m_token == Token::TK_HAVING)
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
      break;
    }
  }
  --m_level;
}

void
SchemaReWriter::PrintToken()
{
  switch(m_token)
  {
    case Token::TK_PLAIN:     PrintSpecials();
                              break;
    case Token::TK_SQUOTE:    m_output += '\'';
                              m_output += m_tokenString;
                              m_output += '\'';
                              break;
    case Token::TK_DQUOTE:    m_output += '\"';
                              m_output += m_tokenString;
                              m_output += '\"';
                              break;
    case Token::TK_POINT:     m_output += '.';
                              break;
    case Token::TK_COMMA:     m_output += ',';
                              break;
    case Token::TK_COMM_SQL:  m_output += "--";
                              m_output += m_tokenString;
                              m_output += '\n';
                              break;
    case Token::TK_COMM_C:    m_output += "/*";
                              m_output += m_tokenString;
                              m_output += "/";
                              break;
    case Token::TK_COMM_CPP:  m_output += "//";
                              m_output += m_tokenString;
                              m_output += '\n';
                              break;
    case Token::TK_PAR_OPEN:  m_output += '(';
                              break;
    case Token::TK_PAR_CLOSE: m_output += ')';
                              break;
    case Token::TK_SPACE:     m_output += ' ';
                              break;
    case Token::TK_TAB:       m_output += '\t';
                              break;
    case Token::TK_CR:        m_output += '\r';
                              break;
    case Token::TK_NEWLINE:   m_output += '\n';
                              break;
    case Token::TK_SELECT:    m_output += "SELECT";
                              break;
    case Token::TK_INSERT:    m_output += "INSERT";
                              break;
    case Token::TK_UPDATE:    m_output += "UPDATE";
                              break;
    case Token::TK_DELETE:    m_output += "DELETE";
                              break;
    case Token::TK_FROM:      m_output += "FROM";
                              break;
    case Token::TK_JOIN:      m_output += "JOIN";
                              break;
    case Token::TK_WHERE:     m_output += "WHERE";
                              break;
    case Token::TK_GROUP:     m_output += "GROUP";
                              break;
    case Token::TK_ORDER:     m_output += "ORDER";
                              break;
    case Token::TK_HAVING:    m_output += "HAVING";
                              break;
    case Token::TK_INTO:      m_output += "INTO";
                              break;
    case Token::TK_UNION:     m_output += "UNION";
                              break;
  }
}

void
SchemaReWriter::PrintSpecials()
{
  if(m_tokenString.CompareNoCase("brief_ew") == 0)
  {
    m_output += m_schema;
    m_output += '.';
  }
  m_output += m_tokenString; 
}

// THIS IS WHY WE ARE HERE IN THIS CLASS!
void
SchemaReWriter::AppendSchema()
{
  SkipSpaceAndComment();

  Token   temp1 = m_token;
  XString temp2 = m_tokenString;
  m_token = GetToken();

  if(m_token == Token::TK_POINT)
  {
    // Er was al een schema naam
    m_output += temp2;
  }
  else
  {
    // Reset token
    UnGetChar();
    m_token = temp1;
    m_tokenString = temp2;

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

void
SchemaReWriter::SkipSpaceAndComment()
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


Token
SchemaReWriter::GetToken()
{
  m_tokenString.Empty();
  if(m_input.GetLength() < m_position)
  {
    return Token::TK_EOS;
  }

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
      case '.':   return Token::TK_POINT;
      case ',':   return Token::TK_COMMA;
      case '(':   return Token::TK_PAR_OPEN;
      case ')':   return Token::TK_PAR_CLOSE;
      case ' ':   return Token::TK_SPACE;
      case '\t':  return Token::TK_TAB;
      case '\r':  return Token::TK_CR;
      case '\n':  return Token::TK_NEWLINE;
    }
    if(isdigit(ch))
    {
      m_tokenString = XString((char)ch,1);
      return Token::TK_PLAIN;
    }
    if(isalpha(ch))
    {
      m_tokenString = XString((char) ch,1);
      ch = GetChar();
      while(isalnum(ch) || ch == '_')
      {
        m_tokenString += (char) ch;
        ch = GetChar();
      }
      if(ch)
      {
        UnGetChar();
      }
      return FindToken();
    }
    m_tokenString = XString((char) ch,1);
    return Token::TK_PLAIN;
  }
  return Token::TK_EOS;
}

Token
SchemaReWriter::FindToken()
{
  for(int ind = 0;ind < sizeof(tokens) / sizeof(const char*); ++ind)
  {
    if(m_tokenString.CompareNoCase(tokens[ind]) == 0)
    {
      return (Token) ind;
    }
  }
  return Token::TK_PLAIN;
}

Token
SchemaReWriter::CommentSQL()
{
  if(GetChar() == '-')
  {
    while(true)
    {
      int ch = GetChar();
      if(ch == 0 || ch == '\n')
      {
        break;
      }
      m_tokenString += (char) ch;
    }
    return Token::TK_COMM_SQL;
  }
  UnGetChar();
  return Token::TK_PLAIN;
}

Token
SchemaReWriter::CommentCPP()
{
  int ch = GetChar();

  if(ch == '/')
  {
    while(true)
    {
      int ch = GetChar();
      if(ch == 0 || ch == '\n')
      {
        break;
      }
      m_tokenString += (char) ch;
    }
    return Token::TK_COMM_CPP;
  }
  else if(ch == '*')
  {
    int lastchar = 0;
    while(true)
    {
      int ch = GetChar();
      if(ch == 0 || (ch == '/' && lastchar == '*'))
      {
        break;
      }
      m_tokenString += (char) ch;
      lastchar = ch;
    }
    return Token::TK_COMM_C;
  }
  else
  {
    UnGetChar();
    return Token::TK_PLAIN;
  }
}

void
SchemaReWriter::QuoteString(int p_ending)
{
  int ch = 0;

  while(true)
  {
    ch = GetChar();
    if(ch == p_ending || ch == 0)
    {
      return;
    }
    m_tokenString += (char) ch;
  } 
}

void
SchemaReWriter::UnGetChar()
{
  if(m_position > 0)
  {
    --m_position;
  }
}

int
SchemaReWriter::GetChar()
{
  if(m_position < m_input.GetLength())
  {
    return m_input.GetAt(m_position++);
  }
  return 0;
}
