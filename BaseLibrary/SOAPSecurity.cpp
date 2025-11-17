/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SOAPSecurity.cpp
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

//////////////////////////////////////////////////////////////////////////
//
// This is the implementation of the WS-Security standard known as:
// Web Services Security Username Token Profile Version 1.1.1 (2012)
// from the OASIS Consortium (IBM, Microsoft, Oracle, VERISIGN, TIBCO)
//
#include "pch.h"
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
    // Set a new timestamp for the message
    // Timestamp must be in UTC, so we can worldwide synchronize our webservices
    m_timestamp.SetSystemTimestamp();

    // Set user in the message
    bool OKUser = SetUsernameInMessage(p_message,secure);

    // Set coded or unencoded password in the message
    XString password(m_password);

    // Maybe the password comes from elsewhere
    if(password.IsEmpty() && m_findPassword)
    {
      password = (*m_findPassword)(m_context,m_username);
    }

    if(m_digest)
    {
      password = DigestPassword();
    }
    bool OKPaswd = SetPasswordInMessage(p_message,secure,password);

    // For encoding, provide Nonce and Created time
    if(m_digest)
    {
      bool OKNonce = SetNonceInMessage  (p_message,secure);
      bool OKCreat = SetCreatedInMessage(p_message,secure);

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
bool 
SOAPSecurity::CheckSecurity(SOAPMessage* p_message)
{
  XMLElement* security = p_message->GetHeaderParameterNode(_T("Security"));

  if(security)
  {
    // Set a new timestamp for checking the message
    // Be aware that we set it in UTC format !!
    m_timestamp.SetSystemTimestamp();

    // Get relevant security header fields, if any at all
    XString username = FindHeaderField(p_message,security,_T("Username"));
    XString password = FindHeaderField(p_message,security,_T("Password"));
    XString nonce    = FindHeaderField(p_message,security,_T("Nonce"));
    XString created  = FindHeaderField(p_message,security,_T("Created"));

    // Find what the password should be
    XString shouldbePassword;
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
        // Invalid timestamp
        return false;
      }

      Crypto  crypt;
      XString encrypt  = DeBase64(nonce) + created + shouldbePassword;
#ifdef _UNICODE
      AutoCSTR enc(encrypt);
      shouldbePassword = crypt.Digest(enc.cstr(),enc.size(),CALG_SHA1);
#else
      shouldbePassword = crypt.Digest(encrypt,encrypt.GetLength(),CALG_SHA1);
#endif
    }

    // Try if passwords do match
    if(password.Compare(shouldbePassword) == 0)
    {
      p_message->SetUser(username);
      return true;
    }

    // <Security> present but **NOT** valid!
    return false;
  }
  // No WS-Security present
  return true;
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
  XMLElement* envelope = p_message->FindElement(_T("Envelope"));
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
    p_message->SetAttribute(envelope,_T("xmlns:wsse"),NAMESPACE_SECEXT);
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
  XMLElement* header = p_message->FindElement(_T("Header"));
  if(header)
  {
    // <wsse:Security> **MUST** be a first child of the header node
    XMLElement* security = p_message->FindElement(header,_T("Security"),false);
    if(security)
    {
      return security;
    }
    // Add the security node to the SOAP header
    return p_message->AddElement(header,_T("wsse:Security"),_T(""));
  }
  return nullptr;
}

bool
SOAPSecurity::SetUsernameInMessage(SOAPMessage* p_message,XMLElement* p_secure)
{
  if(p_message && p_secure)
  {
    XMLElement* username = p_message->FindElement(p_secure,_T("Username"),false);
    if(username)
    {
      username->SetValue(m_username);
      return true;
    }
    else if(p_message->AddElement(p_secure,_T("wsse:Username"),m_username))
    {
      return true;
    }
  }
  return false;
}

bool
SOAPSecurity::SetPasswordInMessage(SOAPMessage* p_message,XMLElement* p_secure,XString p_password)
{
  if(p_message && p_secure)
  {
    XMLElement* password = p_message->FindElement(p_secure,_T("Password"),false);
    if(password)
    {
      password->SetValue(p_password);
    }
    else
    {
      password = p_message->AddElement(p_secure,_T("wsse:Password"),p_password);
    }
    if(password)
    {
      p_message->SetAttribute(password,_T("Type"),m_digest ? _T("wsse:PasswordDigest") : _T("wsse:PasswordText"));
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
    XMLElement* nonce = p_message->FindElement(p_secure,_T("Nonce"),false);
    if(nonce)
    {
      nonce->SetValue(m_nonce);
      return true;
    }
    else if(p_message->AddElement(p_secure,_T("wsse:Nonce"),m_nonce))
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
    // Timestamp in timezone 'Z' (Zebra) for UTC
    XString timestamp = m_timestamp.AsString() + _T("Z");
    XMLElement* stamp = p_message->FindElement(p_secure,_T("Created"),false);
    if(stamp)
    {
      stamp->SetValue(timestamp);
      return true;
    }
    stamp = p_message->AddElement(p_secure,_T("wsu:Created"),timestamp);
    if(stamp)
    {
      p_message->SetAttribute(stamp,_T("xmlns:wsu"),_T("http://schemas.xmlsoap.org/ws/2002/07/utility"));
      return true;
    }
  }
  return false;
}

// Create a new nonce and a digested password string
XString 
SOAPSecurity::DigestPassword()
{
  XString nonce = GenerateGUID();
  nonce.Remove('-');
  GenerateNonce(nonce);

  // According to the standard. This is how we scramble the password
  XString encrypted = nonce + m_timestamp.AsString() + XString(_T("Z")) + m_password;

  Crypto crypt;
#ifdef _UNICODE
  AutoCSTR enc(encrypted);
  return crypt.Digest(enc.cstr(),enc.size(),CALG_SHA1);
#else
  return crypt.Digest(encrypted,encrypted.GetLength(),CALG_SHA1);
#endif
}

void
SOAPSecurity::GenerateNonce(const XString& p_nonce)
{
  Base64 base;
  m_nonce = base.Encrypt(p_nonce);
}

// Incoming control field
XString 
SOAPSecurity::FindHeaderField(SOAPMessage* p_message,XMLElement* p_secure,const XString& p_field)
{
  XString value;

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

XString 
SOAPSecurity::DeBase64(const XString& p_field)
{
  Base64  base;
  return base.Decrypt(p_field);
}
