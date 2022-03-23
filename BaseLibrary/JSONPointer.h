/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: JSONPointer.h
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

//////////////////////////////////////////////////////////////////////////////
//
// JSONPointer
//
// Implementation of IETF RFC 6901: JavaScript Object Notation (JSON) Pointer
// See: https://datatracker.ietf.org/doc/html/rfc6901
//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "JSONMessage.h"

enum class JPStatus
{
  JP_None               // Nothing matched (yet)
 ,JP_Match_string       // JPPointer points to a string
 ,JP_Match_number_int   // JPPointer points to an integer
 ,JP_Match_number_bcd   // JPPointer points to a binary-coded-decimal
 ,JP_Match_constant     // JPPointer points to a JSON constant (true,false,null,none)
 ,JP_Match_object       // JPPointer points to an object
 ,JP_Match_array        // JPPointer points to an array
 ,JP_Match_wholedoc     // The complete JSON document is a valid return value
 ,JP_INVALID            // NO MATCH: Incorrect pointer in this JSON!
};

class JSONPointer
{
public:
  JSONPointer(bool p_originOne = false);
  JSONPointer(JSONMessage* p_message,XString p_pointer,bool p_originOne = false);
 ~JSONPointer();

  // Our main purpose: evaluate the pointer in the message
  bool Evaluate();

  // SETTERS + Re-Evaluate
  bool SetPointer(XString      p_pointer);
  bool SetMessage(JSONMessage* p_message);

  // GETTERS
  XString       GetPointer();
  JSONMessage*  GetJSONMessage();
  JPStatus      GetStatus();
  JsonType      GetType();
  bool          GetCanAppend();
  XString       GetLastToken();
  // Result independent of the type!
  JSONvalue*    GetResultJSONValue();
  XString       GetResultForceToString(bool p_whitespace = false);
  // Depending on type, return use of these!
  XString       GetResultString();
  int           GetResultInteger();
  bcd           GetResultBCD();
  JsonConst     GetResultConstant();
  JSONobject*   GetResultObject();
  JSONarray*    GetResultArray();

private:
  // Internal procedures
  void Reset();
  void PresetFromJsonValue();
  bool ParseLevel(XString& p_parsing);
  void UnescapeType(XString& p_pointer);
  void UnescapeJSON(XString& p_token);

  // DATA
  XString       m_pointer;
  JSONMessage*  m_message     { nullptr };
  JPStatus      m_status      { JPStatus::JP_None };
  char          m_delimiter   { '/'   };
  bool          m_canAppend   { false };
  int           m_origin      { 0     };
  XString       m_lastToken;

  // RESULT pointers
  JsonType      m_type        { JsonType::JDT_const  };
  JSONvalue*    m_value       { nullptr };
  JsonConst*    m_constant    { nullptr };
  XString*      m_string      { nullptr };
  int*          m_number_int  { nullptr };
  bcd*          m_number_bcd  { nullptr };
  JSONobject*   m_object      { nullptr };
  JSONarray*    m_array       { nullptr };
};

