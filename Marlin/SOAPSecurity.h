/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SOAPSecurity.h
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

//////////////////////////////////////////////////////////////////////////
//
// This is the implementation of the WS-Security standard known as:
// Web Services Security Username Token Profile Version 1.1.1 (2012)
// from the OASIS Consortium (IBM, Microsoft, Oracle, Verisign, Tibco)
//
#pragma once
#include "SOAPMessage.h"
#include "XMLTemporal.h"

// Default freshness of a provided header
#define SECURITY_FRESHNESS ( 5 * 60)    // On average 5 minutes
#define SECURITY_MINTIME   ( 1 * 60)    // As a minimum 1 minute
#define SECURITY_MAXTIME   (60 * 60)    // As a maximum 1 hour

typedef CString (__stdcall *UserPassword)(void* p_context,CString p_user);

class SOAPSecurity
{
public:
  SOAPSecurity();
 ~SOAPSecurity();

  // Setting primary info
  void    SetUser(CString p_user);
  void    SetPassword(CString p_password);
  void    SetDigesting(bool p_digest);
  void    SetFreshness(int p_seconds);
  void    SetUserPasswordFinder(void* p_context,UserPassword p_function);

  // Add security header to an outgoing message
  bool    SetSecurity(SOAPMessage* p_message);
  // Check security header of an incoming message, returning the user or nullptr
  bool    CheckSecurity(SOAPMessage* p_message);

private:
  // Make sure the message has the correct namespace for the header
  bool    AddSecurityNamespace(SOAPMessage* p_message);
  // Make sure the header has the <wsse::Security> parameter
  XMLElement* SetAddSecurityHeader(SOAPMessage* p_message);

  // Set the parts of the "Security" node
  bool    SetUsernameInMessage(SOAPMessage* p_message,XMLElement* p_secure);
  bool    SetPasswordInMessage(SOAPMessage* p_message,XMLElement* p_secure,CString p_password);
  bool    SetNonceInMessage   (SOAPMessage* p_message,XMLElement* p_secure);
  bool    SetCreatedInMessage (SOAPMessage* p_message,XMLElement* p_secure);
  // Create a new nonce and a digested password string
  CString DigestPassword();
  void    GenerateNonce(CString p_nonce);
  // Incoming control field
  CString FindHeaderField(SOAPMessage* p_message,XMLElement* p_secure,CString p_field);
  CString DeBase64(CString p_field);

  CString       m_username;                           // The username to use
  CString       m_password;                           // The user's password
  CString       m_nonce;                              // Nonce used in calculating the password digest
  bool          m_digest    { true };                 // Digest the security header
  int           m_freshness { SECURITY_FRESHNESS };   // Number of minutes before timestamps expire
  XMLTimestamp  m_timestamp;                          // Timestamp of the security header
  void*         m_context      { nullptr };           // Context of the user/password finder
  UserPassword  m_findPassword { nullptr };           // Function to find the user's password
};

inline void
SOAPSecurity::SetUser(CString p_user)
{
  m_username = p_user;
}

inline void
SOAPSecurity::SetPassword(CString p_password)
{
  m_password = p_password;
}

inline void
SOAPSecurity::SetDigesting(bool p_digest)
{
  m_digest = p_digest;
}

inline void
SOAPSecurity::SetUserPasswordFinder(void* p_context,UserPassword p_function)
{
  m_context      = p_context;
  m_findPassword = p_function;
}
