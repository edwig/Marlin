/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestNamespaces.cpp
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
#include "TestClient.h"
#include "Namespace.h"
#include "SOAPMessage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int Testsplit(CString p_soapAction,CString p_expect)
{
  int result = 0;
  CString namesp,action;
  xprintf("SOAPAction : %s\t",p_soapAction.GetString());
  if(SplitNamespaceAndAction(p_soapAction,namesp,action))
  {
    if(namesp == p_expect && action == "command")
    {
      // --- "---------------------------------------------- - ------
      printf("Splitting of namespace and SOAP Action         : OK\n");
    }
    else
    {
      // --- "---------------------------------------------- - ------
      printf("Spliting of namespace and SOAP Action          : ERROR: %s\n",p_soapAction.GetString());
      ++result;
    }
  }
  else
  {
    if(action != "command")
    {
      // --- "---------------------------------------------- - ------
      printf("Splitting of namespace and SOAP action         : ERROR: %s\n", p_soapAction.GetString());
      ++result;
    }
    else 
    {
      // --- "---------------------------------------------- - ------
      printf("Splitting of namespace and SOAP Action         : OK\n");
    }
  }
  return result;
}

int NamespaceInSOAP11()
{
  int errors = 0;

  double num(77.88);
  CString namesp("http://www.myname.org/interface/");
  CString action("FunctionCall");
  SOAPMessage msg(namesp,action,SoapVersion::SOAP_11);
  msg.SetParameter("ParameterOne", "One");
  msg.SetParameter("ParameterTwo", "Two");
  msg.SetParameter("Number",num);

  CString message = msg.GetSoapMessage();
  CString expected = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                     "<s:Envelope xmlns:i=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">\n"
                     "  <s:Header />\n"
                     "  <s:Body>\n"
                     "    <FunctionCall xmlns=\"http://www.myname.org/interface/\">\n"
                     "      <ParameterOne>One</ParameterOne>\n"
                     "      <ParameterTwo>Two</ParameterTwo>\n"
                     "      <Number>77.88</Number>\n"
                     "    </FunctionCall>\n"
                     "  </s:Body>\n"
                     "</s:Envelope>\n";

  errors = expected.Compare(message) != 0;

  // --- "---------------------------------------------- - ------
  printf("Namespaces check in SOAPMessage 1.1            : %s\n", errors ? "ERROR" : "OK");

  return errors;
}

int NamespaceInSOAP12()
{
  int errors = 0;

  double num(77.88);
  CString namesp("http://www.myname.org/interface/");
  CString action("FunctionCall");
  SOAPMessage msg(namesp,action);
  msg.SetParameter("ParameterOne", "One");
  msg.SetParameter("ParameterTwo", "Two");
  msg.SetParameter("Number",num);

  CString message = msg.GetSoapMessage();
  CString expected = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                     "<s:Envelope xmlns:i=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:a=\"http://www.w3.org/2005/08/addressing\" xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">\n"
                     "  <s:Header>\n"
                     "    <a:Action s:mustUnderstand=\"true\">http://www.myname.org/interface/FunctionCall</a:Action>\n"
                     "  </s:Header>\n"
                     "  <s:Body>\n"
                     "    <FunctionCall xmlns=\"http://www.myname.org/interface/\">\n"
                     "      <ParameterOne>One</ParameterOne>\n"
                     "      <ParameterTwo>Two</ParameterTwo>\n"
                     "      <Number>77.88</Number>\n"
                     "    </FunctionCall>\n"
                     "  </s:Body>\n"
                     "</s:Envelope>\n";
  errors = expected.Compare(message) != 0;

  // --- "---------------------------------------------- - ------
  printf("Namespaces check in SOAPMessage 1.2            : %s\n", errors ? "ERROR" : "OK");
  return errors;
}

int NamespaceInSOAP_Parsed()
{
  int errors = 0;

  CString internal = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                     "<s:Envelope xmlns:i=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:a=\"http://www.w3.org/2005/08/addressing\" xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">\n"
                     "  <s:Header>\n"
                     "    <a:Action s:mustUnderstand=\"true\">http://www.myname.org/interface/FunctionCall</a:Action>\n"
                     "  </s:Header>\n"
                     "  <s:Body>\n"
                     "    <FunctionCall xmlns=\"http://www.myname.org/interface/\">\n"
                     "      <ParameterOne>One</ParameterOne>\n"
                     "      <ParameterTwo>Two</ParameterTwo>\n"
                     "      <Number>77.88</Number>\n"
                     "    </FunctionCall>\n"
                     "  </s:Body>\n"
                     "</s:Envelope>\n";
  SOAPMessage msg(internal,false);
  CString action = msg.GetSoapAction();
  CString namesp = msg.GetNamespace();

  errors += action.Compare("FunctionCall") != 0;
  errors += namesp.Compare("http://www.myname.org/interface/") != 0;

  // --- "---------------------------------------------- - ------
  printf("Namespaces check in <XML> parsed SOAPMessage   : %s\n", errors ? "ERROR" : "OK");
  return errors;
}

int NamespaceInSOAP_Reparsed()
{
  int errors = 0;

  double num(77.88);
  CString namesp("http://www.myname.org/interface/");
  CString action("FunctionCall");
  SOAPMessage msg(namesp, action);
  msg.SetParameter("ParameterOne", "One");
  msg.SetParameter("ParameterTwo", "Two");
  msg.SetParameter("Number", num);
  msg.CompleteTheMessage();

  CString newmsg = "<MyFunction>\n"
                   "   <One>1</One>\n"
                   "   <Two>2</Two>\n"
                   "</MyFunction>";
  msg.ParseAsBody(newmsg);
  msg.SetNamespace("http://www.other.au/api/");
  CString newaction("MyTestFunction");
  msg.SetSoapAction(newaction);
  CString message = msg.GetSoapMessage();

  CString expected = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                     "<s:Envelope xmlns:i=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:a=\"http://www.w3.org/2005/08/addressing\" xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">\n"
                     "  <s:Header>\n"
                     "    <a:Action s:mustUnderstand=\"true\">http://www.other.au/api/MyTestFunction</a:Action>\n"
                     "  </s:Header>\n"
                     "  <s:Body>\n"
                     "    <MyFunction xmlns=\"http://www.other.au/api/\">\n"
                     "      <One>1</One>\n"
                     "      <Two>2</Two>\n"
                     "    </MyFunction>\n"
                     "  </s:Body>\n"
                     "</s:Envelope>\n";
  errors = expected.Compare(message) != 0;

  // --- "---------------------------------------------- - ------
  printf("Namespaces check in SOAPMessage Reparsed body  : %s\n", errors ? "ERROR" : "OK");
  return errors;
}

int TestNamespaces(void)
{
  int errors = 0;
  CString left  ("http://Name.Test.lower\\something");
  CString right("https://NAME.test.LOWER/SomeThing/");

  xprintf("TESTING NAMESPACE FUNCTIONS:\n");
  xprintf("========================================================\n");
  xprintf("Left  namesp:  %s\n", left.GetString());
  xprintf("Right namesp: %s\n", right.GetString());
  xprintf("-------------------------------------------------------\n");
  int result = CompareNamespaces(left,right);
  xprintf("%s = %s\n",left.GetString(),result == 0 ? "OK" : "NAMESPACE ERROR");
  errors += result;

  xprintf("TESTING NAMESPACE + SOAP ACTION:\n");
  xprintf("========================================================\n");
  errors += Testsplit("\"http://server/uri/command/\"","http://server/uri");
  errors += Testsplit("http://server/uri/some#command","http://server/uri/some");
  errors += Testsplit("command","");

  // Namespaces in SOAPMessage
  errors += NamespaceInSOAP11();
  errors += NamespaceInSOAP12();
  errors += NamespaceInSOAP_Parsed();
  errors += NamespaceInSOAP_Reparsed();

  return errors;
}
