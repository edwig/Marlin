/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestContract.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
#include "TestServer.h"
#include "WebServiceServer.h"
#include "SiteHandlerSoap.h"
#include "XMLRestriction.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int totalChecks = 10;

// Testing an external WSDL file
const bool  test_external_wsdl   = false;
const char* MARLIN_WSDL_CONTRACT = "..\\ExtraParts\\MarlinWeb.wsdl";
const char* EASY_MATCH_CONTRACT  = "..\\ExtraParts\\EasyMatch.wsdl";
const char* K2W_CONTRACT         = "..\\ExtraParts\\K2WServices.wsdl";

XMLRestriction g_restrict("Language");

#define EXTERN_CONTRACT EASY_MATCH_CONTRACT

// Ordinals for the services
#define CONTRACT_MF   1 // First
#define CONTRACT_MS   2 // Second
#define CONTRACT_MT   3 // Third
#define CONTRACT_M4   4 // Fourth
#define CONTRACT_MV   5 // Fifth

// DERIVED CLASS FROM WebServiceServer

class TestContract: public WebServiceServer
{
public:
  TestContract(CString    p_name
              ,CString    p_webroot
              ,CString    p_url
              ,PrefixType p_channelType
              ,CString    p_targetNamespace
              ,unsigned   p_maxThreads);
  virtual ~TestContract();
protected:
  WEBSERVICE_MAP; // Using a WEBSERVICE mapping

  // Declare all our webservice call names
  // which will translate in the On.... methods
  WEBSERVICE_DECLARE(OnMarlinFirst)
  WEBSERVICE_DECLARE(OnMarlinSecond)
  WEBSERVICE_DECLARE(OnMarlinThird)
  WEBSERVICE_DECLARE(OnMarlinFourth)
  WEBSERVICE_DECLARE(OnMarlinFifth)
private:
  // Our functionality
  CString Translation(CString p_language,CString p_translation,CString p_word);
  // Set input/output languages
  CString m_language;
  CString m_translation;

};

// Implementation of the TestContract class

TestContract::TestContract(CString    p_name
                          ,CString    p_webroot
                          ,CString    p_url
                          ,PrefixType p_channelType
                          ,CString    p_targetNamespace
                          ,unsigned   p_maxThreads)
:WebServiceServer(p_name,p_webroot,p_url,p_channelType,p_targetNamespace,p_maxThreads)
{
}

TestContract::~TestContract()
{
}

// Mapping corresponding to the AddOperation of the WSDL
// Binding ordinals to the service declarations for the On..... members
WEBSERVICE_MAP_BEGIN(TestContract)
  WEBSERVICE(CONTRACT_MF,OnMarlinFirst)
  WEBSERVICE(CONTRACT_MS,OnMarlinSecond)
  WEBSERVICE(CONTRACT_MT,OnMarlinThird)
  WEBSERVICE(CONTRACT_M4,OnMarlinFourth)
  WEBSERVICE(CONTRACT_MV,OnMarlinFifth)
WEBSERVICE_MAP_END

//////////////////////////////////////////////////////////////////////////
//
// HERE ARE THE SERVICE HANDLERS!!
// Derived from the definition above in the WEBSERVICE_MAP
//
//////////////////////////////////////////////////////////////////////////

void
TestContract::OnMarlinFirst(int p_code,SOAPMessage* p_message)
{
  ASSERT(p_code == CONTRACT_MF);
  UNREFERENCED_PARAMETER(p_code);
  
  m_language = p_message->GetParameter("Language");
  // --- "---------------------------------------------- - ------
  qprintf("WSDL Contract: First: Setting base language    : OK\n");

  CString resp("ResponseFirst");
  p_message->SetSoapAction(resp);
  p_message->Reset(ResponseType::RESP_ACTION_NAME,m_targetNamespace);
  p_message->SetParameter("Accepted",m_language == "Dutch");

  --totalChecks;
}

void
TestContract::OnMarlinSecond(int p_code,SOAPMessage* p_message)
{
  ASSERT(p_code == CONTRACT_MS);
  UNREFERENCED_PARAMETER(p_code);

  m_translation = p_message->GetParameter("Translation");
  // --- "---------------------------------------------- - ------
  qprintf("WSDL Contract: Second: translation language    : OK\n");

  CString resp("ResponseSecond");
  p_message->SetSoapAction(resp);
  p_message->Reset(ResponseType::RESP_ACTION_NAME,m_targetNamespace);
  bool cando = false;
  if(m_translation == "English"  ||
     m_translation == "Français" ||
     m_translation == "Deutsch"  ||
     m_translation == "Espagnol")
  {
    cando = true;
    --totalChecks;
  }
  else
  {
    xerror();
    // --- "---------------------------------------------- - ------
    qprintf("WSDL Contract: Second: translation language    : ERROR\n");
  }
  p_message->SetParameter("CanDo",cando);
}

void
TestContract::OnMarlinThird(int p_code,SOAPMessage* p_message)
{
  ASSERT(p_code == CONTRACT_MT);
  UNREFERENCED_PARAMETER(p_code);

  CString word   = p_message->GetParameter("WordToTranslate");
  CString result = Translation(m_language,m_translation,word);
  // --- "---------------------------------------------- - ------
  qprintf("WSDL Contract: Translated givven               : OK\n");

  CString resp("ResponseThird");
  p_message->SetSoapAction(resp);
  p_message->Reset(ResponseType::RESP_ACTION_NAME,m_targetNamespace);
  // Optional parameter, need not be set!
  if(!result.IsEmpty())
  {
    p_message->SetParameter("TranslatedWord",result);
  }
  // Called 3 times
  --totalChecks;
}

void
TestContract::OnMarlinFourth(int p_code,SOAPMessage* /*p_message*/)
{
  ASSERT(p_code == CONTRACT_M4);
  UNREFERENCED_PARAMETER(p_code);

  // --- "---------------------------------------------- - ------
  qprintf("WSDL Contract: Wrong contract (Fourth)         : ERROR\n");
  xerror();
}

void
TestContract::OnMarlinFifth(int p_code,SOAPMessage* p_message)
{
  ASSERT(p_code == CONTRACT_MV);
  UNREFERENCED_PARAMETER(p_code);

  CString pi;
  XMLElement* pielem = p_message->FindElement("PiApprox");
  if(pielem)
  {
    pi = pielem->GetValue();
  }
  p_message->Reset(ResponseType::RESP_ACTION_NAME,m_targetNamespace);

  if(!pi.IsEmpty())
  {
    if(pi.Find("3.141592653589") >= 0)
    {
      // --- "---------------------------------------------- - ------
      qprintf("WSDL Contract: Found the value of PI as        : OK\n");

      p_message->SetParameter("TranslatedWord","pi");
    }
    else
    {
      // --- "---------------------------------------------- - ------
      qprintf("WSDL Contract: Wrong value of PI              : ERROR\n");
      xerror();
    }
  }
  else
  {
    // --- "---------------------------------------------- - ------
    qprintf("WSDL Contract: Wrong contract (Fifth)          : ERROR\n");
    xerror();
  }
}

//////////////////////////////////////////////////////////////////////////
//
// VERY LIMITED TRANSLATION FUNCTION :-)
//
CString 
TestContract::Translation(CString p_language,CString p_translation,CString p_word)
{
  if(p_language == "Dutch" && p_translation == "Français")
  {
    if(p_word == "altijd")  return CString("toujours");
    if(p_word == "maandag") return CString("lundi");
  }
  return CString("");
}

//////////////////////////////////////////////////////////////////////////
//
// PREPARING OUR WSDL, This is what will fill the WSDL file
//
//////////////////////////////////////////////////////////////////////////

void
AddOperations(WebServiceServer& p_server,CString p_contract)
{
  // Defining the names of the operations
  CString first ("MarlinFirst");
  CString second("MarlinSecond");
  CString third ("MarlinThird");
  CString fourth("MarlinFourth");
  CString fifth ("MarlinFifth");

  // Response with "Response" before name. Good testing WSDL / XSD
  CString respFirst ("ResponseFirst");
  CString respSecond("ResponseSecond");
  CString respThird ("ResponseThird");
  CString respFourth("ResponseFourth");
  CString respFifth ("ResponseFifth");

  // Defining input and output messages for the operations
  SOAPMessage input1 (p_contract,first);
  SOAPMessage output1(p_contract,respFirst);
  SOAPMessage input2 (p_contract,second);
  SOAPMessage output2(p_contract,respSecond);
  SOAPMessage input3 (p_contract,third);
  SOAPMessage output3(p_contract,respThird);
  SOAPMessage input4 (p_contract,fourth);
  SOAPMessage output4(p_contract,respFourth);
  SOAPMessage input5 (p_contract,fifth);
  SOAPMessage output5(p_contract,respFifth);

  g_restrict.AddEnumeration("English", "The English language");
  g_restrict.AddEnumeration("Deutsch", "Die Deutsche sprache");
  g_restrict.AddEnumeration("Espagnol","El lingua franca");
  g_restrict.AddEnumeration("Français","La plus belle langue");
  // Defining the parameters for all the operations

  // First: Getting an accepted language
  input1 .AddElement(NULL,"Language",WSDL_Mandatory | XDT_String, "string");
  input1 .AddElement(NULL,"Version", WSDL_Mandatory | XDT_Integer,   "int");
  output1.AddElement(NULL,"Accepted",WSDL_Mandatory | XDT_Boolean,  "bool");
  
  // Second: Getting an accepted translation
  XMLElement* trans = 
  input2 .AddElement(NULL,"Translation",WSDL_Mandatory | XDT_String, "string");
  output2.AddElement(NULL,"CanDo",      WSDL_Mandatory | XDT_Boolean,"bool");
  trans->SetRestriction(&g_restrict);

  // Third Getting the answer
  input3 .AddElement(NULL,"WordToTranslate",WSDL_Mandatory | XDT_String,"string");
  output3.AddElement(NULL,"TranslatedWord", WSDL_Optional  | XDT_String,"string");

  // Fourth. Recursive parameters and answers
  XMLElement* param   = input4.AddElement(NULL, "Parameters",   WSDL_Mandatory|XDT_Complex,"");
  XMLElement* lanFrom = input4.AddElement(param,"LanguageFrom", WSDL_Mandatory|XDT_String, "language");
  XMLElement* lanTo   = input4.AddElement(param,"LanguageTo",   WSDL_Mandatory|XDT_String, "language");
  XMLElement* datatp  = input4.AddElement(param,"DataTypes",    WSDL_Mandatory|XDT_Complex,"");
  input4.AddElement(datatp,"String",  WSDL_Optional|XDT_String,"string");
  input4.AddElement(datatp,"Diacrits",WSDL_Optional|XDT_String,"diacritics");

  XMLElement* answer  = input4.AddElement(NULL, "DoubleWord",   WSDL_Mandatory|XDT_Complex,"");
  input4.AddElement(answer,"WordToTranslate",WSDL_Mandatory|XDT_String,"to_be_translated");
  input4.AddElement(answer,"AlternativeWord",WSDL_Optional |XDT_String,"alternative");
  lanFrom->SetRestriction(&g_restrict);
  lanTo  ->SetRestriction(&g_restrict);

  output4.AddElement(NULL,"TranslatedWord",WSDL_Optional|XDT_String,"string");
  output4.AddElement(NULL,"TranslationAlt",WSDL_Optional|XDT_String,"string");

  // Fifth. Has another "Parameters" node and "DoubleWord"
  XMLElement* param5   = input5.AddElement(NULL, "Parameters",   WSDL_Mandatory|XDT_Complex,"");
  input5.AddElement(param5,"Dialect",      WSDL_Mandatory|XDT_String, "Dialect");
  input5.AddElement(param5,"Region",       WSDL_Mandatory|XDT_String, "Region");
  input5.AddElement(param5,"PiApprox",     WSDL_Optional |XDT_Float,  "Approximation of PI");
  XMLElement* datatyp  = input5.AddElement(param5,"DataTypes", WSDL_Optional|XDT_Complex,"");
  input5.AddElement(datatyp,"MinLength",  WSDL_Mandatory|XDT_Integer,"int");
  input5.AddElement(datatyp,"MaxLength",  WSDL_Mandatory|XDT_Integer,"int");
  input5.AddElement(datatyp,"MaxDecimals",WSDL_Mandatory|XDT_Integer,"int");
  
  XMLElement* answer5  = input5.AddElement(NULL, "DoubleWord",   WSDL_Mandatory|XDT_Complex,"Wording");
  input5.AddElement(answer5,"Word",        WSDL_Mandatory|XDT_String,"to_be_translated");
  input5.AddElement(answer5,"Alternative", WSDL_Optional |XDT_String,"alternative");

  output5.AddElement(NULL,"TranslatedWord",WSDL_Optional|XDT_String,"string");
  output5.AddElement(NULL,"TranslationAlt",WSDL_Optional|XDT_String,"string");

  // Putting the operations in the WSDL Cache
  p_server.AddOperation(CONTRACT_MF,first, &input1,&output1);
  p_server.AddOperation(CONTRACT_MS,second,&input2,&output2);
  p_server.AddOperation(CONTRACT_MT,third, &input3,&output3);
  p_server.AddOperation(CONTRACT_M4,fourth,&input4,&output4);
  p_server.AddOperation(CONTRACT_MV,fifth, &input5,&output5);
}

//////////////////////////////////////////////////////////////////////////
//
// The final testing function
//
//////////////////////////////////////////////////////////////////////////

int 
TestJsonServer(HTTPServer* p_server,CString p_contract,int p_loglevel)
{
  CString  url;
  CString  name("MarlinExtra");
  CString  webroot("C:\\WWW\\Marlin");
  unsigned threads = 4;
  int      errors  = 0;

  // Formatting the testing port
  url.Format("http://localhost:%d/MarlinTest/TestInterface/Extra/",TESTING_HTTP_PORT);

  // Defined in TestServer.cpp
  extern CString logfileName;

  // Defining the server and the handler
  TestContract* server = new TestContract(name,webroot,url,PrefixType::URLPRE_Strong,p_contract,threads);
  // If you want a different SOAP/GET handler, use
  // server.GetHTTPSite()->SetHandler(....);

  // See if we are running stand alone, or in the context of another server
  server->SetHTTPServer(p_server);
  server->SetLogAnalysis(p_server->GetLogfile());

  // Checking of the field values of incoming services
  server->SetCheckFieldValues(true);

  // TESTING THE SOAP->JSON->SOAP roundtripping
  server->SetGenerateWsdl(false);
  server->SetJsonSoapTranslation(true);

  // Allways adding the operations
  AddOperations(*server,p_contract);

  // Do the logging
  server->SetLogLevel(p_loglevel);

  // Register for de-allocation
  p_server->RegisterService(server);

  qprintf("\n");
  // Try running the service
  if(server->RunService())
  {
    qprintf("WebServiceServer [%s] is now running OK\n",(LPCTSTR)name);
    qprintf("Running contract      : %s\n",(LPCTSTR)p_contract);
    qprintf("JSON service on URL   : %s\n",(LPCTSTR)url);
    qprintf("\n");
  }
  else
  {
    ++errors;
    xerror();
    qprintf("ERROR Starting WebServiceServer for: %s\n",(LPCTSTR)p_contract);
    qprintf("ERROR Reported by the server: %s\n",(LPCTSTR)server->GetErrorMessage());
  }
  return errors;
}

int 
TestWebServiceServer(HTTPServer* p_server,CString p_contract,int p_loglevel)
{
  int result = 0;

  CString  url;
  CString  name("MarlinWeb");
  CString  webroot("C:\\WWW\\Marlin"); // If stand alone marlin server
  unsigned threads = 4;

  // Formatting the testing port
  url.Format("http://localhost:%d/MarlinTest/TestInterface/",TESTING_HTTP_PORT);

  // Defined in TestServer.cpp
  extern CString logfileName;

  // Defining the server and the handler
  TestContract* server = new TestContract(name,webroot,url, PrefixType::URLPRE_Strong,p_contract,threads);
  // If you want a different SOAP/GET handler, use
  // server.GetHTTPSite()->SetHandler(....);

  // See if we are running stand alone, or in the context of another server
  server->SetHTTPServer(p_server);
  if(p_server)
  {
    server->SetLogAnalysis(p_server->GetLogfile());
  }

  // Do the logging
  server->SetLogLevel(p_loglevel);

  // Checking of the field values of incoming services
  server->SetCheckFieldValues(true);

  // See if we must use an externally defined WSDL
  if(test_external_wsdl)
  {
    server->SetExternalWsdl(EXTERN_CONTRACT);
  }
  else 
  {
    // Testing the skipping of the WSDL by commenting out next line
    // server.SetGenerateWsdl(false);

    // Setting operations for the WSDL
    AddOperations(*server,p_contract);
  }

  if(p_server)
  {
    p_server->RegisterService(server);
  }

  // Try running the service
  if(server->RunService())
  {
    qprintf("WebServiceServer [%s] is now running OK\n",(LPCTSTR)name);
    qprintf("Running contract      : %s\n",       (LPCTSTR)p_contract);
    qprintf("For WSDL download use : %s%s.wsdl\n",(LPCTSTR)url,(LPCTSTR)name);
    qprintf("For interface page use: %s%s%s\n",   (LPCTSTR)url,(LPCTSTR)name,(LPCTSTR)server->GetServicePostfix());
    qprintf("\n");
  }
  else
  {
    xerror();
    qprintf("ERROR Starting WebServiceServer for: %s\n",(LPCTSTR)p_contract);
    qprintf("ERROR Reported by the server: %s\n",(LPCTSTR)server->GetErrorMessage());
  }
  return result;
}

int
AfterTestContract()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("WSDL contract calls (SOAP/JSON)                : %s\n",totalChecks > 0 ? "ERROR" : "OK");
  return totalChecks > 0;
}

