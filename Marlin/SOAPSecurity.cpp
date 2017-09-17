/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SOAPSecurity.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2017 ir. W.E. Huisman
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
// from the OASIS Consortium (IBM, Microsoft, Oracle, VERISIGN, TIBCO)
//
#include "stdafx.h"
#include "SOAPSecurity.h"
#include "GenerateGUID.h"
#include "Base64.h"
#include "Crypto.h"

SOAPSecurity::SOAPSecurity()
{
}

SOAPSecurity::~SOAPSecurity()
{
}

void
SOAPSecurity::SetFreshness(int p_seconds)
{
  if(p_seconds < SECURITY_MINTIME)
  {
    p_seconds = SECURITY_MINTIME;
  }
  if(p_seconds > SECURITY_MAXTIME)
  {
    p_seconds = SECURITY_MAXTIME;
  }
  m_freshness = p_seconds;
}

// Add security header to an outgoing message
// Uses the preset properties:
// m_username -> Username to be authenticated
// m_password -> Password for the authentication
// m_digest   -> true=encode password with SHA1(nonce + timestamp + password)
//
bool
SOAPSecurity::SetSecurity(SOAPMessage* p_message)
{
  bool        namesp = AddSecurityNamespace(p_message);
  XMLElement* secure = SetAddSecurityHeader(p_message);

  if(namesp && secure)
  {
    // Prepare booleans
    bool OKUser  = false;
    bool OKPaswd = false;
    bool OKNonce = false;
    bool OKCreat = false;

    // Set a new timestamp for the message
    m_timestamp.SetCurrentTimestamp();

    // Set user in the message
    OKUser = SetUsernameInMessage(p_message,secure);

    // Set coded or unencoded password in the message
    CString password(m_password);

    // Maybe the password comes from elsewhere
    if(password.IsEmpty() && m_findPassword)
    {
      password = (*m_findPassword)(m_context,m_username);
    }

    if(m_digest)
    {
      password = DigestPassword();
    }
    OKPaswd = SetPasswordInMessage(p_message,secure,password);

    // For encoding, provide Nonce and Created time
    if(m_digest)
    {
      OKNonce = SetNonceInMessage  (p_message,secure);
      OKCreat = SetCreatedInMessage(p_message,secure);

      return (OKUser && OKPaswd && OKNonce && OKCreat);
    }
    return (OKUser && OKPaswd);
  }
  return false;
}

// Check security header of an incoming message, returning the user or nullptr
// No WS-Security has virtually no performance hit, as the header field is not found
//
// Uses the preset properties:
// m_username            -> If only authenticating this user
// m_password            -> If only authenticating above user
// OR
// SetUserPasswordFinder -> Find password of incoming user
// AND
// m_digest              -> if true,  do **NOT** accept uncoded passwords
//                       -> if false, also allow coded passwords
//
CString 
SOAPSecurity::CheckSecurity(SOAPMessage* p_message)
{
  XMLElement* security = p_message->GetHeaderParameterNode("Security");

  if(security)
  {
    // Set a new timestamp for the message
    m_timestamp.SetCurrentTimestamp();

    // Get relevant security header fields, if any at all
    CString username = FindHeaderField(p_message,security,"Username");
    CString password = FindHeaderField(p_message,security,"Password");
    CString nonce    = FindHeaderField(p_message,security,"Nonce");
    CString created  = FindHeaderField(p_message,security,"Created");

    // Find what the password should be
    CString shouldbePassword;
    if((username.Compare(m_username) == 0) && !m_password.IsEmpty())
    {
      // We have given a username/password to match
      shouldbePassword = m_password;
    }
    else if(m_password.IsEmpty() && m_findPassword)
    {
      // Let this function find a password for us
      shouldbePassword = (*m_findPassword)(m_context,username);
    }

    // If nonce and timestamp are given, re-calculate the password digest
    // If digest was set, we **MUST** try to use the encrypted digest
    if(!nonce.IsEmpty() && !created.IsEmpty() || m_digest)
    {
      // Find out if timestamp is not fresh enough
      // Otherwise a hacker might be at large!
      XMLTimestamp stamp(created);
      if((m_timestamp.GetValue() - stamp.GetValue()) > m_freshness)
      {
        return "";
      }

      Crypto  crypt;
      CString encrypt  = DeBase64(nonce) + created + shouldbePassword;
      shouldbePassword = crypt.Digest(encrypt,encrypt.GetLength(),CALG_SHA1);
    }

    // Try if passwords do match
    if(password.Compare(shouldbePassword) == 0)
    {
      return username;
    }
  }
  return "";
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

// Make sure the message has the correct namespace for the header
bool    
SOAPSecurity::AddSecurityNamespace(SOAPMessage* p_message)
{
  // Check that we have a message
  if(!p_message)
  {
    return false;
  }
  // Check that we are at least dealing with SOAP 1.2
  if(p_message->GetSoapVersion() != SoapVersion::SOAP_12)
  {
    return false;
  }

  // Find the envelope and check the namespaces
  XMLElement* envelope = p_message->FindElement("Envelope");
  if(envelope)
  {
    XmlAttribMap& map = envelope->GetAttributes();
    for(auto& attribute : map)
    {
      if(attribute.m_value.CompareNoCase(NAMESPACE_SECEXT) == 0)
      {
        // Found our namespace. Already there
        return true;
      }
    }
    p_message->SetAttribute(envelope,"xmlns:wsse",NAMESPACE_SECEXT);
    return true;
  }
  return false;
}

// Make sure the header has the <wsse::Security> parameter
XMLElement* 
SOAPSecurity::SetAddSecurityHeader(SOAPMessage* p_message)
{
  // Check that we have a message
  if(!p_message)
  {
    return nullptr;
  }

  // Find the SOAP Header
  XMLElement* header = p_message->FindElement("Header");
  if(header)
  {
    // <wsse:Security> **MUST** be a first child of the header node
    XMLElement* security = p_message->FindElement(header,"Security",false);
    if(security)
    {
      return security;
    }
    // Add the security node to the SOAP header
    return p_message->AddElement(header,"wsse:Security",XDT_String,"");
  }
  return nullptr;
}

bool
SOAPSecurity::SetUsernameInMessage(SOAPMessage* p_message,XMLElement* p_secure)
{
  if(p_message && p_secure)
  {
    if(p_message->AddElement(p_secure,"wsse:Username",XDT_String,m_username))
    {
      return true;
    }
  }
  return false;
}

bool
SOAPSecurity::SetPasswordInMessage(SOAPMessage* p_message,XMLElement* p_secure,CString p_password)
{
  if(p_message && p_secure)
  {
    XMLElement* pwd = p_message->AddElement(p_secure,"wsse:Password",XDT_String,p_password);
    if(pwd)
    {
      if(m_digest)
      {
        p_message->SetAttribute(pwd,"Type","#PasswordDigest");
      }
      return true;
    }
  }
  return false;
}


bool
SOAPSecurity::SetNonceInMessage(SOAPMessage* p_message,XMLElement* p_secure)
{
  if(p_message && p_secure)
  {
    if(p_message->AddElement(p_secure,"wsse:Nonce",XDT_String,m_nonce))
    {
      return true;
    }
  }
  return false;
}

bool
SOAPSecurity::SetCreatedInMessage(SOAPMessage* p_message,XMLElement* p_secure)
{
  if(p_message && p_secure)
  {
    CString timestamp = m_timestamp.AsString();
    if(p_message->AddElement(p_secure,"wsse:Created",XDT_String,timestamp))
    {
      return true;
    }
  }
  return false;
}

// Create a new nonce and a digested password string
CString 
SOAPSecurity::DigestPassword()
{
  CString nonce = GenerateGUID();
  nonce.Replace("-","");
  GenerateNonce(nonce);

  // According to the standard. This is how we scramble the password
  CString encrypted = nonce + m_timestamp.AsString() + m_password;

  Crypto crypt;
  return crypt.Digest(encrypted,encrypted.GetLength(),CALG_SHA1);
}

void
SOAPSecurity::GenerateNonce(CString p_nonce)
{
  Base64 base;
  int    len = (int) base.B64_length(p_nonce.GetLength());
  char* dest = m_nonce.GetBufferSetLength(len + 1);

  base.Encrypt((const unsigned char*)p_nonce.GetString(),len,(unsigned char*)dest);
  m_nonce.ReleaseBuffer();
}

// Incoming control field
CString 
SOAPSecurity::FindHeaderField(SOAPMessage* p_message,XMLElement* p_secure,CString p_field)
{
  CString value;

  if(p_message && p_secure)
  {
    XMLElement* elem = p_message->FindElement(p_secure,p_field,false);
    if(elem)
    {
      value = elem->GetValue();
    }
  }
  return value;
}

CString 
SOAPSecurity::DeBase64(CString p_field)
{
  Base64  base;
  CString string;
  int len = (int)base.Ascii_length(p_field.GetLength());
  char* dest = string.GetBufferSetLength(len + 1);
  base.Decrypt((const unsigned char*) p_field.GetString(),len,(unsigned char*)dest);
  string.ReleaseBuffer();

  return string;
}
