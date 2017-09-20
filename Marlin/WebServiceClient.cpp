/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebServiceClient.cpp
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
#include "stdafx.h"
#include "WebServiceClient.h"
#include "GenerateGUID.h"
#include "SOAPSecurity.h"
#include <objbase.h>
#include <rpc.h>
#include <wincrypt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// General logfile macro
#undef  DETAILLOG
#define DETAILLOG(text,...)  if(m_logfile)m_logfile->AnalysisLog(__FUNCTION__,LogType::LOG_INFO,true,text,__VA_ARGS__)
#define DETAILLOG1(text)     if(m_logfile)m_logfile->AnalysisLog(__FUNCTION__,LogType::LOG_INFO,false,text)
#undef  ERRORLOG
#define ERRORLOG(text)       if(m_logfile)m_logfile->AnalysisLog(__FUNCTION__,LogType::LOG_ERROR,false,text);

WebServiceClient::WebServiceClient(CString p_contract
                                  ,CString p_url
                                  ,CString p_wsdl
                                  ,bool    p_reliable)
           :m_contract(p_contract)
           ,m_url(p_url)
           ,m_wsdlFile(p_wsdl)
           ,m_reliable(p_reliable)
{
  // Set semi-constants from SOAPMessage.h
  m_rm  = NAMESPACE_RELIABLE;
  m_adr = NAMESPACE_WSADDRESS;
  m_env = NAMESPACE_ENVELOPE;

  // Initialize COM+ for GUID creation
  // S_FALSE is for already initialized
  HRESULT res = CoInitialize(NULL);
  if(res != S_OK && res != S_FALSE)
  {
    m_errorText = "Cannot initialize the GUID creation interface. "
                  "Apartment threading changed?";
  }
}

WebServiceClient::~WebServiceClient()
{
  // Do not really close on destruction.
  // You should call this method earlier on, before all static objects destruct
  Close();

  // Release COM+ modules
  CoUninitialize();
}

void
WebServiceClient::SetTimeouts(int p_resolve,int p_connect,int p_send,int p_receive)
{
  // Check for correct timeout values
  if(p_resolve < MIN_TIMEOUT_RESOLVE) p_resolve = MIN_TIMEOUT_RESOLVE;
  if(p_connect < MIN_TIMEOUT_CONNECT) p_connect = MIN_TIMEOUT_CONNECT;
  if(p_send    < MIN_TIMEOUT_SEND)    p_send    = MIN_TIMEOUT_SEND;
  if(p_receive < MIN_TIMEOUT_RECEIVE) p_receive = MIN_TIMEOUT_RECEIVE;
  // remember these values
  m_timeoutResolve = p_resolve;
  m_timeoutConnect = p_connect;
  m_timeoutSend    = p_send;
  m_timeoutReceive = p_receive;
}

void
WebServiceClient::SetLogAnalysis(LogAnalysis* p_log)
{
  if(m_logfile && m_logOwner)
  {
    delete m_logfile;
    m_logfile = nullptr;
  }
  m_logfile  = p_log;
  m_logOwner = false;
}

void
WebServiceClient::SetHTTPClient(HTTPClient* p_client)
{
  if(m_httpClient && m_owner)
  {
    delete m_httpClient;
    m_httpClient = nullptr;
  }
  m_httpClient = p_client;
  m_owner = false;
}

void
WebServiceClient::SetLogLevel(int p_logLevel)
{
  // Check boundaries
  if(p_logLevel < HLL_NOLOG)   p_logLevel = HLL_NOLOG;
  if(p_logLevel > HLL_HIGHEST) p_logLevel = HLL_HIGHEST;

  // Setting the loglevel
  m_logLevel = p_logLevel;
  if (m_logfile)
  {
    m_logfile->SetLogLevel(p_logLevel);
  }
}

// Old loglevel interface
bool
WebServiceClient::GetDetailLogging()
{
  TRACE("WARNING: Rewrite your program with GetLogLevel()\n");
  return (m_logLevel > HLL_ERRORS);
}

void
WebServiceClient::SetDetailLogging(bool p_detail)
{
  TRACE("WARNING: Rewrite your program with SetLogLevel()\n");
  m_logLevel = p_detail ? HLL_LOGGING : HLL_NOLOG;
}

bool
WebServiceClient::Open()
{
  if(m_isopen || m_isSending)
  {
    CString error;
    error.Format("WebServiceClient is already open for: %s",m_contract.GetString());
    DETAILLOG1(error);
    m_errorText += error;
    throw error;
  }
  DETAILLOG1("WebServiceClient starting");

  // Create the client if not given in advance
  // and set the URL to this client
  if(m_httpClient == NULL)
  {
    DETAILLOG1("Creating new HTTPClient");
    m_owner  = true;
    m_httpClient = new HTTPClient();
    // Propagate SOAP compression
    m_httpClient->SetSoapCompress(m_soapCompress);
    // Propagate all timeout values
    m_httpClient->SetTimeoutResolve(m_timeoutResolve);
    m_httpClient->SetTimeoutConnect(m_timeoutConnect);
    m_httpClient->SetTimeoutSend   (m_timeoutSend);
    m_httpClient->SetTimeoutReceive(m_timeoutReceive);
  }

  if(m_logfile == NULL)
  {
    DETAILLOG1("Creating new logfile");
    m_logOwner = true;
    m_logfile = new LogAnalysis("WebServiceClient");
    m_logfile->SetLogFilename(m_logFilename);
    m_logfile->SetLogLevel(m_logLevel);
    m_logfile->SetDoTiming(true);
    m_logfile->SetDoEvents(false);
    m_logfile->SetCache(1000);

    m_httpClient->SetLogging(m_logfile);
    m_httpClient->SetLogLevel(m_logfile->GetLogLevel());
  }

  // Propagate URL
  m_httpClient->SetURL(m_url);
  DETAILLOG("WebServiceClient for URL: %s",m_url.GetString());

  // Perform general check
  MinimumCheck();

  if(!m_wsdlFile.IsEmpty())
  {
    m_wsdl.SetLogAnalysis(m_logfile);

    bool res = m_wsdl.ReadWSDLFile(m_wsdlFile);
    DETAILLOG("WSDL checks out: %s",res ? "OK" : "WITH ERRORS!");
  }

  // See if we must create a token profile provider
  if(m_tokenProfile && m_soapSecurity == nullptr)
  {
    m_soapSecurity = new SOAPSecurity();

    m_soapSecurity->SetUser(m_user);
    m_soapSecurity->SetPassword(m_password);
    m_soapSecurity->SetDigesting(true);
  }

  // Currently not sending
  m_isSending = false;

  // See if we must do WS-ReliableMessaging
  if(m_reliable == false)
  {
    return (m_isopen = true);
  }

  // RM Needs a sequences
  CreateSequence();

  // Gotten to the end without a throw
  DETAILLOG1("WebServiceClient ready to go");
  return (m_isopen = true);
}

// Closing
void
WebServiceClient::Close()
{
  DETAILLOG1("Closing WebServiceClient");
  if(m_isopen && m_reliable && m_result)
  {
    try
    {
      // Send a LastMessage
      LastMessage();

      // Send a TerminateSequence
      TerminateSequence();
    }
    catch(CString& error)
    {
      CString er = "ERROR in closing the ReliableMessaging protocol: " + error;
      ERRORLOG(er);
    }
    catch(...)
    {
      ERRORLOG("Unexpected ERROR in closing WS-RM protocol!");
    }
  }
  // Ended RM without a throw
  DETAILLOG1("WebServiceClient closed");
  m_isopen    = false;
  m_isSending = false;

  // Destruct our client
  if(m_owner && m_httpClient)
  {
    delete m_httpClient;
    m_httpClient = nullptr;
    m_owner = false;
  }

  // Destruct message store, freeing memory
  // New connections on Open() will need their own messagestore
  if(m_messages.size())
  {
    DETAILLOG1("Freeing the WS message store");
    for(auto msg : m_messages)
    {
      delete msg.m_message;
    }
    m_messages.clear();
  }
  m_errorText.Empty();

  // All logging done. destruct logfile
  if(m_logfile && m_logOwner)
  {
    delete m_logfile;
    m_logfile = nullptr;
    m_logOwner = false;
  }

  // Destroy SOAPSecurity
  if(m_soapSecurity)
  {
    delete m_soapSecurity;
    m_soapSecurity = nullptr;
  }
}

void
WebServiceClient::StopSendport()
{
  if(m_isopen)
  {
    DETAILLOG1("WebServiceClient stopping sendport");
    m_httpClient->StopClient();
  }
}

// Now really sending the XML
bool
WebServiceClient::Send(SOAPMessage* p_message)
{
  // Program error. Already busy sending
  if(m_isSending)
  {
    return false;
  }

  // If not opened: try to open it now
  if(!m_isopen)
  {
    Open();
    if(!m_isopen)
    {
      return false;
    }
  }

  // Busy sending from now on
  m_isSending = true;

  // Name of call for error messages
  m_request = p_message->GetSoapAction();

  // Check whether message is for us
  if(p_message->GetURL().IsEmpty())
  {
    p_message->SetURL(m_url);
  }
  if(p_message->GetNamespace().IsEmpty() && p_message->GetSoapVersion() == SoapVersion::SOAP_12)
  {
    p_message->SetNamespace(m_contract);
  }

  // Check WS-Security Encryption-levels
  // Do this before reliability check and message store!
  if(m_encryptionLevel != XMLEncryption::XENC_Plain)
  {
    p_message->SetSecurityLevel(m_encryptionLevel);
    p_message->SetSecurityPassword(m_encryptionPassword);
    p_message->SetSigningMethod(m_signingMethod);
  }

  // Keep message to a total of StoreSize messages
  if(m_reliable)
  {
    ReliabilityChecks(p_message);
    PrepareForReliable(p_message);
  }

  // Provide the authentication of this message
  if(m_tokenProfile)
  {
    m_soapSecurity->SetSecurity(p_message);

    // Remove user/password from the message
    CString empty;
    p_message->SetUser(empty);
    p_message->SetPassword(empty);
  }
  else if(p_message->GetUser().IsEmpty())
  {
    // If user/password not set in the message, user ours (if any)
    p_message->SetUser(m_user);
    p_message->SetPassword(m_password);
  }

  // Send by the HTTP client
  if(m_jsonTranslation)
  {
    // Doing the SOAP -> JSON -> SOAP roundtrip
    m_result = m_httpClient->SendAsJSON(p_message);
  }
  else
  {
    // Send directly as a SOAP service
    m_result = m_httpClient->Send(p_message);
  }

  // Generic error handling
  if(m_result == false)
  {
    ErrorHandling(p_message);
  }
  else if(m_reliable)
  {
    // Check results for consistency (WS-RM)
    CheckHeader(p_message);
  }

  // Check if we have to retransmit last messages
  DoRetransmit();

  // No longer sending
  m_isSending = false;

  return m_result;
}

// Add message to queue (NOT WS-Reliable!!)
// Pass through to the HTTPClient queue.
bool
WebServiceClient::AddToQueue(SOAPMessage* p_message)
{
  if(m_isSending || !m_isopen)
  {
    return false;
  }

  // Check WS-Security Encryption-levels
  if(m_encryptionLevel != XMLEncryption::XENC_Plain)
  {
    p_message->SetSecurityLevel(m_encryptionLevel);
    p_message->SetSecurityPassword(m_encryptionPassword);
    p_message->SetSigningMethod(m_signingMethod);
  }

  m_httpClient->AddToQueue(p_message);
  return true;
}

void
WebServiceClient::ReliabilityChecks(SOAPMessage* p_message)
{
  // WS-ReliableMessaging does SOAP 1.2 instead of 1.0 of 1.1
  if(p_message->GetSoapVersion() != SoapVersion::SOAP_12)
  {
    CString error("Webservice calls under reliability protocol must be of SOAP XML version 1.2");
    DETAILLOG1(error);
    m_errorText += error;
    throw error;
  }

  // Override of "text/xml"
  if(p_message->GetContentType().CompareNoCase("application/soap+xml"))
  {
    // Reliable messaging requests high level content-type
    p_message->SetContentType("application/soap+xml");
  }
}

void
WebServiceClient::ErrorHandling(SOAPMessage* p_message)
{
  if(!p_message->GetFaultCode().IsEmpty())
  {
    // XML-error stack or HTML error present
    m_errorText.Format("ERROR: %s\n"
                       "\n"
                       "WS Fault actor : %s\n"
                       "WS Fault code  : %s\n"
                       "WS Fault string: %s\n"
                       "WS Fault detail: %s\n"
                       "\n"
                       "Total WS-SOAP error stack:\n"
                       "\n"
                       "%s"
                       ,m_httpClient->GetStatusText().GetString()
                       ,p_message->GetFaultCode().GetString()
                       ,p_message->GetFaultActor().GetString()
                       ,p_message->GetFaultString().GetString()
                       ,p_message->GetFaultDetail().GetString()
                       ,p_message->GetSoapMessage().GetString());
  }
  else if(m_httpClient->GetStatus() != HTTP_STATUS_OK)
  {
    // HTTP Protocol fout (connection etc)
    m_errorText.Format("HTTP WS Protocol error. %s\n%s\n"
                      ,m_httpClient->GetStatusText().GetString()
                      ,p_message->GetSoapMessage().GetString());
  }
}

void      
WebServiceClient::SetReliable(bool p_reliable,ReliableType p_type /*= RELIABLE_ONCE*/)
{
  if(m_isopen == false)
  {
    m_reliable = p_reliable;

    if(p_type >= ReliableType::RELIABLE_1ATMOST && p_type <= ReliableType::RELIABLE_ATLEAST1)
    {
      // 0 = Transmit/Receive at most 1
      // 1 = Transmit/Receive exactly one!!
      // 2 = Transmit/Receive at least 1
      m_reliableType = p_type;
    }
    return;
  }
  CString error("WebServiceClient: Cannot set attribute 'reliable' after opening.");
  DETAILLOG1(error);
  m_errorText += error;
  throw error;
}

void      
WebServiceClient::SetStoreSize(int p_size)
{
  if(m_isopen == false)
  {
    if(p_size >= MESSAGESTORE_MINIMUM && 
       p_size  < MESSAGESTORE_MAXIMUM )
    {
      m_storeSize = p_size;
      return;
    }
    CString error;
    error.Format("WebServiceClient: Message store size out of range [%d:%d]",MESSAGESTORE_MINIMUM,MESSAGESTORE_MAXIMUM);
    DETAILLOG1(error);
    m_errorText += error;
    throw error;
  }
  CString error("WebServiceClient: Cannot set message store size after 'Open' for: " + m_contract);
  DETAILLOG1(error);
  m_errorText += error;
  throw error;
}

//////////////////////////////////////////////////////////////////////////
//
//  Check protocols in return header
//  1) Correct MessageNumber
//  2) Sequence complete or resend messages
//  3) Relates to our request (MessageID)
//  4) Check that the answering protocol is known by us
// 
//////////////////////////////////////////////////////////////////////////

void      
WebServiceClient::CheckHeader(SOAPMessage* p_message)
{
  // Do only for WS-ReliableMessaging
  if(m_reliable == false)
  {
    return;
  }
  // 1: CHECK IF <r:Sequence> HAS THE CORRECT MessageNumber IN IT
  CheckHeaderHasSequence(p_message);

  // 2: CHECK IF <r:SequenceAcknowledgement> HAS A COMPLETE SERIES
  CheckHeaderHasAcknowledgement(p_message);

  // 3: CHECK IF <a:RelatesTo> RELATES TO OUR MESSAGE
  CheckHeaderRelatesTo(p_message);
}

//////////////////////////////////////////////////////////////////////////
//
// Detailed functions of CheckHeader
//
//////////////////////////////////////////////////////////////////////////

void
WebServiceClient::CheckHeaderHasSequence(SOAPMessage* p_message)
{
  if(m_guidSequenceClient.CompareNoCase(p_message->GetClientSequence()))
  {
    // Answer for another sequence
    CString error;
    error.Format("Incorrect Sequence Identifier in header on WS-ReliableMessaging for call [%s/%s]",m_contract.GetString(),p_message->GetSoapAction().GetString());
    DETAILLOG1(error);
    m_errorText += error;
    throw error;
  }

  int answerNumber = p_message->GetServerMessageNumber();
  if((m_serverMessageNumber + 1) == answerNumber)
  {
    // YOEHEI!! Server has sent another message!!
    ++m_serverMessageNumber;
  }
  else
  {
    // NOT OUR MESSAGE
    // reliableType == 0 -> Ignore
    // reliableType == 1 -> Throw error
    // reliableType == 2 -> Retransmit original 
    if(m_reliableType == ReliableType::RELIABLE_ADRESSING)
    {
      CString error;
      error.Format("WS-ReliableMessaging server message number out-of-sequence for call [%s/%s]",m_contract.GetString(),p_message->GetSoapAction().GetString());
      DETAILLOG1(error);
      m_errorText += error;
      throw error;
    }
    if(m_reliableType == ReliableType::RELIABLE_1ATMOST)
    {
      // Mark last message for retransmit
      m_retransmitLast = true;      
    }
  }
  if(m_inLastMessage)
  {
    if(p_message->GetLastMessage() == false)
    {
      CString error;
      error.Format("Incorrect Sequence response. No correct 'LastMessage' response in WS-ReliableMessaging for call [%s/%s]",m_contract.GetString(),p_message->GetSoapAction().GetString());
      DETAILLOG1(error);
      m_errorText += error;
      throw error;
    }
  }
}

void
WebServiceClient::CheckHeaderHasAcknowledgement(SOAPMessage* p_message)
{
  CString ident = p_message->GetServerSequence();
  if(ident.CompareNoCase(m_guidSequenceServer))
  {
    // Not an acknowledge from our server sequence
    CString error;
    error.Format("Sequence Acknowledgment requested is not for [%s/%s]",m_contract.GetString(),p_message->GetSoapAction().GetString());
    DETAILLOG1(error);
    m_errorText += error;
    throw error;
  }
  // Find ranges
  RangeMap& ranges = p_message->GetRangeMap();
  RangeMap::iterator it;
  for(it = ranges.begin(); it != ranges.end(); ++it)
  {
    // Range incomplete -> Retransmit
    CheckMessageRange(it->m_lower,it->m_upper);
  }
}

void
WebServiceClient::CheckHeaderRelatesTo(SOAPMessage* p_message)
{
  CString answerMessageID = p_message->GetMessageNonce();
  if(answerMessageID.IsEmpty())
  {
    return;
  }
  if(m_messageGuidID.CompareNoCase(answerMessageID))
  {
    // Not related to our call. No <RelatesTo> in answer
    CString error;
    error.Format("Out of band answer on call from [%s/%s]. Wrong message ID",m_contract.GetString(),p_message->GetSoapAction().GetString());
    DETAILLOG1(error);
    m_errorText += error;
    throw error;
  }
}

// Check message store for needed retransmits
// Number of messages in the deque is always very low
void
WebServiceClient::CheckMessageRange(int lower,int upper)
{
  for(unsigned int ind = 0;ind < m_messages.size(); ++ind)
  {
    SoapMsg& mesg = m_messages[ind];
    if(mesg.m_messageNumber >= lower && mesg.m_messageNumber <= upper)
    {
      mesg.m_retransmit = false;
    }
  }
}

// After acknowledge-ranges are checked
void      
WebServiceClient::DoRetransmit()
{
  // Nothing to do
  if(m_reliableType == ReliableType::RELIABLE_NONE    ||
     m_reliableType == ReliableType::RELIABLE_ADRESSING)
  {
    return;
  }
  if(m_reliableType == ReliableType::RELIABLE_1ATMOST ||
     m_reliableType == ReliableType::RELIABLE_ONCE)
  {
    // Transmit at most 1. So don't retransmit
    // Transmit exactly 1. So don't retransmit
    for (auto msg : m_messages)
    {
      delete msg.m_message;
    }
    m_messages.clear();
    return;
  }
  if(m_reliableType == ReliableType::RELIABLE_ATLEAST1)
  {
    // Else type = 2. Retransmit messages not gotten
    for(MessageStore::iterator it = m_messages.begin();it != m_messages.end(); ++ it)
    {
      SoapMsg* mesg = &(*it);
      if(mesg->m_retransmit)
      {
        DETAILLOG1("*** RETRANSMIT from message store ***");
        m_request = mesg->m_message->GetSoapAction();
        m_httpClient->Send(mesg->m_message);
      }
    }
    // Or simply retransmit last message
    if(m_retransmitLast)
    {
      m_retransmitLast = false;
      if(m_messages.size())
      {
        MessageStore::iterator it = m_messages.end();
        // Go to last message
        --it;
        SoapMsg* mesg = &(*it);

        DETAILLOG1("*** RETRANSMIT from message store ***");
        m_request = mesg->m_message->GetSoapAction();
        m_httpClient->Send(mesg->m_message);
      }
    }
  }
}

void
WebServiceClient::PutInMessageStore(SOAPMessage* p_message)
{
  // Put in the message store
  SoapMsg soap;
  soap.m_message       = new SOAPMessage(p_message);
  soap.m_retransmit    = true;
  soap.m_messageNumber = m_clientMessageNumber;
  m_messages.push_back(soap);

  // If more messages are present: forget them
  // if requested later for retransmit, it will be too late!
  if(m_messages.size() > (size_t)m_storeSize)
  {
    delete m_messages[0].m_message;
    m_messages.pop_front();
  }
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE METHODS
//
//////////////////////////////////////////////////////////////////////////

// Check if the minimum conditions have been met for the defining of the
// 'endpoint' of the service (url,port,servie) and the optional contract
void
WebServiceClient::MinimumCheck()
{
  if(m_url.IsEmpty())
  {
    CString error("WebServiceClient without an endpoint URL definition.");
    DETAILLOG1(error);
    m_errorText += error;
    throw error;

  }
  // Contract is in the SOAP-header for the WS-ReliableMessaging protocol
  if(m_reliable == false && m_contract.IsEmpty())
  {
    CString error("WebServiceClient without a contract definition, needed for reliable messaging.");
    DETAILLOG1(error);
    m_errorText += error;
    throw error;
  }

  // JSON Translation cannot be done in reliable messaging mode or in encryption modes!
  if(m_jsonTranslation && (m_reliable || m_encryptionLevel != XMLEncryption::XENC_Plain))
  {
    CString error("WebServiceClient cannot do SOAP->JSON->SOAP translation when in reliable messaging or encryption mode!");
    DETAILLOG1(ERROR);
    m_errorText += error;
    throw error;
  }
}

//////////////////////////////////////////////////////////////////////////
// 
// START/STOP WS-RM
//
//////////////////////////////////////////////////////////////////////////

void      
WebServiceClient::PrepareForReliable(SOAPMessage* p_message)
{
  // Send a new message, so a new client message ID is needed
  ++m_clientMessageNumber;
  m_messageGuidID = GenerateGUID();

  // IF no RM contract set, use that of our application
  if(p_message->GetNamespace().IsEmpty())
  {
    p_message->SetNamespace(m_contract);
  }
  // Pass on to the message
  p_message->SetClientSequence(m_guidSequenceClient);
  p_message->SetServerSequence(m_guidSequenceServer);
  p_message->SetClientMessageNumber(m_clientMessageNumber);
  p_message->SetServerMessageNumber(m_serverMessageNumber);
  p_message->SetMessageNonce(m_messageGuidID);
  p_message->SetAddressing(true);
  // If everything above is set, ONLY THEN will switching to RM work!
  p_message->SetReliability(m_reliable);

  PutInMessageStore(p_message);
}

void
WebServiceClient::CreateSequence()
{
  // GUID of the first message
  CString sequenceID;

  m_messageGuidID     = "urn:uuid:" + GenerateGUID();
  // Proposition of a GUID for a sequence of messages
  CString guidOfferID = "urn:uuid:" + GenerateGUID();

  // Set for sending
  m_request = "CreateSequence";

  // Create request "CreateSequence" message
  SOAPMessage message(m_rm,m_request,SoapVersion::SOAP_12,m_url);
  message.SetMessageNonce(m_messageGuidID);
  message.SetAddressing(true);
  message.SetReliability(true,false);

  // Set message parameters (AcksTo:Address and Offer:Identifier)
  XMLElement* acksto = message.SetParameter("AcksTo","");
  XMLElement* offer  = message.SetParameter("Offer" ,"");
  message.SetElement(acksto,"adr:Address",m_adr + "/anonymous");
  message.SetElement(offer, "Identifier", guidOfferID);

  // Check WS-Security Encryption-levels
  if(m_encryptionLevel != XMLEncryption::XENC_Plain)
  {
    message.SetSecurityLevel   (m_encryptionLevel);
    message.SetSecurityPassword(m_encryptionPassword);
    message.SetSigningMethod   (m_signingMethod);
  }

  // Set username-password combination
  if(m_tokenProfile)
  {
    m_soapSecurity->SetSecurity(&message);
  }
  else
  {
    message.SetUser(m_user);
    message.SetPassword(m_password);
  }

  // Put in the logfile
  DETAILLOG1("*** Starting WS-ReliableMessaging protocol ***");

  try
  {
    // Send directly via the client
    // Not via Send(). Does not go in messageStore
    bool ok = m_httpClient->Send(&message);
    if(ok == false)
    {
      // Throw
      ErrorHandling(&message);
      DETAILLOG1(m_errorText);
      throw m_errorText;
    }

    CString acceptURL;
    sequenceID = message.GetParameter("Identifier");
    XMLElement* accept  = message.FindElement("Accept");
    XMLElement* address = message.FindElement(accept,"Address");
    if(address)
    {
      acceptURL = address->GetValue();
    }

    // Check that the server responds to our call
    if(m_messageGuidID.CompareNoCase(message.GetMessageNonce()))
    {
        // Error: answer fro a previous message
        CString fout("Received a 'CreateSequenceResponse' on another message from another program/process or thread.");
        DETAILLOG1(fout);
        m_errorText += fout;
        throw fout;
    }

    // Check that the server accepts our URL
    CString clientURL = m_url;
    acceptURL.MakeLower();
    clientURL.MakeLower();

    // De-normalize the URL's to remove redundant ":80/" and ":443" ports and the-like
    CrackedURL normAccept(acceptURL);
    CrackedURL normClient(clientURL);

    if(normClient.URL().Find(normAccept.URL()) < 0)
    {
      CString error("Received a 'CreateSequenceResponse' that does not answer to our URL address.");
      DETAILLOG1(error);
      m_errorText += error;
      throw error;
    }

    // Check that we have both
    if(m_messageGuidID.IsEmpty() || acceptURL.IsEmpty())
    {
      CString error("Received a 'CreateSequenceResponse' without an proper 'Accept'");
      DETAILLOG1(error);
      m_errorText += error;
      throw error;
    }
  }
  catch(CString& error)
  {
    CString fout;
    fout  = "WebService CreateSequence in WS-ReliableMessage protocol failed.\n";
    fout += error;

    fout += "\n";
    fout += message.GetFault();

    DETAILLOG1(fout);
    m_errorText += fout;
    throw fout;
  }
  // CreateSequence did work
  // Remember the GUID of the sequence
  m_guidSequenceServer = sequenceID;
  m_guidSequenceClient = guidOfferID;
}

void
WebServiceClient::LastMessage()
{
  CString request("LastMessage");
  SOAPMessage message(m_rm,request,SoapVersion::SOAP_12,m_url);

  // Set username-password combination
  if(m_tokenProfile)
  {
    m_soapSecurity->SetSecurity(&message);
  }
  else
  {
    message.SetUser(m_user);
    message.SetPassword(m_password);
  }

  DETAILLOG1("*** Ending WS-ReliableMessaging protocol ***");
  try
  {
    // Send and do error handling
    // Send via Send (and in messageStore)
    bool ok = Send(&message);
    if(ok == false)
    {
      CString error("No answer on 'LastMessage' call");
      DETAILLOG1(error);
      m_errorText += error;
      throw error;
    }
  }
  catch(CString& error)
  {
    CString msg;
    msg  = "WebService LastMessage in WS-ReliableMessage protocol did fail.\n";
    msg += error;
    DETAILLOG1(msg);
    m_errorText += msg;
    throw msg;
  }
}

void
WebServiceClient::TerminateSequence()
{
  CString terminate("TerminateSequence");
  SOAPMessage message(m_rm,terminate,SoapVersion::SOAP_12,m_url);
  message.SetReliability(true,false);
  message.SetParameter("Identifier",m_guidSequenceServer);

  // Set username-password combination
  if(m_tokenProfile)
  {
    m_soapSecurity->SetSecurity(&message);
  }
  else
  {
    message.SetUser(m_user);
    message.SetPassword(m_password);
  }

  try
  {
    CString error;
    // Send and do error handling
    CString request("TerminateSequence");
    // Send directly via client
    // Not via Send(). Not in MessageStore!
    bool ok = Send(&message);

    if(ok)
    {
      CString clientIdentifier = message.GetParameter("Identifier");
      if(clientIdentifier.CompareNoCase(m_guidSequenceClient))
      {
        // Error: wrong terminate
        error.Format("Wrong TerminateSequence in answer on call from [%s/%s].",m_rm.GetString(),request.GetString());
        DETAILLOG1(error);
        m_errorText += error;
        throw error;
      }
      // End reached: no throw
    }
    else
    {
      // Error no envelope
      error.Format("No correct answer on [%s%s]",m_rm.GetString(),request.GetString());
      DETAILLOG1(error);
      m_errorText += error;
      throw error;
    }
  }
  catch(CString& error)
  {
    CString msg;
    msg  = "WebService TerminateSequence in WS-ReliableMessage protocol failed.\n";
    msg += error;
    DETAILLOG1(msg);
    throw msg;
  }
  DETAILLOG1("*** WS-ReliableMessaging protocol terminated correctly ***");
}
