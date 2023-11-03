/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestContract.cpp
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
#include "stdafx.h"
#include "TestMarlinServer.h"
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
const TCHAR* MARLIN_WSDL_CONTRACT = _T("..\\ExtraParts\\MarlinWeb.wsdl");
const TCHAR* EASY_MATCH_CONTRACT  = _T("..\\ExtraParts\\EasyMatch.wsdl");
const TCHAR* K2W_CONTRACT         = _T("..\\ExtraParts\\K2WServices.wsdl");

XMLRestriction g_restrict(_T("Language"));

#define EXTERN_CONTRACT EASY_MATCH_CONTRACT

// Ordinals for the services
#define CONTRACT_MF   1 // First
#define CONTRACT_MS   2 // Second
#define CONTRACT_MT   3 // Third
#define CONTRACT_M4   4 // Fourth
#define CONTRACT_MV   5 // Fifth


// Mapping corresponding to the AddOperation of the WSDL
// Binding ordinals to the service declarations for the On..... members
WEBSERVICE_MAP_BEGIN(TestMarlinServer)
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
TestMarlinServer::OnMarlinFirst(int p_code,SOAPMessage* p_message)
{
  ASSERT(p_code == CONTRACT_MF);
  UNREFERENCED_PARAMETER(p_code);
  
  m_language = p_message->GetParameter(_T("Language"));
  // --- "---------------------------------------------- - ------
  qprintf(_T("WSDL Contract: First: Setting base language    : OK\n"));

  XString resp(_T("ResponseFirst"));
  p_message->SetSoapAction(resp);
  p_message->Reset(ResponseType::RESP_ACTION_NAME,m_targetNamespace);
  p_message->SetParameter(_T("Accepted"),m_language == _T("Dutch"));

  --totalChecks;
}

void
TestMarlinServer::OnMarlinSecond(int p_code,SOAPMessage* p_message)
{
  ASSERT(p_code == CONTRACT_MS);
  UNREFERENCED_PARAMETER(p_code);

  m_translation = p_message->GetParameter(_T("Translation"));
  // --- "---------------------------------------------- - ------
  qprintf(_T("WSDL Contract: Second: translation language    : OK\n"));

  XString resp(_T("ResponseSecond"));
  p_message->SetSoapAction(resp);
  p_message->Reset(ResponseType::RESP_ACTION_NAME,m_targetNamespace);
  bool cando = false;
  if(m_translation == _T("English")  ||
     m_translation == _T("Fran�ais") ||
     m_translation == _T("Deutsch")  ||
     m_translation == _T("Espagnol"))
  {
    cando = true;
    --totalChecks;
  }
  else
  {
    xerror();
    // --- "---------------------------------------------- - ------
    qprintf(_T("WSDL Contract: Second: translation language    : ERROR\n"));
  }
  p_message->SetParameter(_T("CanDo"),cando);
}

void
TestMarlinServer::OnMarlinThird(int p_code,SOAPMessage* p_message)
{
  ASSERT(p_code == CONTRACT_MT);
  UNREFERENCED_PARAMETER(p_code);

  XString word   = p_message->GetParameter(_T("WordToTranslate"));
  XString result = Translation(m_language,m_translation,word);
  // --- "---------------------------------------------- - ------
  qprintf(_T("WSDL Contract: Translated givven               : OK\n"));

  XString resp(_T("ResponseThird"));
  p_message->SetSoapAction(resp);
  p_message->Reset(ResponseType::RESP_ACTION_NAME,m_targetNamespace);
  // Optional parameter, need not be set!
  if(!result.IsEmpty())
  {
    p_message->SetParameter(_T("TranslatedWord"),result);
  }
  // Called 3 times
  --totalChecks;
}

void
TestMarlinServer::OnMarlinFourth(int p_code,SOAPMessage* /*p_message*/)
{
  ASSERT(p_code == CONTRACT_M4);
  UNREFERENCED_PARAMETER(p_code);

  // --- "---------------------------------------------- - ------
  qprintf(_T("WSDL Contract: Wrong contract (Fourth)         : ERROR\n"));
  xerror();
}

void
TestMarlinServer::OnMarlinFifth(int p_code,SOAPMessage* p_message)
{
  ASSERT(p_code == CONTRACT_MV);
  UNREFERENCED_PARAMETER(p_code);

  XString pi;
  XMLElement* pielem = p_message->FindElement(_T("PiApprox"));
  if(pielem)
  {
    pi = pielem->GetValue();
  }
  p_message->Reset(ResponseType::RESP_ACTION_NAME,m_targetNamespace);

  if(!pi.IsEmpty())
  {
    if(pi.Find(_T("3.141592653589")) >= 0)
    {
      // --- "---------------------------------------------- - ------
      qprintf(_T("WSDL Contract: Found the value of PI as        : OK\n"));

      p_message->SetParameter(_T("TranslatedWord"),_T("pi"));
    }
    else
    {
      // --- "---------------------------------------------- - ------
      qprintf(_T("WSDL Contract: Wrong value of PI              : ERROR\n"));
      xerror();
    }
  }
  else
  {
    // --- "---------------------------------------------- - ------
    qprintf(_T("WSDL Contract: Wrong contract (Fifth)          : ERROR\n"));
    xerror();
  }
}

//////////////////////////////////////////////////////////////////////////
//
// VERY LIMITED TRANSLATION FUNCTION :-)
//
XString 
TestMarlinServer::Translation(XString p_language,XString p_translation,XString p_word)
{
  if(p_language == _T("Dutch") && p_translation == _T("Fran�ais"))
  {
    if(p_word == _T("altijd"))  return XString(_T("toujours"));
    if(p_word == _T("maandag")) return XString(_T("lundi"));
  }
  return XString(_T(""));
}

//////////////////////////////////////////////////////////////////////////
//
// PREPARING OUR WSDL, This is what will fill the WSDL file
//
//////////////////////////////////////////////////////////////////////////

void
TestMarlinServer::AddOperations(XString p_contract)
{
  // Defining the names of the operations
  XString first (_T("MarlinFirst"));
  XString second(_T("MarlinSecond"));
  XString third (_T("MarlinThird"));
  XString fourth(_T("MarlinFourth"));
  XString fifth (_T("MarlinFifth"));

  // Response with "Response" before name. Good testing WSDL / XSD
  XString respFirst (_T("ResponseFirst"));
  XString respSecond(_T("ResponseSecond"));
  XString respThird (_T("ResponseThird"));
  XString respFourth(_T("ResponseFourth"));
  XString respFifth (_T("ResponseFifth"));

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

  g_restrict.AddEnumeration(_T("English"), _T("The English language"));
  g_restrict.AddEnumeration(_T("Deutsch"), _T("Die Deutsche sprache"));
  g_restrict.AddEnumeration(_T("Espagnol"),_T("El lingua franca"));
  g_restrict.AddEnumeration(_T("Fran�ais"),_T("La plus belle langue"));
  // Defining the parameters for all the operations

  // First: Getting an accepted language
  input1 .AddElement(NULL,_T("Language"),WSDL_Mandatory | XDT_String, _T("string"));
  input1 .AddElement(NULL,_T("Version"), WSDL_Mandatory | XDT_Integer,   _T("int"));
  output1.AddElement(NULL,_T("Accepted"),WSDL_Mandatory | XDT_Boolean,  _T("bool"));
  
  // Second: Getting an accepted translation
  XMLElement* trans = 
  input2 .AddElement(NULL,_T("Translation"),WSDL_Mandatory | XDT_String, _T("string"));
  output2.AddElement(NULL,_T("CanDo"),      WSDL_Mandatory | XDT_Boolean,_T("bool"));
  trans->SetRestriction(&g_restrict);

  // Third Getting the answer
  input3 .AddElement(NULL,_T("WordToTranslate"),WSDL_Mandatory | XDT_String,_T("string"));
  output3.AddElement(NULL,_T("TranslatedWord"), WSDL_Optional  | XDT_String,_T("string"));

  // Fourth. Recursive parameters and answers
  XMLElement* param   = input4.AddElement(NULL, _T("Parameters"),   WSDL_Mandatory|XDT_Complex,_T(""));
  XMLElement* lanFrom = input4.AddElement(param,_T("LanguageFrom"), WSDL_Mandatory|XDT_String, _T("language"));
  XMLElement* lanTo   = input4.AddElement(param,_T("LanguageTo"),   WSDL_Mandatory|XDT_String, _T("language"));
  XMLElement* datatp  = input4.AddElement(param,_T("DataTypes"),    WSDL_Mandatory|XDT_Complex,_T(""));
  input4.AddElement(datatp,_T("String"),  WSDL_Optional|XDT_String,_T("string"));
  input4.AddElement(datatp,_T("Diacrits"),WSDL_Optional|XDT_String,_T("diacritics"));

  XMLElement* answer  = input4.AddElement(NULL, _T("DoubleWord"),   WSDL_Mandatory|XDT_Complex,_T(""));
  input4.AddElement(answer,_T("WordToTranslate"),WSDL_Mandatory|XDT_String,_T("to_be_translated"));
  input4.AddElement(answer,_T("AlternativeWord"),WSDL_Optional |XDT_String,_T("alternative"));
  lanFrom->SetRestriction(&g_restrict);
  lanTo  ->SetRestriction(&g_restrict);

  output4.AddElement(NULL,_T("TranslatedWord"),WSDL_Optional|XDT_String,_T("string"));
  output4.AddElement(NULL,_T("TranslationAlt"),WSDL_Optional|XDT_String,_T("string"));

  // Fifth. Has another "Parameters" node and "DoubleWord"
  XMLElement* param5   = input5.AddElement(NULL, _T("Parameters"),   WSDL_Mandatory|XDT_Complex,_T(""));
  input5.AddElement(param5,_T("Dialect"),      WSDL_Mandatory|XDT_String, _T("Dialect"));
  input5.AddElement(param5,_T("Region"),       WSDL_Mandatory|XDT_String, _T("Region"));
  input5.AddElement(param5,_T("PiApprox"),     WSDL_Optional |XDT_Float,  _T("Approximation of PI"));
  XMLElement* datatyp  = input5.AddElement(param5,_T("DataTypes"), WSDL_Optional|XDT_Complex,_T(""));
  input5.AddElement(datatyp,_T("MinLength"),  WSDL_Mandatory|XDT_Integer,_T("int"));
  input5.AddElement(datatyp,_T("MaxLength"),  WSDL_Mandatory|XDT_Integer,_T("int"));
  input5.AddElement(datatyp,_T("MaxDecimals"),WSDL_Mandatory|XDT_Integer,_T("int"));
  
  XMLElement* answer5  = input5.AddElement(NULL, _T("DoubleWord"),   WSDL_Mandatory|XDT_Complex,_T("Wording"));
  input5.AddElement(answer5,_T("Word"),        WSDL_Mandatory|XDT_String,_T("to_be_translated"));
  input5.AddElement(answer5,_T("Alternative"), WSDL_Optional |XDT_String,_T("alternative"));

  output5.AddElement(NULL,_T("TranslatedWord"),WSDL_Optional|XDT_String,_T("string"));
  output5.AddElement(NULL,_T("TranslationAlt"),WSDL_Optional|XDT_String,_T("string"));

  // Putting the operations in the WSDL Cache
  AddOperation(CONTRACT_MF,first, &input1,&output1);
  AddOperation(CONTRACT_MS,second,&input2,&output2);
  AddOperation(CONTRACT_MT,third, &input3,&output3);
  AddOperation(CONTRACT_M4,fourth,&input4,&output4);
  AddOperation(CONTRACT_MV,fifth, &input5,&output5);

  // Check the field values in the WSDL
  SetCheckFieldValues(true);
}

//////////////////////////////////////////////////////////////////////////
//
// The final testing function
//
//////////////////////////////////////////////////////////////////////////

int
TestMarlinServer::AfterTestContract()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("WSDL contract calls (SOAP/JSON)                : %s\n"),totalChecks > 0 ? _T("ERROR") : _T("OK"));
  return totalChecks > 0;
}

