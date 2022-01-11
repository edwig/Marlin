/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebServiceClient.h
//
// Marlin Server: Internet server/client
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

// This client is a intermediate layer between the client and the generic HTTPClient
// All handling of th WS-ReliableMessaging protcol is done in this layer
// Message numbers and Start/Terminate sequences ar kept here and the
// needed protocols and namespaces are stored here.
// 
// Also the handling of the WS-Security has been added with minimal:
// - Message signing in the header
// - Message-body encryption
// - Whole message encryption

#pragma once
#include "HTTPClient.h"
#include "SOAPMessage.h"
#include "WSDLCache.h"
#include "Analysis.h"
#include <deque>

// Default and maximum nr. of messages to store
constexpr auto MESSAGESTORE_MINIMUM = 0;
constexpr auto MESSAGESTORE_DEFAULT = 10;
constexpr auto MESSAGESTORE_MAXIMUM = 256;

// Forward declaration
class SOAPSecurity;

enum class ReliableType
{
  RELIABLE_NONE      = 0 // Just soap message
 ,RELIABLE_ADRESSING = 1 // Addressing in soap header
 ,RELIABLE_1ATMOST   = 2 // Transmit/receive at most  1 message
 ,RELIABLE_ONCE      = 3 // Transmit/receive exactly  1 message
 ,RELIABLE_ATLEAST1  = 4 // Transmit/receive at least 1 message
};

class SoapMsg
{
public:
  int          m_messageNumber;
  bool         m_retransmit;
  SOAPMessage* m_message;
};

// Keeping messages for retransmit in WS-RM
using MessageStore = std::deque<SoapMsg>;

// BEWARE: The WebServiceClient can throw CStrings in all of its behavior
//         Even an Open() or Close() can do this as they handle the WS-Reliable protocol
//
class WebServiceClient
{
public:
  WebServiceClient(CString p_contract
                  ,CString p_url
                  ,CString p_wsdlFile = ""
                  ,bool    p_reliable = false);
 ~WebServiceClient();

  // Opening and closing the sendport (HTTPClient)
  bool      Open();
  void      Close();
  void      StopSendport();
  // Send this message (w/o reliable protocol)
  bool      Send(SOAPMessage* p_message);
  // Add message to queue (NOT WS-Reliable!!)
  bool      AddToQueue(SOAPMessage* p_message);
 
  // General Getters
  bool      HasErrors()                  { return m_result == false;        };
  bool      GetIsOpen()                  { return m_isopen;                 };
  bool      GetWsdlCheck()               { return m_wsdlCheck;              };
  int       GetMessageNumber()           { return ++m_clientMessageNumber;  }; // Automatic increment!
  CString   GetUser()                    { return m_user;                   };
  CString   GetPassword()                { return m_password;               };
  CString   GetGuidSequenceServer()      { return m_guidSequenceServer;     };
  CString   GetGuidSequenceClient()      { return m_guidSequenceClient;     };
  CString   GetAddressing()              { return m_adr;                    };
  CString   GetErrorText()               { return m_errorText;              };
  bool      GetTokenProfile()            { return m_tokenProfile;           };
  bool      GetReliable()                { return m_reliable;               };
  CString   GetSecurityPassword()        { return m_encryptionPassword;     };
  bool      GetSoapCompress()            { return m_soapCompress;           };
  bool      GetJsonSoapTranslation()     { return m_jsonTranslation;        };
  int       GetLogLevel()                { return m_logLevel;               };
  CString   GetLogFilename()             { return m_logFilename;            };
  ReliableType  GetReliableType()        { return m_reliableType;           };
  XMLEncryption GetSecurityLevel()       { return m_encryptionLevel;        };
  unsigned      GetSigningMethod()       { return m_signingMethod;          };
  HTTPClient*   GetHTTPClient()          { return m_httpClient;             };
  WSDLCache&    GetWSDLCache()           { return m_wsdl;                   };
  CString       GetWSDLFilename()        { return m_wsdlFile;               };
  SOAPSecurity* GetSOAPSecurity()        { return m_soapSecurity;           };
  bool          GetDetailLogging();

  // General Setters
  void      SetHTTPClient(HTTPClient* p_client);
  void      SetStoreSize(int p_size);
  void      SetLogAnalysis(LogAnalysis* p_log);
  void      SetReliable(bool p_reliable,ReliableType p_type = ReliableType::RELIABLE_ONCE);
  void      SetTimeouts(int p_resolve,int p_connect,int p_send,int p_receive);
  void      SetUser(CString p_user)                         { m_user               = p_user;        };
  void      SetPassword(CString p_password)                 { m_password           = p_password;    };
  void      SetTokenProfile(bool p_token)                   { m_tokenProfile       = p_token;       };
  void      SetWsdlCheck(bool p_check)                      { m_wsdlCheck          = p_check;       };
  void      SetLogFilename(CString p_logFilename)           { m_logFilename        = p_logFilename; };
  void      SetSigningMethod(unsigned p_method)             { m_signingMethod      = p_method;      };
  void      SetSecurityLevel(XMLEncryption p_encryption)    { m_encryptionLevel    = p_encryption;  };
  void      SetSecurityPassword(CString p_password)         { m_encryptionPassword = p_password;    };
  void      SetSoapCompress(bool p_compress)                { m_soapCompress       = p_compress;    };
  void      SetWSDLFilename(CString p_file)                 { m_wsdlFile           = p_file;        };
  void      SetJsonSoapTranslation(bool p_json)             { m_jsonTranslation    = p_json;        };
  void      SetLogLevel(int p_logLevel);
  void      SetDetailLogging(bool p_detail);

private:
  void      MinimumCheck();
  void      ReliabilityChecks(SOAPMessage* p_message);
  void      ErrorHandling(SOAPMessage* p_message);

  // Extra messages for reliable messaging
  void      PrepareForReliable(SOAPMessage* p_message);
  void      CreateSequence();
  void      LastMessage();
  void      TerminateSequence();

  // Check protocols in return header
  void      CheckHeader(SOAPMessage* p_message);
  // Detail functions of CheckHeader
  void      CheckHeaderHasSequence       (SOAPMessage* p_message);
  void      CheckHeaderHasAcknowledgement(SOAPMessage* p_message);
  void      CheckHeaderRelatesTo         (SOAPMessage* p_message);

  // Check message store for needed retransmits
  void      CheckMessageRange(int lower,int upper);
  // Keep message in message store
  void      PutInMessageStore(SOAPMessage* p_message);
  // After ack-ranges are checked
  void      DoRetransmit();

  // PRIVATE DATA
  HTTPClient*   m_httpClient          { nullptr   };           // Sendport to send XML to
  WSDLCache     m_wsdl                { false     };           // Client side WSDL
  bool          m_owner               { false     };           // We created the HTTPClient
  bool          m_isopen              { false     };           // Status open/close
  CString       m_url;                                         // URL to send to
  CString       m_contract;                                    // Service contract
  CString       m_wsdlFile;                                    // URL/File for WSDL definitions
  CString       m_user;                                        // User for RM messages
  CString       m_password;                                    // Password for RM messages
  bool          m_tokenProfile        { false     };           // Use WS-Security tokenProfile authentication
  bool          m_reliable            { false     };           // Using WS-RM?
  bool          m_wsdlCheck           { false     };           // Using WSDL checks
  ReliableType  m_reliableType { ReliableType::RELIABLE_ONCE };// 0=AtMostOnce,1=ExactlyOnce,2=AtLeastOnce
  XMLEncryption m_encryptionLevel{ XMLEncryption::XENC_Plain };// Signing, body encryption, message encryption
  CString       m_encryptionPassword;                          // Password for encryption
  unsigned      m_signingMethod       { CALG_SHA1 };           // Signing method
  bool          m_jsonTranslation     { false     };           // Do SOAP->JSON round trip translation
  LogAnalysis*  m_logfile             { nullptr   };           // Logfile
  CString       m_logFilename;                                 // Filename for creating our own logfile
  bool          m_logOwner            { false     };           // Owner of the logfile
  int           m_logLevel            { HLL_NOLOG };           // Loglevel of the client
  bool          m_soapCompress        { false     };           // Compress SOAP web services

  // Send status
  bool          m_isSending           { false     };           // Currently in a send
  CString       m_errorText;                                   // Error fault stack
  bool          m_result              { true      };           // Correct result(true) or error(false)
  // Sequence for WS-ReliableMessaging
  CString       m_guidSequenceClient;                          // Offer GUID to server
  CString       m_guidSequenceServer;                          // Answer to createSequence
  int           m_clientMessageNumber { 0         };           // Serial incrementing
  int           m_serverMessageNumber { 0         };           // Serial incrementing
  CString       m_messageGuidID;                               // ID of current message
  CString       m_request;                                     // Current request name 
  bool          m_retransmitLast      { false     };           // Do again for ReliableType = RELIABLE_ATLEAST1
  bool          m_inLastMessage       { false     };           // Final termination
  // WS-Security 
  SOAPSecurity* m_soapSecurity         { nullptr  };           // Where we fill the SOAPMessage with the <Security> header
  // Messages in store for retransmit
  MessageStore  m_messages;                                    // All messages          
  int           m_storeSize           { MESSAGESTORE_DEFAULT };// Max number of messages to store
  // Timeouts
  int           m_timeoutResolve      { DEF_TIMEOUT_RESOLVE }; // Timeout resolving URL
  int           m_timeoutConnect      { DEF_TIMEOUT_CONNECT }; // Timeout connecting to URL
  int           m_timeoutSend         { DEF_TIMEOUT_SEND    }; // Timeout sending to URL
  int           m_timeoutReceive      { DEF_TIMEOUT_RECEIVE }; // Timeout receiving answer from URL

  // Semi-Constants: make mutable later
  CString       m_rm;  // WS-ReliableMessaging contract
  CString       m_adr; // WS-Addressing contract
  CString       m_env; // W3C Soap-envelope
};
