/////////////////////////////////////////////////////////////////////////////
//
// SQL ReWriter parser
// ir. W.E. Huisman
//
#pragma once

enum class Token
{
  TK_EOS    = 0     // END-OF-STRING
 ,TK_PLAIN          // Plain character string
 ,TK_SQUOTE         // Single quote string
 ,TK_DQUOTE         // Double quote string
 ,TK_POINT          // .
 ,TK_COMMA          // ,
 ,TK_COMM_SQL       // --
 ,TK_COMM_C         // /*
 ,TK_COMM_CPP       // //
 ,TK_PAR_OPEN       // (
 ,TK_PAR_CLOSE      // )
 ,TK_SPACE 
 ,TK_TAB
 ,TK_CR
 ,TK_NEWLINE
 ,TK_SELECT
 ,TK_INSERT
 ,TK_UPDATE
 ,TK_DELETE
 ,TK_FROM
 ,TK_JOIN
 ,TK_WHERE
 ,TK_GROUP
 ,TK_ORDER
 ,TK_HAVING 
 ,TK_INTO
 ,TK_UNION
};


class SchemaReWriter
{
public:
  SchemaReWriter(XString p_schema);

  XString Parse(XString p_input);
  int     GetReplaced() { return m_replaced; };
private:
  void    ParseStatement();

  // Token parsing
  Token   GetToken();
  void    PrintToken();
  void    PrintSpecials();
  Token   FindToken();
  void    AppendSchema();

  void    SkipSpaceAndComment();
  Token   CommentSQL();
  Token   CommentCPP();
  void    QuoteString(int p_ending);
  void    UnGetChar();
  int     GetChar();


  // Primary data
  XString m_schema;
  XString m_input;
  XString m_output;

  // Processing data
  int     m_position { 0 };
  Token   m_token;
  XString m_tokenString;
  int     m_level       { 0      };
  Token   m_inStatement { Token::TK_EOS };
  bool    m_inFrom      { false  };
  bool    m_nextTable   { false  };
  int     m_replaced    { 0      };
};
