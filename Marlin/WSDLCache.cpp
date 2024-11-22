/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WSDLCache.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2024 ir. W.E. Huisman
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
// WSDLCache
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WSDLCache.h"
#include "HTTPClient.h"
#include "CrackURL.h"
#include "Namespace.h"
#include "LogAnalysis.h"
#include "ErrorReport.h"
#include <WinFile.h>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

#ifdef CRASHLOG
#undef CRASHLOG
#endif

// Logging to the logfile
#define DETAILLOG(str)        if(m_logging) m_logging->AnalysisLog(_T(__FUNCTION__),LogType::LOG_INFO,false,str)
#define CRASHLOG(code,text)   if(m_logging)\
                              {\
                                m_logging->AnalysisLog(_T(__FUNCTION__),LogType::LOG_ERROR,false,text); \
                                m_logging->AnalysisLog(_T(__FUNCTION__),LogType::LOG_ERROR,true,_T("Error code: %d"),code); \
                              }


// CTOR for the WSDL cache
WSDLCache::WSDLCache(bool p_server)
          :m_server(p_server)
          // Not seen anywhere else: The service postfix is 'active server pages for C ++'
          ,m_servicePostfix(_T(".acx"))
{
}

WSDLCache::~WSDLCache()
{
  Reset();
}

void
WSDLCache::Reset()
{
  ClearCache();

  m_types.clear();
  m_filename.Empty();
  m_serviceName.Empty();
  m_targetNamespace.Empty();
  m_performSoap10 = true;
  m_performSoap12 = true;

  // Leave m_webroot intact
  // Leave m_url     intact
  // Leave m_absPath intact
  // Leave m_servicePostfix intact
}

void
WSDLCache::ClearCache()
{
  OperationMap::iterator it;
  for(it = m_operations.begin();it != m_operations.end(); ++it)
  {
    delete it->second.m_input;
    delete it->second.m_output;
  }
  m_operations.clear();
}

// MANDATORY: Set a service
// Webroot must be set first !!!
bool
WSDLCache::SetService(XString p_servicename,XString p_url)
{
  // Record service name and URL
  m_serviceName = p_servicename;
  m_url         = p_url;

  // Calculate WSDL filename
  CrackedURL cracked(p_url);
  if(cracked.Valid() && !m_webroot.IsEmpty())
  {
    // Remember absolute path
    m_absPath = cracked.m_path;

    // Only if not yet set by ReadWSDLFile() !!
    if(m_filename.IsEmpty())
    {
      m_filename = m_webroot + _T("\\") + cracked.m_path + _T("\\") + m_serviceName + _T(".wsdl");
      m_filename.Replace(_T('/'),'\\');
      m_filename.Replace(_T("\\\\"),_T("\\"));
    }
    return true;
  }
  return false;
}

// Add SOAP message call and answer
// Call exactly once for every SOAPMessage combination
bool 
WSDLCache::AddOperation(int p_code,XString p_name,SOAPMessage* p_input,SOAPMessage* p_output)
{
  // See if the operation is already registered
  OperationMap::iterator it = m_operations.find(p_name);
  if(it != m_operations.end())
  {
    return false;
  }

  // Register a new operation
  WsdlOperation operation;
  operation.m_code   = p_code;
  operation.m_input  = new SOAPMessage(p_input);
  operation.m_output = new SOAPMessage(p_output);

  m_operations.insert(std::make_pair(p_name,operation));
  return true;
}

// Get command code from SOAP command name
int
WSDLCache::GetCommandCode(XString& p_commandName)
{
  OperationMap::iterator it = m_operations.find(p_commandName);

  if(it != m_operations.end())
  {
    return it->second.m_code;
  }
  return 0;
}

bool
WSDLCache::GenerateWSDL()
{
  // See if WSDL filename set
  if(m_filename.IsEmpty() || m_targetNamespace.IsEmpty() || m_webroot.IsEmpty())
  {
    return false;
  }
  // See if we have work to do
  if(m_performSoap10 == false && m_performSoap12 == false)
  {
    return false;
  }
  // See if there are registered operations
  if(m_operations.empty())
  {
    return false;
  }

  // Open file;
  WinFile file(m_filename);
  file.CreateDirectory();
  if(file.Open(winfile_write | open_trans_text,FAttributes::attrib_none,Encoding::UTF8))
  {
    XString wsdlcontent;

    // GENERATE THE CONTENT

    // Step 1: Generate types
    GenerateTypes(wsdlcontent);
    // Step 2: Generate messages
    GenerateMessages(wsdlcontent);
    // Step 3: Generate port type
    GeneratePortType(wsdlcontent);
    // Step 4: Generate bindings
    GenerateBindings(wsdlcontent);
    // Step 5: Generate service bindings
    GenerateServiceBindings(wsdlcontent);
    // Step 6: Generate definitions envelope
    GenerateDefinitions(wsdlcontent);

    // Print content to the WSDL file
    file.Write(wsdlcontent);
    if(!file.Close())
    {
      return false;
    }
    XString generated;
    generated.Format(_T("WSDL File for service [%s] generated in: %s"),m_serviceName.GetString(),m_filename.GetString());
    DETAILLOG(generated);
    return true;
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////
//
// GENERATING THE CONTENT OF THE WSDL FILE
//
//////////////////////////////////////////////////////////////////////////

// Generate types
void
WSDLCache::GenerateTypes(XString& p_wsdlcontent)
{
  XString  temp;
  TypeDone done;

  p_wsdlcontent += _T("\n<!-- Abstract types -->\n");

  // Generate header
  temp.Format(_T("<wsdl:types>\n")
              _T("  <s:schema elementFormDefault=\"qualified\" targetNamespace=\"%s\">\n"),m_targetNamespace.GetString());
  p_wsdlcontent += temp;

  OperationMap::iterator it;
  for(it = m_operations.begin();it != m_operations.end(); ++it)
  {
    GenerateMessageTypes(p_wsdlcontent,it->second.m_input, done);
    GenerateMessageTypes(p_wsdlcontent,it->second.m_output,done);
  }
  // Footer types
  p_wsdlcontent += _T("  </s:schema>\n")
                   _T("</wsdl:types>\n");
}

// Generate types for one message
void
WSDLCache::GenerateMessageTypes(XString&      p_wsdlcontent
                               ,SOAPMessage*  p_msg
                               ,TypeDone&     p_gedaan)
{
  // Put the parameters here
  GenerateParameterTypes(p_wsdlcontent
                        ,p_msg->GetSoapAction()
                        ,p_msg->m_paramObject->GetChildren()
                        ,p_gedaan
                        ,p_msg->GetWSDLOrder()
                        ,0);
}

// Generate types for all parameters
void
WSDLCache::GenerateParameterTypes(XString&       p_wsdlcontent
                                 ,XString        p_element
                                 ,XmlElementMap& p_map
                                 ,TypeDone&      p_done
                                 ,WsdlOrder      p_order
                                 ,int            p_level)
{
  XString temp;

  // Test for duplicates
  TypeDone::iterator it = p_done.find(p_element);
  if(it != p_done.end())
  {
    // Already done. Do not do another time
    return;
  }

  // No parameters given
  if(p_map.size() == 0)
  {
    return;
  }


  // Element header
  if(p_level == 0)
  {
    p_wsdlcontent.AppendFormat(_T("    <s:element name=\"%s\">\n"),p_element.GetString());
  }

  // Get the ordering
  XString order;
  switch(p_order)
  {
    default:                     [[fallthrough]];
    case WsdlOrder::WS_All:      order = _T("all");      break;
    case WsdlOrder::WS_Choice:   order = _T("choice");   break;
    case WsdlOrder::WS_Sequence: order = _T("sequence"); break;
  }

  // Header of the complex type
  if(p_level)
  {
    p_wsdlcontent.AppendFormat(_T("    <s:complexType name=\"%s\">\n"),p_element.GetString());
  }
  else
  {
    p_wsdlcontent.AppendFormat(_T("    <s:complexType>\n"));
  }
  p_wsdlcontent.AppendFormat(_T("      <s:%s>\n"),order.GetString());  // all, choice or sequence


  for (unsigned ind = 0; ind < p_map.size(); ++ind)
  {
    XString array;
    XMLElement* param = p_map[ind];
    //Only when the value is not empty or the number of children > 0
    if(param->GetChildren().size() > 0 || !param->GetName().IsEmpty() || !param->GetValue().IsEmpty())
    {
      // Begin element
      p_wsdlcontent += _T("        <s:element ");

      // Do occurrence type
      switch (param->GetType() & WSDL_Mask)
      {
        default:              [[fallthrough]];
        case WSDL_OnceOnly:   [[fallthrough]];
        case WSDL_Mandatory:  temp.Empty();
                              break;
        case WSDL_ZeroOne:    [[fallthrough]];
        case WSDL_Optional:   temp = _T("minOccurs=\"0\" ");
                              break;
        case WSDL_ZeroMany:   temp = _T("minOccurs=\"0\" maxOccurs=\"unbounded\" ");
                              array = p_element;
                              break;
        case WSDL_OneMany:    temp = _T("maxOccurs=\"unbounded\" ");
                              array = p_element;
                              break;
      }
      p_wsdlcontent += temp;

      // Do name
      temp.Format(_T("name=\"%s\""), param->GetName().GetString());
      p_wsdlcontent += temp;

      // Do data type
      switch (param->GetType() & XDT_Mask)
      {
        case XDT_CDATA:             [[fallthrough]];
        case (XDT_String|XDT_CDATA):[[fallthrough]];
        case XDT_String:            temp = _T(" type=\"s:string\"");              break;
        case XDT_Integer:           temp = _T(" type=\"s:integer\"");             break;
        case XDT_Double:            temp = _T(" type=\"s:double\"");              break;
        case XDT_Boolean:           temp = _T(" type=\"s:boolean\"");             break;
        case XDT_Base64Binary:      temp = _T(" type=\"s:base64Binary\"");        break;
        case XDT_DateTime:          temp = _T(" type=\"s:dateTime\"");            break;
        case XDT_AnyURI:            temp = _T(" type=\"s:anyURI\"");              break;
        case XDT_Date:              temp = _T(" type=\"s:date\"");                break;
        case XDT_DateTimeStamp:     temp = _T(" type=\"s:dateTimeStamp\"");       break;
        case XDT_Decimal:           temp = _T(" type=\"s:decimal\"");             break;
        case XDT_Long:              temp = _T(" type=\"s:long\"");                break;
        case XDT_Int:               temp = _T(" type=\"s:int\"");                 break;
        case XDT_Short:             temp = _T(" type=\"s:short\"");               break;
        case XDT_Byte:              temp = _T(" type=\"s:byte\"");                break;
        case XDT_NonNegativeInteger:temp = _T(" type=\"s:nonNegativeInteger\"");  break;
        case XDT_PositiveInteger:   temp = _T(" type=\"s:positiveInteger\"");     break;
        case XDT_UnsignedLong:      temp = _T(" type=\"s:unsignedLong\"");        break;
        case XDT_UnsignedInt:       temp = _T(" type=\"s:unsignedInt\"");         break;
        case XDT_UnsignedShort:     temp = _T(" type=\"s:unsignedShort\"");       break;
        case XDT_UnsignedByte:      temp = _T(" type=\"s:unsignedByte\"");        break;
        case XDT_NonPositiveInteger:temp = _T(" type=\"s:nonPositiveInteger\"");  break;
        case XDT_NegativeInteger:   temp = _T(" type=\"s:negativeInteger\"");     break;
        case XDT_Duration:          temp = _T(" type=\"s:duration\"");            break;
        case XDT_DayTimeDuration:   temp = _T(" type=\"s:dayTimeDuration\"");     break;
        case XDT_YearMonthDuration: temp = _T(" type=\"s:yearMonthDuration\"");   break;
        case XDT_Float:             temp = _T(" type=\"s:float\"");               break;
        case XDT_GregDay:           temp = _T(" type=\"s:gDay\"");                break;
        case XDT_GregMonth:         temp = _T(" type=\"s:gMonth\"");              break;
        case XDT_GregMonthDay:      temp = _T(" type=\"s:gMonthDay\"");           break;
        case XDT_GregYear:          temp = _T(" type=\"s:gYear\"");               break;
        case XDT_GregYearMonth:     temp = _T(" type=\"s:gYearMonth\"");          break;
        case XDT_HexBinary:         temp = _T(" type=\"s:hexBinary\"");           break;
        case XDT_NOTATION:          temp = _T(" type=\"s:NOTATION\"");            break;
        case XDT_QName:             temp = _T(" type=\"s:QName\"");               break;
        case XDT_NormalizedString:  temp = _T(" type=\"s:normalizedString\"");    break;
        case XDT_Token:             temp = _T(" type=\"s:token\"");               break;
        case XDT_Language:          temp = _T(" type=\"s:language\"");            break;
        case XDT_Name:              temp = _T(" type=\"s:name\"");                break;
        case XDT_NCName:            temp = _T(" type=\"s:NCName\"");              break;
        case XDT_ENTITY:            temp = _T(" type=\"s:ENTITY\"");              break;
        case XDT_ID:                temp = _T(" type=\"s:ID\"");                  break;
        case XDT_IDREF:             temp = _T(" type=\"s:IDREF\"");               break;
        case XDT_NMTOKEN:           temp = _T(" type=\"s:NMTOKEN\"");             break;
        case XDT_Time:              temp = _T(" type=\"s:time\"");                break;
        case XDT_ENTITIES:          temp = _T(" type=\"s:ENTITIES\"");            break;
        case XDT_IDREFS:            temp = _T(" type=\"s:IDREFS\"");              break;
        case XDT_NMTOKENS:          temp = _T(" type=\"s:NMTOKENS\"");            break;
        case XDT_Complex:           temp.Format(_T(" type=\"tns:%s%s\""),p_element.GetString(),param->GetName().GetString());
                                    break;
        default:                    temp = _T(" type=\"s:string\"");
                                    break;
      }
      p_wsdlcontent += temp;

      // End of element
      p_wsdlcontent += _T(" />\n");
    }
  }

  // Footer complex type sequence
  p_wsdlcontent.AppendFormat(_T("      </s:%s>\n")
                             _T("    </s:complexType>\n"),order.GetString());
  if(p_level == 0)
  {
    p_wsdlcontent += _T("    </s:element>\n");
  }

  // This type has been done. Do not do it again
  p_done.insert(std::make_pair(p_element,1));


  // Try recursion for complex type, or simply the added type definition
  for(unsigned ind = 0;ind < p_map.size(); ++ind)
  {
    XMLElement* param = p_map[ind];

    if(param->GetChildren().size())
    {
      // Recurse WITHOUT postfix and target namespace
      XString name;
      if((param->GetType() & WSDL_Mask) == WSDL_OneMany  ||
         (param->GetType() & WSDL_Mask) == WSDL_ZeroMany ||
         (param->GetType() & XDT_Mask)  == XDT_Complex   )
      {
        name += p_element;
      }
      name += param->GetName();
      GenerateParameterTypes(p_wsdlcontent,name,param->GetChildren(),p_done,p_order,p_level + 1);
    }
  }
}

// Generate messages
void
WSDLCache::GenerateMessages(XString& p_wsdlcontent)
{
  XString temp;

  p_wsdlcontent += _T("\n<!-- Abstract messages -->\n");

  OperationMap::iterator it;
  for(it = m_operations.begin();it != m_operations.end(); ++it)
  {
    // Input message
    temp.Format(_T("<wsdl:message name=\"%sSoapIn\">\n"),it->first.GetString());
    p_wsdlcontent += temp;
    temp.Format(_T("  <wsdl:part name=\"parameters\" element=\"tns:%s\" />\n"),it->second.m_input->GetSoapAction().GetString());
    p_wsdlcontent += temp;
    p_wsdlcontent += _T("</wsdl:message>\n");

    // Output message
    temp.Format(_T("<wsdl:message name=\"%sSoapOut\">\n"),it->first.GetString());
    p_wsdlcontent += temp;
    temp.Format(_T("  <wsdl:part name=\"parameters\" element=\"tns:%s\" />\n"),it->second.m_output->GetSoapAction().GetString());
    p_wsdlcontent += temp;
    p_wsdlcontent += _T("</wsdl:message>\n");
  }
}

// Generate porttype
void
WSDLCache::GeneratePortType(XString& p_wsdlcontent)
{
  XString temp;

  p_wsdlcontent += _T("\n<!-- Abstract porttypes -->\n");

  // Generate port type header
  temp.Format(_T("<wsdl:portType name=\"%s\">\n"),m_serviceName.GetString());
  p_wsdlcontent += temp;

  // Generate port type of messages
  OperationMap::iterator it;
  for(it = m_operations.begin();it != m_operations.end(); ++it)
  {
    // Operation header;
    temp.Format(_T("  <wsdl:operation name=\"%s\">\n"),it->first.GetString());
    p_wsdlcontent += temp;

    // input
    temp.Format(_T("    <wsdl:input message=\"tns:%sSoapIn\" />\n"),it->first.GetString());
    p_wsdlcontent += temp;

    // output
    temp.Format(_T("    <wsdl:output message=\"tns:%sSoapOut\" />\n"),it->first.GetString());
    p_wsdlcontent += temp;

    // operation footer
    p_wsdlcontent += _T("  </wsdl:operation>\n");
  }

  // Generate port type footer
  p_wsdlcontent += _T("</wsdl:portType>\n");
}

// Generate bindings
void
WSDLCache::GenerateBindings(XString& p_wsdlcontent)
{
  p_wsdlcontent += _T("\n<!-- Concrete binding with SOAP -->\n");

  if(m_performSoap10)
  {
    GenerateBinding(p_wsdlcontent,_T("Soap"),_T("soap"));
  }
  if(m_performSoap12)
  {
    GenerateBinding(p_wsdlcontent,_T("Soap12"),_T("soap12"));
  }
}

// Generate detailed binding
void
WSDLCache::GenerateBinding(XString& p_wsdlcontent,XString p_binding,XString p_soapNamespace)
{
  XString temp;

  // Generate binding header
  temp.Format(_T("<wsdl:binding name=\"%s%s\" type=\"tns:%s\">\n"),m_serviceName.GetString(),p_binding.GetString(),m_serviceName.GetString());
  p_wsdlcontent += temp;
  // Generate soap transport binding
  temp.Format(_T("  <%s:binding transport=\"http://schemas.xmlsoap.org/soap/http/\" />\n"),p_soapNamespace.GetString());
  p_wsdlcontent += temp;

  // Generate all operations
  OperationMap::iterator it;
  for(it = m_operations.begin();it != m_operations.end(); ++it)
  {
    // Operation
    temp.Format(_T("  <wsdl:operation name=\"%s\">\n"),it->first.GetString());
    p_wsdlcontent += temp;

    // Soap Operation
    temp.Format(_T("    <%s:operation soapAction=\"%s/%s\" style=\"document\" />\n"),p_soapNamespace.GetString(),m_targetNamespace.GetString(),it->first.GetString());
    p_wsdlcontent += temp;
    // literal input/output
    temp.Format(_T("    <wsdl:input><%s:body use=\"literal\" /></wsdl:input>\n"),p_soapNamespace.GetString());
    p_wsdlcontent += temp;
    temp.Format(_T("    <wsdl:output><%s:body use=\"literal\" /></wsdl:output>\n"),p_soapNamespace.GetString());
    p_wsdlcontent += temp;

    // End of operation
    p_wsdlcontent += _T("  </wsdl:operation>\n");
  }

  // Generate binding footer
  p_wsdlcontent += _T("</wsdl:binding>\n");
}

// Generate service bindings
void
WSDLCache::GenerateServiceBindings(XString& p_wsdlcontent)
{
  XString temp;
  XString path;
  path.Format(_T("%s%s%s%s"), m_url.GetString(), m_url.GetAt(m_url.GetLength() - 1) == _T('/') ? _T("") : _T("/"), m_serviceName.GetString(), m_servicePostfix.GetString());
                         
  p_wsdlcontent += _T("\n<!-- Web service offering endpoints for bindings -->\n");

  // Begin envelope
  temp.Format(_T("<wsdl:service name=\"%s\">\n"),m_serviceName.GetString());
  p_wsdlcontent += temp;

  if(m_performSoap10)
  {
    temp.Format(_T("  <wsdl:port name=\"%sSoap\" binding=\"tns:%sSoap\">\n"),m_serviceName.GetString(),m_serviceName.GetString());
    p_wsdlcontent += temp;
    temp.Format(_T("    <soap:address location=\"%s\" />\n"),path.GetString());
    p_wsdlcontent += temp;
    p_wsdlcontent += _T("  </wsdl:port>\n");
  }
  if(m_performSoap12)
  {
    temp.Format(_T("  <wsdl:port name=\"%sSoap12\" binding=\"tns:%sSoap12\">\n"),m_serviceName.GetString(),m_serviceName.GetString());
    p_wsdlcontent += temp;
    temp.Format(_T("    <soap12:address location=\"%s\" />\n"),path.GetString());
    p_wsdlcontent += temp;
    p_wsdlcontent += _T("  </wsdl:port>\n");
  }
  // Generate envelope end
  p_wsdlcontent += _T("</wsdl:service>\n");
}

// Generate definitions envelope
// This is a WSDL 1.1 definition!!
// Microsoft C# .NET cannot handle any higher definitions!!
// And we do not want to lose compatibility with that platform
void
WSDLCache::GenerateDefinitions(XString& p_wsdlcontent)
{
  XString header(_T("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"));

  XString def = _T("<wsdl:definitions \n");
  def += _T("  xmlns:soap=\"http://schemas.xmlsoap.org/wsdl/soap/\"\n");
  def += _T("  xmlns:soap12=\"http://schemas.xmlsoap.org/wsdl/soap12/\"\n");
  def += _T("  xmlns:tm=\"http://microsoft.com/wsdl/mime/textMatching/\"\n");
  def += _T("  xmlns:soapenc=\"http://schemas.xmlsoap.org/soap/encoding/\"\n");
  def += _T("  xmlns:mime=\"http://schemas.xmlsoap.org/wsdl/mime/\"\n");
  def += _T("  xmlns:s=\"http://www.w3.org/2001/XMLSchema\"\n");
  def += _T("  xmlns:http=\"http://schemas.xmlsoap.org/wsdl/http/\"\n");
  def += _T("  xmlns:wsdl=\"http://schemas.xmlsoap.org/wsdl/\"\n");

  // BEWARE: In .NET 2.0 it used to be:
  // def += "  xmlns:http=\"http://schemas.xmlsoap.org/soap/http/\"\n";

  // Adding the real target namespace
  XString temp;
  temp.Format(_T("  xmlns:tns=\"%s\"\n"),m_targetNamespace.GetString());
  def += temp;
  temp.Format(_T("  targetNamespace=\"%s\">\n"),m_targetNamespace.GetString());
  def += temp;

  p_wsdlcontent = header + def + p_wsdlcontent + _T("</wsdl:definitions>\n");
}

//////////////////////////////////////////////////////////////////////////
//
// RUNTIME CHECKING INTERFACE
//
//////////////////////////////////////////////////////////////////////////

// Check incoming SOAP message against WSDL
bool
WSDLCache::CheckIncomingMessage(SOAPMessage* p_msg,bool p_checkFields)
{
  XString name = p_msg->GetSoapAction();
  OperationMap::iterator it = m_operations.find(name);

  if(it != m_operations.end())
  {
    return CheckMessage(it->second.m_input,p_msg,_T("Client"),p_checkFields);
  }
  // No valid operation found
  p_msg->Reset();
  p_msg->SetFault(_T("No operation"),_T("Client"),_T("No operation [") + p_msg->GetSoapAction() + _T("] found"),_T("While testing against WSDL"));
  return false;
}

// Check outgoing SOAP message against WSDL
bool
WSDLCache::CheckOutgoingMessage(SOAPMessage* p_msg,bool p_checkFields)
{
  XString name = p_msg->GetSoapAction();
  

  // Check if we are already in error state
  // So we can send already generated errors.
  if(p_msg->GetErrorState())
  {
    return true;
  }

  // See if it is an registered operation in this WSDL
  OperationMap::iterator it = m_operations.find(name);
  if(it != m_operations.end())
  {
    return CheckMessage(it->second.m_output,p_msg,_T("Server"),p_checkFields);
  }
  // No valid operation found
  p_msg->Reset();
  p_msg->SetFault(_T("No operation"),_T("Server"),_T("No operation [") + p_msg->GetSoapAction() + _T("] found"),_T("While testing against WSDL"));
  return false;
}

// Check message
bool
WSDLCache::CheckMessage(SOAPMessage* p_orig,SOAPMessage* p_tocheck,XString p_who,bool p_checkFields)
{
  if(p_orig == p_tocheck)
  {
    p_tocheck->Reset();
    p_tocheck->SetFault(_T("No reuse"),p_who,_T("Server cannot reuse registration messages"),_T("While testing against WSDL"));
    return false;
  }
  // Check parameter object
  if((p_orig->GetParameterObject() != p_tocheck->GetParameterObject()) &&
     !p_orig->GetParameterObject().IsEmpty() &&
     !p_tocheck->GetParameterObject().IsEmpty())
  {
    XString msg;
    msg.Format(_T("Request/Response object not the same. Expected '%s' got '%s'")
              ,p_orig->GetParameterObject().GetString()
              ,p_tocheck->GetParameterObject().GetString());

    p_tocheck->Reset();
    p_tocheck->SetFault(_T("Request/Response object"),p_who,msg,_T("While testing against WSDL"));
    return false;

  }
  if(p_orig->GetParameterCount() && p_tocheck->GetParameterCount())
  {
    // Recursively check all parameters
    XMLElement* orig  = p_orig->GetParameterObjectNode();
    XMLElement* param = p_tocheck->GetParameterObjectNode();
    return CheckParameters(orig,p_orig,param,p_tocheck,p_who,p_checkFields);
  }
  // One of both have no parameters. Always allowed (but no very efficient in calling!)
  return true;
}

// Check parameters
bool
WSDLCache::CheckParameters(XMLElement*  p_orgBase
                          ,SOAPMessage* p_orig
                          ,XMLElement*  p_checkBase
                          ,SOAPMessage* p_check
                          ,XString      p_who
                          ,bool         p_fields)
{
  XmlDataType type = 0;
  XMLElement* orgParam   = p_orig->GetElementFirstChild(p_orgBase);
  XMLElement* checkParam = p_check->GetElementFirstChild(p_checkBase);
  bool        scanning   = false;
  while(orgParam)
  {
    // Name of node to find in the message to check
    XString orgName = orgParam->GetName();
    type = orgParam->GetType();
    // If the ordering is choice, instead of sequence: do a free search
    if(!(type & WSDL_Sequence) && !scanning)
    {
      checkParam = p_check->FindElement(p_checkBase,orgName,false);
    }
    XString chkName = checkParam ? checkParam->GetName() : XString();

    // DO CHECKS

    // Parameter is mandatory but not given in the definition
    if((orgName != chkName) && (type & WSDL_Mandatory))
    {
      p_check->Reset();
      p_check->SetFault(_T("Mandatory field not found"),p_who,_T("Message is missing a field"),orgParam->GetName());
      return false;
    }

    // Only if parameter field found
    if(orgName == chkName)
    {
      if(checkParam)
      {
        if(p_fields)
        {
          if(CheckFieldDatatypeValues(orgParam,checkParam,p_check,p_who) == false)
          {
            return false;
          }
        }
        // RECURSE
        if(orgParam->GetChildren().size())
        {
          if(CheckParameters(orgParam,p_orig,checkParam,p_check,p_who,p_fields) == false)
          {
            return false;
          }
        }
      }

      // Message can have more than one nodes of this name
      // So check that next node, before continuing on the original template
      if((type & WSDL_OneMany) || (type & WSDL_ZeroMany))
      {
        XMLElement* next = p_check->GetElementSibling(checkParam);
        if(next && next->GetName().Compare(orgName) == 0)
        {
          checkParam = next;
          scanning = true;
          continue;
        }
      }
      // Get next parameter in sequence list
      scanning   = false;
      checkParam = p_check->GetElementSibling(checkParam);
    }
    // Next parameter in the template
    orgParam = p_orig ->GetElementSibling(orgParam);
    type     = orgParam ? orgParam->GetType() : 0;
  }

  // See if we've got something extra left
  checkParam = p_check->GetElementFirstChild(p_checkBase);
  while(checkParam)
  {
    XString checkName = checkParam->GetName();
    if(p_orig->FindElement(p_orgBase,checkName,false) == nullptr)
    {
      p_check->Reset();
      p_check->SetFault(_T("Extra field found"),p_who,_T("Message has unexpected parameter"),checkName);
      return false;
    }
    // Get next parameter in sequence list
    checkParam = p_check->GetElementSibling(checkParam);
  }

  // Gotten to the end, it's OK
  return true;
}

// Check data field in depth
bool
WSDLCache::CheckFieldDatatypeValues(XMLElement*   p_origParam
                                   ,XMLElement*   p_checkParam
                                   ,SOAPMessage*  p_check
                                   ,XString       p_who)
{
  XString         value;
  XString         result;
  XMLRestriction  restrict(_T("empty"));
  XMLRestriction* restriction = p_checkParam->GetRestriction();
  XmlDataType     type = p_origParam->GetType() & XDT_MaskTypes;

  // Use the restriction, or an empty one
  if(restriction)
  {
    value  = restriction->HandleWhitespace(type,p_checkParam->GetValue());
    result = restriction->CheckDatatype(type,value);
  }
  else
  {
    value  = restrict.HandleWhitespace(type,p_checkParam->GetValue());
    result = restrict.CheckDatatype(type,value);
  }

  // Datatype failed?
  if(!result.IsEmpty())
  {
    XString details;
    details.Format(_T("Datatype check failed! Field: %s Value: %s Result: %s")
                   ,p_checkParam->GetName().GetString()
                   ,value.GetString()
                   ,result.GetString());
    p_check->Reset();
    p_check->SetFault(_T("Datatype"),p_who,_T("Restriction"),details);
    return false;
  }

  // Variable XSD Restriction check, other than the datatype
  // including (min/max)length, digits, fraction, notation, enumerations, pattern etc.
  if(restriction)
  {
    result = restriction->CheckRestriction(type,value);
    if(!result.IsEmpty())
    {
      XString details;
      details.Format(_T("Fieldvalue check failed! Field: %s Value: %s Result: %s")
                     ,p_checkParam->GetName().GetString()
                     ,value.GetString()
                     ,result.GetString());
      p_check->Reset();
      p_check->SetFault(_T("Fieldvalue"),p_who,_T("Restriction"),result);
      return false;
    }
  }
  return true;
}

//////////////////////////////////////////////////////////////////////////
//
// SHOWING THE INTERFACE IN A 'REAL-LIFE' CONNECTION
//
//////////////////////////////////////////////////////////////////////////

// Name of the Service base page
XString
WSDLCache::GetServiceBasePageName()
{
  return m_absPath + m_serviceName + m_servicePostfix; 
}

// Get service page for HTTP GET service
XString 
WSDLCache::GetServicePage()
{
  XString page = GetPageHeader();

  // Getting the WSDL reference
  page += GetOperationWsdl();

  // Getting all the operations links
  OperationMap::iterator it;
  for(it = m_operations.begin();it != m_operations.end();++it)
  {
    page += GetOperationNameLink(it->first);

  }
  return page + GetPageFooter();
}

// Get service page for operation
XString 
WSDLCache::GetOperationPage(XString p_operation,XString p_hostname)
{
  XString temp;
  XString page;

  OperationMap::iterator it = m_operations.find(p_operation);

  if (it == m_operations.end())
  {
    return page;
  }
  const WsdlOperation& operation = it->second;
  page  = GetPageHeader();
  page += GetOperationPageIntro(p_operation);

  if(m_performSoap10)
  {
    page += GetOperationPageHttpI(p_operation,p_hostname,false);
    page += GetOperationPageSoap(operation.m_input, false);
    page += GetOperationPageHttpO(false);
    page += GetOperationPageSoap(operation.m_output,false);
    page += GetOperationPageFooter();
  }

  if(m_performSoap12)
  {
    page += GetOperationPageHttpI(p_operation,p_hostname,true);
    page += GetOperationPageSoap(operation.m_input, true);
    page += GetOperationPageHttpO(true);
    page += GetOperationPageSoap(operation.m_output,true);
    page += GetOperationPageFooter();
  }

  page += GetOperationPageFooter();
  page += GetPageFooter();

  return page;
}

XString
WSDLCache::GetPageHeader()
{
  XString header;
  header.Format(_T("<html>\n")
                 _T("  <head>\n")
                 _T("    <style type=\"text/css\">")
                 _T("      BODY { color: #000000; background-color: white; font-family: Verdana; margin-left: 0px; margin-top: 0px; }\n")
                 _T("      #content { margin-left: 30px; font-size: .70em; padding-bottom: 2em; }\n")
                 _T("      A:link { color: #336699; font-weight: bold; text-decoration: underline; }\n")
                 _T("      A:visited { color: #6699cc; font-weight: bold; text-decoration: underline; }\n")
                 _T("      A:active { color: #336699; font-weight: bold; text-decoration: underline; }\n")
                 _T("      A:hover { color: cc3300; font-weight: bold; text-decoration: underline; }\n")
                 _T("      P { color: #000000; margin-top: 0px; margin-bottom: 12px; font-family: Verdana; }\n")
                 _T("      pre { background-color: #e5e5cc; padding: 5px; font-family: Courier New; font-size: 1.2em; margin-top: -5px; border: 1px #f0f0e0 solid; }\n")
                 _T("      td { color: #000000; font-family: Verdana; font-size: .7em; }\n")
                 _T("      h2 { font-size: 1.5em; font-weight: bold; margin-top: 25px; margin-bottom: 10px; border-top: 1px solid #003366; margin-left: -15px; color: #003366; }\n")
                 _T("      h3 { font-size: 1.1em; color: #000000; margin-left: -15px; margin-top: 10px; margin-bottom: 10px; }\n")
                 _T("      ul { margin-top: 10px; margin-left: 20px; }\n")
                 _T("      ol { margin-top: 10px; margin-left: 20px; }\n")
                 _T("      li { margin-top: 10px; color: #000000; }\n")
                 _T("      font.value { color: darkblue; font: bold; }\n")
                 _T("      font.key { color: darkgreen; font: bold; }\n")
                 _T("      font.error { color: darkred; font: bold; }\n")
                 _T("      .heading1 { color: #ffffff; font-family: Tahoma; font-size: 26px; font-weight: normal; background-color: #003366; margin-top: 0px; margin-bottom: 0px; margin-left: -30px; padding-top: 10px; padding-bottom: 3px; padding-left: 15px; width: 105%%; }\n")
                 _T("      .button { background-color: #dcdcdc; font-family: Verdana; font-size: 1em; border-top: #cccccc 1px solid; border-bottom: #666666 1px solid; border-left: #cccccc 1px solid; border-right: #666666 1px solid; }\n")
                 _T("      .frmheader { color: #000000; background: #dcdcdc; font-family: Verdana; font-size: .7em; font-weight: normal; border-bottom: 1px solid #dcdcdc; padding-top: 2px; padding-bottom: 2px; }\n")
                 _T("      .frmtext { font-family: Verdana; font-size: .7em; margin-top: 8px; margin-bottom: 0px; margin-left: 32px; }\n")
                 _T("      .frmInput { font-family: Verdana; font-size: 1em; }\n")
                 _T("      .intro { margin-left: -15px; }\n")
                 _T("    </style>\n")
                 _T("    <title>%s Web Service</title>\n")
                 _T("  </head>\n")
                 _T("  <body>\n")
                 _T("    <div id=\"content\">\n")
                 _T("    <p class=\"heading1\">%s</p><br>\n")
                ,m_serviceName.GetString()
                ,m_serviceName.GetString());

  return header;
}

XString
WSDLCache::GetPageFooter()
{
  XString footer;
  footer = _T("  </body>\n")
            _T("</html>\n");

  return footer;
}

XString
WSDLCache::GetOperationWsdl()
{
  XString wsdl(_T("<br>\n"));
  XString filename = m_url + m_serviceName + m_servicePostfix;

  wsdl += _T("    <p class=\"intro\">This is the overview of the service.</p>");
  wsdl += _T("    <p class=\"intro\">To use this service you can create a client for it on the basis of the WSDL file below.</p>");
  
  wsdl.AppendFormat(_T("    <pre><a href=\"%s?wsdl\">%s?wsdl</a></pre>"),filename.GetString(),filename.GetString());

  wsdl += _T("    <br>\n");
  wsdl += _T("    <h2>These are all the separate SOAP messages:</h2>\n");
  return wsdl;
}

XString
WSDLCache::GetOperationNameLink(XString p_operation)
{
  XString link;
  XString linkpage;
  linkpage = m_serviceName + _T(".") + p_operation + _T(".html");

  link.Format(_T("    <h3><a href=\"%s\">%s</a></h3>\n"),linkpage.GetString(),p_operation.GetString());
  return link;
}


XString
WSDLCache::GetOperationPageIntro(XString p_operation)
{
  XString intro;

  intro.Format(_T("    <span>\n")
               _T("      <p class=\"intro\">Click <a href=\"%s%s\">here</a> for a complete list of operations.</p>\n")
               _T("      <h2>%s</h2>\n")
               _T("      <p class=\"intro\"></p>\n")
               ,m_serviceName.GetString()
               ,m_servicePostfix.GetString()
               ,p_operation.GetString());

  return intro;
}

XString
WSDLCache::GetOperationPageHttpI(XString p_operation,XString p_hostname,bool p_soapVersion)
{
  XString text; 
  text.Format(_T("      <span>\n")
              _T("        <h3>SOAP %s</h3>\n")
              _T("        <p>The following is a sample SOAP %s request and response.  The <font class=value>placeholders</font> shown need to be replaced with actual values.</p>\n")
              _T("<pre>POST %s/%s%s HTTP/1.1\n")
              _T("Host: %s\n")
              _T("Content-Type: %s; charset=utf-8\n")
              _T("Content-Length: <font class=value>length</font>\n")
              ,p_soapVersion ? _T("1.2") : _T("1.1")
              ,p_soapVersion ? _T("1.2") : _T("1.1")
              ,m_absPath.GetString()
              ,m_serviceName.GetString()
              ,m_servicePostfix.GetString()
              ,p_hostname.GetString()
              ,p_soapVersion ? _T("application/soap+xml") : _T("text/xml"));
  if(p_soapVersion == false)
  {
    XString extra;
    extra.Format(_T("SOAPAction: \"%s/%s\"\n"),m_targetNamespace.GetString(),p_operation.GetString());
    text += extra;
  }
  text += _T("\n");

  return text;
}

XString
WSDLCache::GetOperationPageHttpO(bool p_soapVersion)
{
  XString text;
  text.Format(_T("<pre>HTTP/1.1 200 OK\n")
              _T("Content-Type: %s; charset=utf-8\n")
              _T("Content-Length: <font class=value>length</font>\n\n")
              ,p_soapVersion ? _T("application/soap+xml") : _T("text/xml"));
  return text;
}

XString
WSDLCache::GetOperationPageSoap(SOAPMessage* p_msg,bool p_soapVersion)
{
  // Set the SOAP Version
  p_msg->SetSoapVersion(p_soapVersion ? SoapVersion::SOAP_12 : SoapVersion::SOAP_11);
  p_msg->SetPrintRestrictions(true);

  XString message = p_msg->GetSoapMessage();
  message.Replace(_T("<"),_T("&lt;"));
  message.Replace(_T(">"),_T("&gt;"));

  // Datatypes replacements
  message.Replace(_T("&gt;int&lt;"),          _T("&gt;<font class=value>int</font>&lt;"));
  message.Replace(_T("&gt;string&lt;"),       _T("&gt;<font class=value>string</font>&lt;"));
  message.Replace(_T("&gt;double&lt;"),       _T("&gt;<font class=value>double</font>&lt;"));
  message.Replace(_T("&gt;boolean&lt;"),      _T("&gt;<font class=value>boolean</font>&lt;"));
  message.Replace(_T("&gt;dateTime&lt;"),     _T("&gt;<font class=value>dateTime</font>&lt;"));
  message.Replace(_T("&gt;base64Binary&lt;"), _T("&gt;<font class=value>base64Binary</font>&lt;"));

  // Ending of pre formatted section
  message += _T("</pre>\n");

  return message;
}

XString
WSDLCache::GetOperationPageFooter()
{
  return _T("    </span>\n");
}


//////////////////////////////////////////////////////////////////////////
//
// READING THE WSDL CACHE FROM A WSDL FILE
//
//////////////////////////////////////////////////////////////////////////

struct _baseType
{
  const TCHAR* m_name;
  int         m_type;
}
baseTypes[] =
{
  { _T("anyURI"),               XDT_AnyURI                }
 ,{ _T("base64Binary"),         XDT_Base64Binary          }
 ,{ _T("boolean"),              XDT_Boolean               }
 ,{ _T("date"),                 XDT_Date                  }
 ,{ _T("dateTime"),             XDT_DateTime              }
 ,{ _T("dateTimeStamp"),        XDT_DateTimeStamp         }
 ,{ _T("decimal"),              XDT_Decimal               }
 ,{ _T("integer"),              XDT_Integer               }
 ,{ _T("long"),                 XDT_Long                  }
 ,{ _T("int"),                  XDT_Int                   }
 ,{ _T("short"),                XDT_Short                 }
 ,{ _T("byte"),                 XDT_Byte                  }
 ,{ _T("nonNegativeInteger"),   XDT_NonNegativeInteger    }
 ,{ _T("positiveInteger"),      XDT_PositiveInteger       }
 ,{ _T("unsignedLong"),         XDT_UnsignedLong          }
 ,{ _T("unsignedInt"),          XDT_UnsignedInt           }
 ,{ _T("unsignedShort"),        XDT_UnsignedShort         }
 ,{ _T("unsignedByte"),         XDT_UnsignedByte          }
 ,{ _T("nonPositiveInteger"),   XDT_NonPositiveInteger    }
 ,{ _T("negativeInteger"),      XDT_NegativeInteger       }
 ,{ _T("double"),               XDT_Double                }
 ,{ _T("duration"),             XDT_Duration              }
 ,{ _T("dayTimeDuration"),      XDT_DayTimeDuration       }
 ,{ _T("yearMonthDuration"),    XDT_YearMonthDuration     }
 ,{ _T("float"),                XDT_Float                 }
 ,{ _T("gDay"),                 XDT_GregDay               }
 ,{ _T("gMonth"),               XDT_GregMonth             }
 ,{ _T("gMonthDay"),            XDT_GregMonthDay          }
 ,{ _T("gYear"),                XDT_GregYear              }
 ,{ _T("gYearMonth"),           XDT_GregYearMonth         }
 ,{ _T("hexBinary"),            XDT_HexBinary             }
 ,{ _T("NOTATION"),             XDT_NOTATION              }
 ,{ _T("QName"),                XDT_QName                 }
 ,{ _T("string"),               XDT_String                }
 ,{ _T("normalizedString"),     XDT_NormalizedString      }
 ,{ _T("token"),                XDT_Token                 }
 ,{ _T("language"),             XDT_Language              }
 ,{ _T("Name"),                 XDT_Name                  }
 ,{ _T("NCName"),               XDT_NCName                }
 ,{ _T("ENTITY"),               XDT_ENTITY                }
 ,{ _T("ID"),                   XDT_ID                    }
 ,{ _T("IDREF"),                XDT_IDREF                 }
 ,{ _T("NMTOKEN"),              XDT_NMTOKEN               }
 ,{ _T("time"),                 XDT_Time                  }
 ,{ _T("ENTITIES"),             XDT_ENTITIES              }
 ,{ _T("IDREFS"),               XDT_IDREFS                }
 ,{ _T("NMTOKENS"),             XDT_NMTOKENS              }
 ,{ _T("anyAtomicType"),        XDT_String                }
 ,{ _T("anySimpleType"),        XDT_String                }
 ,{ _T("anyType"),              XDT_String                }
 ,{ NULL,                   0                         }
};

// Reading a WSDL file is protected by a SEH handler
// Users could provide a WSDL definition with circular references
// causing the reading functions to overrun the stack space
bool
WSDLCache::ReadWSDLFile(LPCTSTR p_filename)
{
  try
  {
    return ReadWSDLFileSafe(p_filename);
  }
  catch(StdException& ex)
  {
    if(ex.GetSafeExceptionCode())
    {
      // We need to detect the fact that a second exception can occur,
      // so we do **not** call the error report method again
      // Otherwise we would end into an infinite loop
      m_exception = true;
      m_exception = ErrorReport::Report(ex.GetSafeExceptionCode(),ex.GetExceptionPointers());

      if(m_exception)
      {
        // Error while sending an error report
        // This error can originate from another thread, OR from the sending of this error report
        CRASHLOG(0xFFFF,_T("DOUBLE INTERNAL ERROR while making an error report.!!"));
        m_exception = false;
      }
      else
      {
        CRASHLOG(WER_S_REPORT_UPLOADED,_T("CRASH: Errorreport while reading WSDL has been made"));
      }
    }
    else
    {
      // 'Normal' C++ exception: But we forgot to catch it!
      XString empty;
      ErrorReport::Report(ex.GetErrorMessage(),0,m_webroot,empty);
    }
  }
  return false;
}

bool
WSDLCache::ReadWSDLFileSafe(LPCTSTR p_filename)
{
  bool result = false;

  XString filename(p_filename);
  XString protocol1 = filename.Left(8);
  XString protocol2 = filename.Left(7);
  protocol1.MakeLower();
  protocol2.MakeLower();
  if(protocol1 == _T("https://") || protocol2 == _T("http://"))
  {
    result = ReadWSDLFileFromURL(p_filename);
  }
  else
  {
    result = ReadWSDLLocalFile(p_filename);
  }

  if(result)
  {
    // Remember the filename/URL
    m_filename = p_filename;
  }
  return result;
}

bool
WSDLCache::ReadWSDLFileFromURL(XString p_url)
{
  bool result = false;

  HTTPClient client;
  FileBuffer buffer;

  client.SetLogging(m_logging);
  if(client.Send(p_url,&buffer))
  {
    uchar* contents = nullptr;
    size_t size = 0;
    if(buffer.GetBufferCopy(contents,size))
    {
      XString message(reinterpret_cast<LPCTSTR>(contents));
      XMLMessage wsdl;
      wsdl.ParseMessage(message);
      if(wsdl.GetInternalError() == XmlError::XE_NoError)
      {
        result = ReadWSDL(wsdl);
      }
      delete[] contents;
    }
  }
  return result;
}

bool
WSDLCache::ReadWSDLLocalFile(XString p_filename)
{
  XString message;
  message.Format(_T("Reading WSDL file: %s"),p_filename.GetString());
  DETAILLOG(message);

  // Reading the WSDL as a XML message
  XMLMessage wsdl;
  if(wsdl.LoadFile(p_filename) == false)
  {
    DETAILLOG(_T("Abort reading WSDL file. Cannot load XML document"));
    return false;
  }
  return ReadWSDL(wsdl);
}

// Read an existing WSDL from a file buffer
bool
WSDLCache::ReadWSDLString(XString p_wsdl)
{
  DETAILLOG(_T("Reading WSDL from internal buffer"));

  // Reading the WSDL as a XML message
  XMLMessage wsdl;
  wsdl.ParseMessage(p_wsdl);
  if(wsdl.GetInternalError() == XmlError::XE_NoError)
  {
    return ReadWSDL(wsdl);
  }
  DETAILLOG(_T("Abort reading WSDL buffer. Cannot load XML document"));
  return false;
}

bool
WSDLCache::ReadWSDL(XMLMessage& p_wsdl)
{
  // Start freshly for a new WSDL
  Reset();

  // READING THE CONTENT

  // Step 1: Check Definitions
  if(ReadDefinitions(p_wsdl) == false)
  {
    Reset();
    DETAILLOG(_T("Abort reading WSDL file. Cannot load definition part"));
    DETAILLOG(m_errormessage);
    return false;
  }
  // Step 2: Read Service bindings
  if(ReadServiceBindings(p_wsdl) == false)
  {
    Reset();
    DETAILLOG(_T("Abort reading WSDL file. Cannot load service bindings"));
    DETAILLOG(m_errormessage);
    return false;
  }

  // Step 3: Read bindings
  ReadBindings(p_wsdl);
//   if(ReadBindings(p_wsdl) == false)
//   {
//     Reset();
//     return false;
//   }

  // Step 4: Read port types / messages / types
  if(ReadPortTypes(p_wsdl) == false)
  {
    Reset();
    DETAILLOG(_T("Abort reading WSDL file. Cannot load port types and messages"));
    DETAILLOG(m_errormessage);
    return false;
  }
  // Clear the temporary cache for the types
  m_types.clear();

  DETAILLOG(_T("WSDL Read in completely"));
  return true;
}

// IMPORT TO BE DONE: <import>
// <types> -> <schema> -> <import + schemaLocation>
// <wsdl:types>
//   <xs:schema targetNamespace="http://www.w3.org/2002/ws/databinding/examples/6/09/">
//     <xs:import namespace="http://example.com/a/namespace" 
//                schemaLocation="http://www.w3.org/2002/ws/databinding/examples/6/09/static/Imported.xsd"/ >
//     <xs:element name="echoImportSchema">
//       <xs:complexType>
//         <xs:sequence>
//           <xs:element ref="ex:importSchema"ElementReference / >
//         </xs:sequence> 
//       </xs:complexType>
//     </xs:element>
//   </xs:schema>
// </wsdl:types>

bool
WSDLCache::ReadDefinitions(XMLMessage& p_wsdl)
{
  XMLElement* root = p_wsdl.GetRoot();

  // Check root name (= "definitions" in namespace "wsdl" / attrib xmlns:wsdl= ... etc
  if((root->GetName().Compare(_T("definitions"))) ||
     (root->GetNamespace().Compare(_T("wsdl"))))
  {
    // Not a WSDL definition
    m_errormessage = _T("Not a WSDL file beginning with <wsdl:definitions...>");
    return false;
  }

  // Check attribute "targetNamespace" -> m_targetnamespace
  XMLAttribute* target = p_wsdl.FindAttribute(root,_T("targetNamespace"));
  if(target && target->m_namespace.IsEmpty())
  {
    m_targetNamespace = target->m_value;
  }
  else
  {
    m_errormessage = _T("No targetNamespace found in the WSDL");
    return false;
  }

  // Check attribute soap
  XMLAttribute* soap = p_wsdl.FindAttribute(root,_T("soap"));
  m_performSoap10 = (soap && soap->m_namespace.Compare(_T("xmlns")) == 0);
  
  // Check attribute soap12
  XMLAttribute* soap12 = p_wsdl.FindAttribute(root,_T("soap12"));
  m_performSoap12 = (soap12 && soap12->m_namespace.Compare(_T("xmlns")) == 0);

  // optional "xmlns:tns"
  XMLAttribute* tns = p_wsdl.FindAttribute(root,_T("tns"));
  if(tns)
  {
    if(tns->m_namespace.Compare(_T("xmlns")) == 0 && tns->m_value.CompareNoCase(target->m_value))
    {
      m_errormessage = _T("Namespaces 'tns' and 'targetNamespace' are not the same!");
      return false;
    }
  }
  return true;
}

bool
WSDLCache::ReadServiceBindings(XMLMessage& p_wsdl)
{
  // "service" is a first level node in the root of the WSDL
  XMLElement* service = p_wsdl.FindElement(_T("service"),false);
  if(!service)
  {
    m_errormessage = _T("No <wsdl:service> within the WSDL.");
    return false;
  }

  const XMLAttribute* name = p_wsdl.FindAttribute(service,_T("name"));
  if(name)
  {
    m_serviceName = name->m_value;
  }
  else
  {
    m_errormessage = _T("No service name attribute in <wsdl:service>");
    return false;
  }

  XMLElement* address10 = p_wsdl.FindElement(service,_T("soap:address"));
  XMLElement* address12 = p_wsdl.FindElement(service,_T("soap12:address"));

  if(address10 && !m_performSoap10)
  {
    m_errormessage = _T("SOAP 1.0 service binding found, but no SOAP 1.0 namespace.");
    return false;
  }

  if(address12 && !m_performSoap12)
  {
    m_errormessage = _T("SOAP 1.2 service binding found, but no SOAP 1.2 namespace.");
    return false;
  }

  // Use highest soap version
  XMLElement* address = address12 ? address12 : address10;
  const XMLAttribute* location = p_wsdl.FindAttribute(address,_T("location"));
  if(!location)
  {
    m_errormessage = _T("Missing <location> in <wsdl:service>");
    return false;
  }
  XString loc = location->m_value;
  loc.TrimRight('/');

  int posPoint = loc.ReverseFind('.');
  int posName  = loc.ReverseFind('/');

  // Find service postfix
  m_servicePostfix.Empty();
  if(posPoint > 0 && posName > 0 && posPoint > posName)
  {
    m_servicePostfix = loc.Mid(posPoint);
    loc = loc.Left(posPoint);
  }

  if(posName > 0)
  {
    XString locname = loc.Mid(posName + 1);
    if(locname.CompareNoCase(m_serviceName))
    {
      m_errormessage = _T("Address location binding not for service name.");
      return false;
    }
    m_url = loc.Left(posName);
  }
  else
  {
    m_errormessage = _T("Service address: url without a service");
    return false;
  }
  return true;
}

bool
WSDLCache::ReadBindings(XMLMessage& /*p_wsdl*/)
{
  // Not used at the moment
  return true;
}

bool
WSDLCache::ReadPortTypes(XMLMessage& p_wsdl)
{
  // "portType" is a first level node in the root of the WSDL
  XMLElement* portType = p_wsdl.FindElement(_T("portType"));
  if(!portType)
  {
    m_errormessage = _T("No <wsdl:portType> within the WSDL.");
    return false;
  }

  // Number of the operation
  int index = 0;

  // Walk the list of operations
  XMLElement* operation = p_wsdl.FindElement(portType,_T("operation"),false);
  while(operation)
  {
    XString name;
    XString msgInput;
    XString msgOutput;
    const XMLAttribute* nameAtt = p_wsdl.FindAttribute(operation,_T("name"));
    if(nameAtt)
    {
      name = nameAtt->m_value;
    }
    else
    {
      m_errormessage = _T("WSDL <portType>/<operation> without a name attribute");
      return false;
    }
    XMLElement* input  = p_wsdl.FindElement(operation,_T("input"), false);
    XMLElement* output = p_wsdl.FindElement(operation,_T("output"),false);
    if(input == nullptr || output == nullptr)
    {
      m_errormessage = _T("WSDL <portType> operation must have a <input> AND a <output> node!");
      return false;
    }
    msgInput  = p_wsdl.GetAttribute(input, _T("message"));
    msgOutput = p_wsdl.GetAttribute(output,_T("message"));
    SplitNamespace(msgInput);
    SplitNamespace(msgOutput);

    SOAPMessage msgIn (m_targetNamespace,msgInput);
    SOAPMessage msgOut(m_targetNamespace,msgOutput);

    if(ReadMessage(p_wsdl,msgIn) == false)
    {
      return false;
    }
    if(ReadMessage(p_wsdl,msgOut) == false)
    {
      return false;
    }
    if(AddOperation(++index,name,&msgIn,&msgOut) == false)
    {
      m_errormessage.Format(_T("Cannot add WS operation [%s] to the WSDL cache."),name.GetString());
      return false;
    }
    XString message;
    message.Format(_T("Read WSDL service: %s"), msgIn.GetSoapAction().GetString());
    DETAILLOG(message);

    // Find next operation
    operation = p_wsdl.GetElementSibling(operation);
  }

  // Check that we did read some operations
  if(m_operations.empty())
  {
    m_errormessage = _T("WSDL wany <portType> operations.");
    return false;
  }
  return true;
}

bool
WSDLCache::ReadMessage(XMLMessage& p_wsdl,SOAPMessage& p_message)
{
  // Find the correct message
  XString name = p_message.GetSoapAction();

  XMLElement* message = p_wsdl.FindElementWithAttribute(nullptr,_T("message"),_T("name"),name);
  if(!message)
  {
    m_errormessage.Format(_T("<message> with name=%s not found in the WSDL"),p_message.GetSoapAction().GetString());
    return false;
  }

  // Find the parameters element
  XMLElement* part = p_wsdl.FindElement(message,_T("part"));
  if(!part)
  {
    m_errormessage.Format(_T("Message [%s] without a part"),p_message.GetSoapAction().GetString());
    return false;
  }

  XMLAttribute* elem = p_wsdl.FindAttribute(part,_T("element"));
  if(!elem)
  {
    m_errormessage.Format(_T("Message [%s] without an element part"),p_message.GetSoapAction().GetString());
    return false;
  }

  XString parameter(elem->m_value);
  SplitNamespace(parameter);

  p_message.SetSoapAction(parameter);
  p_message.SetParameterObject(parameter);

  // Record the target name space in the object node
  XMLElement* param = p_message.GetParameterObjectNode();
  if(param)
  {
    param->SetNamespace(_T("tns"));
    p_message.SetAttribute(param,_T("tns"),m_targetNamespace);
  }

  return ReadParameters(p_wsdl,p_message,parameter);
}

bool
WSDLCache::ReadParameters(XMLMessage& p_wsdl,SOAPMessage& p_message,XString p_element)
{
  XMLElement* base = nullptr;

  XMLElement* element = ReadTypesElement(p_wsdl,p_element);
  if(element == nullptr)
  {
    m_errormessage.Format(_T("<type><element> with name=%s not found in WSDL"),p_element.GetString());
    return false;
  }
  XMLElement* complex = p_wsdl.FindElement(element,_T("complexType"));
  if(complex == nullptr)
  {
    XString type = p_wsdl.GetAttribute(element,_T("type"));
    if(!type.IsEmpty())
    {
      complex = ReadTypesElement(p_wsdl,type);
    }
    if(complex == nullptr)
    {
      m_errormessage.Format(_T("<type><element> with name=%s without 'complex' definition in WSDL"),p_element.GetString());
      return false;
    }
  }
  XMLElement* order = p_wsdl.GetElementFirstChild(complex);
  while(order && order->GetName() == _T("annotation"))
  {
    order = p_wsdl.GetElementSibling(order);
  }
  if(!order)
  {
    m_errormessage.Format(_T("<type><element> with name=%s without order node in WSDL"),p_element.GetString());
    return false;
  }

  // Setting the WSDL order
  WsdlOrder ord = WsdlOrder::WS_All;
       if(order->GetName() == _T("all"))      ord = WsdlOrder::WS_All;
  else if(order->GetName() == _T("choice"))   ord = WsdlOrder::WS_Choice;
  else if(order->GetName() == _T("sequence")) ord = WsdlOrder::WS_Sequence;
  else 
  {
    m_errormessage.Format(_T("<type><element> with name=%s with illegal order node in WSDL"),p_element.GetString());
    return false;
  }
  p_message.SetWSDLOrder(ord);

  // Read all elements within the order
  return ReadParametersInOrder(p_wsdl,p_message,base,order);
}

XMLElement* 
WSDLCache::ReadTypesType(XMLMessage& p_wsdl,XString p_element)
{
  // Earlier on already found
  TypeMap::iterator it = m_types.find(p_element);
  if(it != m_types.end())
  {
    return it->second;
  }

  // Findin the types node
  XMLElement* types = p_wsdl.FindElement(_T("types"));
  if(types == nullptr)
  {
    return nullptr;
  }

  // Search for complex type of this name (most common)
  XMLElement* complex = p_wsdl.FindElementWithAttribute(types,_T("complexType"),_T("name"),p_element);
  if(complex)
  {
    m_types.insert(std::make_pair(p_element,complex));
    return complex;
  }

  // Search for simple type (with optional restrictions)
  XMLElement* simple = p_wsdl.FindElementWithAttribute(types,_T("simpleType"),_T("name"),p_element);
  if(simple)
  {
    m_types.insert(std::make_pair(p_element,simple));
    return simple;
  }
  return nullptr;
}

XMLElement*
WSDLCache::ReadTypesElement(XMLMessage& p_wsdl,XString p_element)
{
  XMLElement* types = p_wsdl.FindElement(_T("types"));
  if(types == nullptr)
  {
    return nullptr;
  }

  // Find an element of this name
  XMLElement* elem = p_wsdl.FindElement(types,_T("element"));
  while(elem)
  {
    XMLAttribute* nameAtt = p_wsdl.FindAttribute(elem,_T("name"));
    if(nameAtt && nameAtt->m_value.Compare(p_element) == 0)
    {
      return elem;
    }
    // Next type
    elem = p_wsdl.GetElementSibling(elem);
  }
  return nullptr;
}

int
WSDLCache::ReadElementaryType(XString p_type)
{
  int index = 0;
  while(baseTypes[index].m_name)
  {
    if(p_type.Compare(baseTypes[index].m_name) == 0)
    {
      return baseTypes[index].m_type;
    }
    ++index;
  }
  return 0;
}

// Reading the node options for mandatory/optional and relations
// Beware: if not set, the options must default to "1" = Mandatory
int
WSDLCache::ReadWSDLOptions(XMLMessage& p_wsdl,XMLElement* p_element)
{
  int      options = WSDL_Mandatory;
  XString  minOccurs = p_wsdl.GetAttribute(p_element,_T("minOccurs"));
  XString  maxOccurs = p_wsdl.GetAttribute(p_element,_T("maxOccurs"));
  unsigned minNum = static_cast<unsigned>(_ttoi(minOccurs));

  if(!minOccurs.IsEmpty() && minNum == 0)
  {
    options = WSDL_Optional;
  }
  if(maxOccurs.Compare(_T("unbounded")) == 0)
  {
    options = (options == WSDL_Optional) ? WSDL_ZeroMany : WSDL_OneMany;
  }
  return options;
}

int 
WSDLCache::ReadWSDLOrdering(XMLElement* p_order)
{
  if(p_order->GetName() == _T("choice"))
  {
    return WSDL_Choice;
  }
  if(p_order->GetName() == _T("sequence"))
  {
    return WSDL_Sequence;
  }
  return 0;
}

bool
WSDLCache::ReadParametersInOrder(XMLMessage&  p_wsdl
                                ,SOAPMessage& p_message
                                ,XMLElement*  p_base
                                ,XMLElement*  p_order)
{
  int typeOptions = ReadWSDLOrdering(p_order);

  XMLElement* child = p_wsdl.GetElementFirstChild(p_order);
  while(child)
  {
    XString name = p_wsdl.GetAttribute(child,_T("name"));
    XString type = p_wsdl.GetAttribute(child,_T("type"));
    XString nspc = SplitNamespace(type);
    XString elName(name);
    XString nspcName;

    // Halfway a set of elements, we can encounter a new order
    if(child->GetName() == _T("all") || child->GetName() == _T("sequence") || child->GetName() == _T("choice"))
    {
      // all/sequence/choice -> read complex type 
      ReadParametersInOrder(p_wsdl,p_message,p_base,child);
      child = p_wsdl.GetElementSibling(child);
      continue;
    }

    if(!nspc.IsEmpty())
    {
      nspcName = p_wsdl.GetAttribute(child,nspc);
      elName = nspc + _T(":") + name;
    }

    // Get minOccurs, maxOccurs (0,1,unbounded), nullable
    int nodeOptions = ReadWSDLOptions(p_wsdl,child);
    int options = typeOptions + nodeOptions;

    int elemtype = ReadElementaryType(type);
    if(elemtype)
    {
      XMLElement* newElm = p_message.AddElement(p_base,elName,(ushort)(options + elemtype),type);
      if(!nspcName.IsEmpty())
      {
        XString atName = _T("xmlns:") + nspc;
        p_message.SetAttribute(newElm,atName,nspcName);
      }
    }
    else
    {
      XMLElement* newelem = p_message.AddElement(p_base,elName,(ushort)(XDT_Complex + options),_T(""));
      if(!nspcName.IsEmpty())
      {
        XString atName = _T("xmlns:") + nspc;
        p_message.SetAttribute(newelem,atName,nspcName);
      }

      XMLElement* newtype = nullptr;
      if(!type.IsEmpty())
      {
        newtype = ReadTypesType(p_wsdl,type);
      }
      else
      {
        newtype = child;
        XMLElement* detail = p_wsdl.GetElementFirstChild(newtype);
        if(detail->GetName() == _T("complexType") || detail->GetName() == _T("simpleType"))
        {
          newtype = detail;
        }
      }
      XMLElement* order = p_wsdl.GetElementFirstChild(newtype);
      if(order->GetName() == _T("restriction"))
      {
        ReadRestriction(p_wsdl,newelem,order,type,options);
      }
      else
      {
        while(order && (order->GetName() != _T("all") && order->GetName() != _T("sequence") && order->GetName() != _T("choice")))
        {
          // Skipping <annotation> / <documentation> etc
          order = p_wsdl.GetElementSibling(order);
        }
        if(order)
        {
          // all/sequence/choice -> read complex type 
          ReadParametersInOrder(p_wsdl,p_message,newelem,order);
        }
        else
        {
          // Still no complex type
          // Give up and roll over. Add as a string
          newelem->SetType((ushort) (XDT_String + options));
          newelem->SetValue(type);
        }
      }
    }
    typeOptions = 0;
    // Next element
    child = p_wsdl.GetElementSibling(child);
  }
  return true;
}

void
WSDLCache::ReadRestriction(XMLMessage& p_wsdl
                          ,XMLElement* p_newelem
                          ,XMLElement* p_restrict
                          ,XString     p_restriction
                          ,int         p_options)
{
  // Create the enumeration restriction
  XMLRestriction* restrict = m_restrictions.AddRestriction(p_restriction);
  p_newelem->SetRestriction(restrict);

  // Setting the base type
  XString baseType = p_wsdl.GetAttribute(p_restrict,_T("base"));
  if(!baseType.IsEmpty())
  {
    SplitNamespace(baseType);
    int elemBaseType = ReadElementaryType(baseType);
    p_newelem->SetType((ushort)(elemBaseType + p_options));
    p_newelem->SetValue(baseType);

    restrict->AddBaseType(baseType);
  }

  XMLElement* child = p_wsdl.GetElementFirstChild(p_restrict);
  while(child)
  {
    XString value = p_wsdl.GetAttribute(child,_T("value"));

    if(child->GetName() == _T("enumeration"))
    {
      // Reading an enumeration
      XString docum = ReadAnnoDocumentation(p_wsdl,child);
      restrict->AddEnumeration(value,docum);
    }
    else if(child->GetName() == _T("length"))         restrict->AddLength(_ttoi(value));
    else if(child->GetName() == _T("maxLength"))      restrict->AddMaxLength(_ttoi(value));
    else if(child->GetName() == _T("minLength"))      restrict->AddMinLength(_ttoi(value));
    else if(child->GetName() == _T("totalDigits"))    restrict->AddTotalDigits(_ttoi(value));
    else if(child->GetName() == _T("fractionDigits")) restrict->AddFractionDigits(_ttoi(value));
    else if(child->GetName() == _T("pattern"))        restrict->AddPattern(value);
    else if(child->GetName() == _T("maxExclusive"))   restrict->AddMaxExclusive(value);
    else if(child->GetName() == _T("maxInclusive"))   restrict->AddMaxInclusive(value);
    else if(child->GetName() == _T("minExclusive"))   restrict->AddMinExclusive(value);
    else if(child->GetName() == _T("minInclusive"))   restrict->AddMinInclusive(value);
    else if(child->GetName() == _T("whiteSpace"))     restrict->AddWhitespace(ReadWhiteSpace(value));

    // Next child restriction
    child = p_wsdl.GetElementSibling(child);
  }
}

int
WSDLCache::ReadWhiteSpace(XString p_value)
{
  if(p_value.CompareNoCase(_T("preserve")) == 0) return 1;
  if(p_value.CompareNoCase(_T("replace"))  == 0) return 2;
  if(p_value.CompareNoCase(_T("collapse")) == 0) return 3;
  return 0;
}

// Read <annotation><documentation> of a enumeration
XString
WSDLCache::ReadAnnoDocumentation(XMLMessage& p_wsdl,XMLElement* enumeration)
{
  XString documentation;
  XMLElement* annotation = p_wsdl.FindElement(enumeration,_T("annotation"));
  if(annotation)
  {
    XMLElement* docu = p_wsdl.FindElement(annotation,_T("documentation"));
    if(docu)
    {
      return docu->GetValue();
    }
  }
  return documentation;
}
