/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestReader.cpp
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
#include "SOAPMessage.h"
#include "XMLMessage.h"
#include "JSONMessage.h"
#include "TestClient.h"
#include <conio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void
PrintSoapFault(SOAPMessage& p_msg)
{
  printf("---- Error ---------------------------------\n");
  printf(p_msg.GetFault());
  printf("--------------------------------------------\n");
}

int  SimplePOS()
{
  bool result = false;
  XString soap("<FirstCommand>\n"
               "   <Param1>first</Param1>\n"
               "   <Param2 />\n"
               "   <Param3>third</Param3>\n"
               "</FirstCommand>\n");

  xprintf("\nTESTING\n=======\n");
  xprintf("Soap message:\n%s\n", soap.GetString());

  SOAPMessage msg(soap);
  if (msg.GetErrorState())
  {
    PrintSoapFault(msg);
  }
  else
  {
    XString action = msg.GetSoapAction();
    XString param1 = msg.GetParameter("Param1");
    XString param2 = msg.GetParameter("Param2");
    XString param3 = msg.GetParameter("Param3");

    xprintf("Soap action: %s\n", action.GetString());
    xprintf("Parameter 1: %s\n", param1.GetString());
    xprintf("Parameter 2: %s\n", param2.GetString());
    xprintf("Parameter 3: %s\n", param3.GetString());
    if(action == "FirstCommand" && 
       param1 == "first" &&
       param2.IsEmpty()  &&
       param3 == "third" &&
       msg.GetSoapVersion() == SoapVersion::SOAP_POS)
    {
      result = true;
    }

    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    printf("SOAP : Plain old soap mode                     : %s\n",result ? "OK" : "ERROR");
  }
  return result ? 0 : 1;
}

int Soap11_noADR()
{
  bool result = false;
  XString soap("<Envelope>\n"
               "  <Header>\n"
               "    <MessageID>12345</MessageID>\n"
               "  </Header>\n"
               "<Body>\n"
               "  <FirstCommand>\n"
               "    <Param1>first</Param1>\n"
               "    <Param2 />\n"
               "    <Param3>third</Param3>\n"
               "  </FirstCommand>\n"
               "</Body>\n"
               "</Envelope>\n");

  xprintf("\nTESTING\n=======\n");
  xprintf("Soap message:\n%s\n", soap.GetString());

  SOAPMessage msg(soap);
  if (msg.GetErrorState())
  {
    PrintSoapFault(msg);
  }
  else
  {
    XString action = msg.GetSoapAction();
    XString param1 = msg.GetParameter("Param1");
    XString param2 = msg.GetParameter("Param2");
    XString param3 = msg.GetParameter("Param3");

    xprintf("Soap action: %s\n", action.GetString());
    xprintf("Parameter 1: %s\n", param1.GetString());
    xprintf("Parameter 2: %s\n", param2.GetString());
    xprintf("Parameter 3: %s\n", param3.GetString());
    if(action == "FirstCommand" && 
       param1 == "first" &&
       param2.IsEmpty()  &&
       param3 == "third" &&
       msg.GetSoapVersion() == SoapVersion::SOAP_11)
    {
      result = true;
    }

    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    printf("SOAP Version 1.1 Without WS-Addressing         : %s\n",result ? "OK" : "ERROR");
  }
  return result ? 0 : 1;
}

int Soap12_ADR()
{
  bool result = false;
  XString soap("<Envelope>\n"
               "  <Header>\n"
               "    <Action mustUnderstand=\"1\">http://services.marlin.org/Test/SmallObject</Action>\n"
               "  </Header>\n"
               "<Body>\n"
               "  <SmallObject xmlns=\"http://services.marlin.org/Test\">\n"
               "    <Param1>first</Param1>\n"
               "    <Param2 />\n"
               "    <Param3>third</Param3>\n"
               "  </SmallObject>\n"
               "</Body>\n"
               "</Envelope>\n");

  xprintf("\nTESTING\n=======\n");
  xprintf("Soap message:\n%s\n", soap.GetString());

  SOAPMessage msg(soap);
  if (msg.GetErrorState())
  {
    PrintSoapFault(msg);
  }
  else
  {
    XString action = msg.GetSoapAction();
    XString param1 = msg.GetParameter("Param1");
    XString param2 = msg.GetParameter("Param2");
    XString param3 = msg.GetParameter("Param3");

    xprintf("Soap action: %s\n", action.GetString());
    xprintf("Parameter 1: %s\n", param1.GetString());
    xprintf("Parameter 2: %s\n", param2.GetString());
    xprintf("Parameter 3: %s\n", param3.GetString());
    if(action == "SmallObject" && 
       param1 == "first" &&
       param2.IsEmpty()  &&
       param3 == "third" &&
       msg.GetSoapVersion() == SoapVersion::SOAP_12)
    {
      result = true;
    }

    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    printf("SOAP Version 1.2 With WS-Addressing            : %s\n",result ? "OK" : "ERROR");
  }
  return result ? 0 : 1;
}

int Soap12_ADR_MDS()
{
  bool result = false;
  XString soap("<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:a=\"http://www.w3.org/2005/08/addressing\">\n"
               "  <s:Header>\n"
               "    <a:Action s:mustUnderstand=\"1\">http://schemas.marlin.org/general/services/2015/IContainer/CreateContainerResponse</a:Action>\n"
               "    <a:RelatesTo>urn:uuid:b7681987-20f4-4fb5-9a74-383142da799c</a:RelatesTo>\n"
               "  </s:Header>\n"
               "  <s:Body>\n"
               "    <ResultReference xmlns=\"http://schemas.marlin.org/general/services/2015\">\n"
               "      <EventId>0</EventId>\n"
               "      <Reference   xmlns:b=\"http://schemas.marlin.org/general/elementen/2015\"\n"
               "                   xmlns:i=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
               "        <b:Repository>DocContainer</b:Repository>\n"
               "        <b:ApplicationReference>K2B:30008</b:ApplicationReference>\n"
               "        <b:DocumentType i:nil=\"true\"/>\n"
               "        <b:CaseTypeCode>Key2Brief</b:CaseTypeCode>\n"
               "        <b:DcIdentifier/>\n"
               "        <b:FolderIdentifier>FLD-2015-001470</b:FolderIdentifier>\n"
               "        <b:DocumentId i:nil=\"true\"/>\n"
               "        <b:DossierId>2015-001470</b:DossierId>\n"
               "        <b:FileName i:nil=\"true\"/>\n"
               "        <b:RecordId i:nil=\"true\"/>\n"
               "        <b:CaseId/>\n"
               "      </Reference>\n"
               "      <ResultCode>Ok</ResultCode>\n"
               "      <ResultValue>Folder is created the DocContainer</ResultValue>\n"
               "    </ResultReference>\n"
               "  </s:Body>\n"
               "</s:Envelope>");

  xprintf("\nTESTING\n=======\n");
  xprintf("Soap message:\n%s\n", soap.GetString());

  SOAPMessage msg(soap);
  if(msg.GetErrorState())
  {
    PrintSoapFault(msg);
  }
  else
  {
    XString action  = msg.GetSoapAction();
    XString resCode = msg.GetParameter("ResultCode");
    XString folder;

    xprintf("Soap action: %s\n", action.GetString());
    xprintf("Result code: %s\n", resCode.GetString());

    XMLElement* refer = msg.FindElement(NULL,"Reference",true);
    if(refer)
    {
      XMLElement* ident = msg.FindElement(refer,"FolderIdentifier");
      if(ident)
      {
        folder = ident->GetValue();
        xprintf("Folder: %s\n",folder.GetString());
      }
    }
    if(action  == "IContainer/CreateContainerResponse" &&
       resCode == "Ok" &&
       folder  == "FLD-2015-001470")
    {
      result = true;
    }

    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    printf("SOAP Version 1.2 with full WS-Addressing       : %s\n",result ? "OK" : "ERROR");
  }
  return result ? 0 : 1;
};

int Soap12_NS_request()
{
  bool result = false;
  XString soap("<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:a=\"http://www.w3.org/2005/08/addressing\">\n"
               "  <s:Header>\n"
               "    <a:Action s:mustUnderstand=\"1\">http://schemas.marlin.org/general/services/2012/IContent/GetContent</a:Action>\n"
               "    <h:Command xmlns:h=\"http://schemas.marlin.org/general/services/2012\"\n"
               "                 xmlns=\"http://schemas.marlin.org/general/services/2012\">Find</h:Command>\n"
               "    <h:Identity xsi:type=\"q1:LogOnData\" xmlns:h=\"http://schemas.marlin.org/general/services/2012\"\n"
               "                xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
               "                xmlns=\"http://schemas.marlin.org/general/services/2012\"\n"
               "                xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"\n"
               "                xmlns:q1=\"http://schemas.marlin.org/general/security/2012\">\n"
               "      <q1:Context xsi:nil=\"true\"/>\n"
               "      <q1:Domain>its</q1:Domain>\n"
               "      <q1:LogOnType>Anonymous</q1:LogOnType>\n"
               "      <q1:Password/>\n"
               "      <q1:UserName>ehuisman</q1:UserName>\n"
               "    </h:Identity>\n"
               "    <a:MessageID>urn:uuid:82aca07f-88a8-4e05-be69-2c897e6b93bb</a:MessageID>\n"
               "    <a:ReplyTo>\n"
               "      <a:Address>http://www.w3.org/2005/08/addressing/anonymous</a:Address>\n"
               "    </a:ReplyTo>\n"
               "    <a:To s:mustUnderstand=\"1\">http://localhost/general/DocumentManager.svc</a:To>\n"
               "  </s:Header>\n"
               "  <s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\n"
               "    <ContentCommandRequest xmlns=\"http://schemas.marlin.org/general/services/2012\">\n"
               "      <Reference>\n"
               "        <Repository xsi:nil=\"true\" xmlns=\"http://schemas.marlin.org/general/elementen/2012\"/>\n"
               "        <ApplicationCode xsi:nil=\"true\" xmlns=\"http://schemas.marlin.org/general/elementen/2012\"/>\n"
               "        <ApplicationReference xsi:nil=\"true\" xmlns=\"http://schemas.marlin.org/general/elementen/2012\"/>\n"
               "        <DocumentType xsi:nil=\"true\" xmlns=\"http://schemas.marlin.org/general/elementen/2012\"/>\n"
               "        <DcIdentifier xsi:nil=\"true\" xmlns=\"http://schemas.marlin.org/general/elementen/2012\"/>\n"
               "        <FolderIdentifier xsi:nil=\"true\" xmlns=\"http://schemas.marlin.org/general/elementen/2012\"/>\n"
               "        <DocumentId xmlns=\"http://schemas.marlin.org/general/elementen/2012\">12005679</DocumentId>\n"
               "        <DossierId xsi:nil=\"true\" xmlns=\"http://schemas.marlin.org/general/elementen/2012\"/>\n"
               "        <FileName xsi:nil=\"true\" xmlns=\"http://schemas.marlin.org/general/elementen/2012\"/>\n"
               "        <RecordId xsi:nil=\"true\" xmlns=\"http://schemas.marlin.org/general/elementen/2012\"/>\n"
               "      </Reference>\n"
               "  </ContentCommandRequest>\n"
               "  </s:Body>\n"
               "</s:Envelope>\n");

  xprintf("\nTESTING\n=======\n");
  xprintf("Soap message:\n%s\n", soap.GetString());

  SOAPMessage msg(soap);
  if (msg.GetErrorState())
  {
    PrintSoapFault(msg);
  }
  else
  {
    XString action = msg.GetSoapAction();
    xprintf("Soap action: %s\n",action.GetString());
    result = (action == "IContent/GetContent");
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    printf("SOAP Version 1.2 with Header SOAP action       : %s\n",result ? "OK" : "ERROR");
  }
  return result ? 0 : 1;
}

int Soap12_NS_response()
{
  bool result = false;
  XString soap("<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:a=\"http://www.w3.org/2005/08/addressing\">\n"
               "  <s:Header>\n"
               "    <a:Action s:mustUnderstand=\"1\">http://schemas.marlin.org/general/services/2012/IContent/GetContentResponse</a:Action>\n"
               "    <a:RelatesTo>urn:uuid:82aca07f-88a8-4e05-be69-2c897e6b93bb</a:RelatesTo>\n"
               "  </s:Header>\n"
               "  <s:Body>\n"
               "    <ContentResponse xmlns=\"http://schemas.marlin.org/general/services/2012\">\n"
               "      <Content xmlns:b=\"http://schemas.marlin.org/general/elementen/2012\" xmlns:i=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
               "        <b:DocumentId i:nil=\"true\"/>\n"
               "        <b:RecordId i:nil=\"true\"/>\n"
               "        <b:dcIdentifier i:nil=\"true\"/>\n"
               "      </Content>\n"
               "      <ContentReference xmlns:b=\"http://schemas.marlin.org/general/elementen/2012\" xmlns:i=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
               "        <b:DocumentType>Nota</b:DocumentType>\n"
               "        <b:CasetypeCode i:nil=\"true\"/>\n"
               "        <b:DcIdentifier>2015-001442/HalloWorld.txt</b:DcIdentifier>\n"
               "      </ContentReference>\n"
               "      <Result>Ok</Result>\n"
               "      <ResultMessage>Found : General.Elements.DcmiReference</ResultMessage>\n"
               "    </ContentResponse>\n"
               "  </s:Body>\n"
               "</s:Envelope>\n");

  xprintf("\nTESTING\n=======\n");
  xprintf("Soap message:\n%s\n",soap.GetString());

  SOAPMessage msg;
  msg.ParseMessage(soap);
  if(msg.GetInternalError() != XmlError::XE_NoError)
  {
    printf("Error [%d] %s\n",msg.GetInternalError(),msg.GetInternalErrorString().GetString());
  }
  else
  {
    XString action = msg.GetSoapAction();
    xprintf("Soap action: %s\n",action.GetString());
    xprintf("OK\n%s\n",msg.Print().GetString());

    result = (action == "IContent/GetContentResponse");
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    printf("SOAP WS-Addressing namespace response          : %s\n",result ? "OK" : "ERROR");
  }
  return result ? 0 : 1;
}

int TestXMLMessage1()
{
  XMLMessage msg;
  bool result = false;
  // Message inclusief UTF-8 encoding (2e node <Address>)
  XString text("<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>\n"
               "<RootNode xmlns=\"one\">\n"
               "  <a:Node1 atr1=\"12\" xmlns:a=\"http://one.two.com/\">testing</a:Node1>\n"
               "  <Node2>two</Node2>\n"
               "  <Node3 i:nil=\"true\"/>\n"
               "  <a:Unit>\n"
               "     <Number>12</Number>\n"
               "     <Address>abc12</Address>\n"
               "  </a:Unit>\n"
               "  <a:Unit>\n"
               "     <Number>14</Number>\n"
               "     <Address>Sôn bôn de magnà el véder, el me fa minga mal. Mister Gênépèllëtje</Address>\n"
               "  </a:Unit>\n"
               "  <Extra><![CDATA[This is an extra piece of tekst]]></Extra>\n"
               "</RootNode>\n");

  xprintf("\nTESTING\n=======\n");
  xprintf("XML Message:\n%s\n",text.GetString());

  msg.ParseMessage(text);
  
  if(msg.GetInternalError() != XmlError::XE_NoError)
  {
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    printf("XML Message with full UTF-8 nodes and CDATA    : ERROR\n");
    xprintf("XML-Message INTERNAL ERROR [%d] %s\n",msg.GetInternalError(),msg.GetInternalErrorString().GetString());
  }
  else
  {
    result = true;
    xprintf(msg.Print());
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    printf("XML Message with full UTF-8 nodes and CDATA    : OK\n");
  }
  return result ? 0 : 1;
}

int TestXMLMessage2()
{
  XMLMessage msg;
  bool result = false;
  // Incomplete message
  XString text("<?xml  version=\"1.0\" encoding=\"utf-8\" ?>\n"
               "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope\">\n"
               "<soap:Body>\n"
               "  <TestSoapException_FirstOperation>\n"
               "    <FirstOperationMessageIn>\n"
               "      <Field01>Getal</Field01>\n"
               "      <Field07>Getal</Field07>\n"
               "    /FirstOperationMessageIn>\n"
               "  </TestSoapException_FirstOperation>\n"
               "</soap:Body>\n"
               "</soap:Envelope>\n");

  xprintf("\nTESTING\n=======\n");
  xprintf("XML Message:\n%s\n",text.GetString());

  msg.ParseMessage(text);

  if(msg.GetInternalError() != XmlError::XE_NoError)
  {
    if(msg.GetInternalError() == XmlError::XE_NotAnXMLMessage &&
       msg.GetInternalErrorString() == "ERROR parsing XML:  Element [FirstOperationMessageIn] : /FirstOperationMessageIn>\n"
                                       "  </TestSoapException_FirstOperation>\n"
                                       "</soap:Body>\n"
                                       "</soap:Envelope>\n")
    {
      result = true;
      // SUMMARY OF THE TEST
      // --- "---------------------------------------------- - ------
      printf("XML message incompleteness error reporting     : OK\n");
    }
    else
    {
      // SUMMARY OF THE TEST
      // --- "---------------------------------------------- - ------
      printf("XML message incompleteness NOT REPORTED        : ERROR\n");
      xprintf(msg.Print());
    }
  }
  else
  {
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    printf("XML message incompleteness NOT recognized      : ERROR\n");
    xprintf(msg.Print());
  }
  return result ? 0 : 1;
}

int
TestXMLMessage3()
{
  SOAPMessage msg;
  bool result = false;
  // Incomplete message
  XString text("<?xml  version=\"1.0\" encoding=\"utf-8\" ?>\n"
               "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\"\n"
               "       soap:encodingStyle=\"http://www.w3.org/2003/12/soap-encoding\">\n"
               "<soap:Body>\n"
               "  <CreateAddress>\n"
               "    <CreateAddressRequest>\n"
               "      <Address>\n"
               "        <Streetname>a</Streetname>\n"
               "        <Residential>b</Residential>\n"
               "        <County>c</County>\n"
               "        <Postcode>o&#xD;k</Postcode>\n"
               "        <Housenumber>6</Housenumber>\n"
               "        <ExtraLetter>e</ExtraLetter>\n"
               "        <Extra>a&#xa;b</Extra>\n"
               "        <Country>h</Country>\n"
               "      </Address>\n"
               "    </CreateAddressRequest>\n"
               "  </CreateAddress>\n"
               "</soap:Body>\n"
               "</soap:Envelope>\n");

  xprintf("\nTESTING\n=======\n");
  xprintf("XML Message:\n%s\n",text.GetString());

  msg.ParseMessage(text);

  if(msg.GetInternalError() != XmlError::XE_NoError)
  {
    printf("XML Message internal error [%d] %s\n"
          ,msg.GetInternalError()
          ,msg.GetInternalErrorString().GetString());
  }
  else
  {
    XMLElement* aand = msg.FindElement("Country");
    XMLElement* post = msg.FindElement("Postcode");
    XMLElement* toev = msg.FindElement("Extra");
    if(aand && aand->GetValue() == "h")
    {
      if(post && post->GetValue() == "o\rk")
      {
        if(toev && toev->GetValue() == "a\nb")
        {
          result = true;
        }
      }
    }
    xprintf(msg.Print());
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("XML hexadecimal numbers parsing                : %s\n",result ? "OK" : "ERROR");

  return result ? 0 : 1;
}

int 
TestXMLMessage4()
{
  bool result = false;
  XString namesp = DEFAULT_NAMESPACE;
  XString action = "MyAction";

  SOAPMessage msg(namesp,action);
  msg.SetParameter("PriceTag","€ 42,00");
  msg.SetEncoding(StringEncoding::ENC_UTF8);

  XString test1 = msg.GetSoapMessage();

  SOAPMessage msg2(test1,true);
  XString priceTag = msg2.GetParameter("PriceTag");

  // Test for positive result
  if(priceTag == "€ 42,00")
  {
    result = true;
  }

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("XML Supports full UTF-8 in 4 codepages         : %s\n",result ? "OK" : "ERROR");

  return result ? 0 : 1;
}

int
TestXMLMessage5()
{
  bool result = false;
  XString namesp = DEFAULT_NAMESPACE;
  XString action = "MyAction";

  SOAPMessage msg(namesp,action);
  msg.SetParameter("PriceTag","€ 42,00");
  msg.SetEncoding(StringEncoding::ENC_ISO88591);

  XString test1 = msg.GetSoapMessage();

  SOAPMessage msg2(test1,true);
  XString priceTag = msg2.GetParameter("PriceTag");

  // Test for positive result
  if(priceTag == "€ 42,00")
  {
    result = true;
  }

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("XML Supports full ISO8859-1                    : %s\n",result ? "OK" : "ERROR");

  return result ? 0 : 1;
}

int
TestXMLMessage6()
{
  bool result = false;
  XString namesp = DEFAULT_NAMESPACE;
  XString action = "MyAction";

  SOAPMessage msg(namesp,action);
  msg.SetParameter("PriceTag","€ 42,00");
  msg.SetEncoding(StringEncoding::ENC_Plain);

  XString test1 = msg.GetSoapMessage();

  SOAPMessage msg2(test1,true);
  XString priceTag = msg2.GetParameter("PriceTag");

  // Test for positive result
  if(priceTag == "€ 42,00")
  {
    result = true;
  }

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("XML Supports windows-1252                      : %s\n",result ? "OK" : "ERROR");

  return result ? 0 : 1;
}

int
TestJSONMessage1()
{
  bool result = false;
  XString text = "{\"PriceTag\":\"€ 42,00\"}";
  JSONMessage msg(text);
  XString test = msg.GetJsonMessage();

  // msg.SetEncoding(JsonEncoding::JENC_UTF8);
  // test = msg.GetJsonMessage(JsonEncoding::JENC_UTF8);

  if(test == text)
  {
    result = true;
  }

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("JSON Supports MBCS strings                     : %s\n",result ? "OK" : "ERROR");

  return result ? 0 : 1;
}

int
TestJSONMessage2()
{
  int result = 2;

  XString text = "{\"PriceTag\":\"€ 42,00\"}";
  JSONMessage msg(text);
  XString test = msg.GetJsonMessage(StringEncoding::ENC_UTF8);
  XString mustbe = "{\"PriceTag\":\"\xe2\x82\xac 42,00\"}";

  if(test == mustbe)
  {
    --result;
  }

  JSONMessage msg2(mustbe);
  XString tt2 = msg2.GetJsonMessage();

  if(tt2 == text)
  {
    --result;
  }

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("JSON Supports full UTF-8                       : %s\n",result ? "ERROR" : "OK");

  return result;
}

int
TestJSONMessage3()
{
  int result = 2;

  XString text = "{\"PriceTag\":\"€ 42,00\"}";
  JSONMessage msg(text);
  XString test = msg.GetJsonMessage(StringEncoding::ENC_ISO88591);
  XString mustbe = "{\"PriceTag\":\"\x80 42,00\"}";

  if(test == mustbe)
  {
    --result;
  }

  JSONMessage msg2(mustbe,false,StringEncoding::ENC_Plain);
  XString tt2 = msg2.GetJsonMessage();

  if(tt2 == text)
  {
    --result;
  }

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("JSON Supports ISO-8859-1                       : %s\n",result ? "ERROR" : "OK");

  return result;
}

int
TestReader(void)
{
  int errors = 0;

  xprintf("TESTING XML/SOAP PARSER EN MESSAGE STRUCTURES OF WINHTTP COMPONENT\n");
  xprintf("==================================================================\n");
  try
  {
    errors += TestXMLMessage1();
    errors += TestXMLMessage2();
    errors += TestXMLMessage3();
    errors += TestXMLMessage4();
    errors += TestXMLMessage5();
    errors += TestXMLMessage6();
    errors += TestJSONMessage1();
    errors += TestJSONMessage2();
    errors += TestJSONMessage3();
    errors += SimplePOS();
    errors += Soap11_noADR();
    errors += Soap12_ADR();
    errors += Soap12_NS_request();
    errors += Soap12_NS_response();
    errors += Soap12_ADR_MDS();
  }
  catch(StdException& er)
  {
    ++errors;
    printf("Error while reading the SOAP message: %s\n",er.GetErrorMessage().GetString());
  }
  return errors;
}

