/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WSDLCache.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2016 ir. W.E. Huisman
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
#include "EnsureFile.h"
#include "Namespace.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Do not warn about formatting CStrings
#pragma warning(disable:6284)

#ifdef CRASHLOG
#undef CRASHLOG
#endif

// Logging to the logfile
#define DETAILLOG(str)        if(m_logging) m_logging->AnalysisLog(__FUNCTION__,LogType::LOG_INFO,false,str)
#define CRASHLOG(code,text)   if(m_logging)\
                              {\
                                m_logging->AnalysisLog(__FUNCTION__,LogType::LOG_ERROR,false,text); \
                                m_logging->AnalysisLog(__FUNCTION__,LogType::LOG_ERROR,true,"Error code: %d",code); \
                              }


// CTOR for the WSDL cache
WSDLCache::WSDLCache(bool p_server)
          :m_server(p_server)
{
  // Not seen anywhere else: The service postfix is 'active server pages for C ++'
  m_servicePostfix = ".acx";
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
WSDLCache::SetService(CString p_servicename,CString p_url)
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
      m_filename = m_webroot + "\\" + cracked.m_path + "\\" + m_serviceName + ".wsdl";
      m_filename.Replace('/','\\');
      m_filename.Replace("\\\\","\\");
    }
    return true;
  }
  return false;
}

// Add SOAP message call and answer
// Call exactly once for every SOAPMessage combination
bool 
WSDLCache::AddOperation(int p_code,CString p_name,SOAPMessage* p_input,SOAPMessage* p_output)
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
WSDLCache::GetCommandCode(CString& p_commandName)
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
  FILE* file = nullptr;
  EnsureFile ensfile(m_filename);
  ensfile.OpenFile(&file,"w");
  if(file)
  {
    CString wsdlcontent;

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
    fprintf(file,wsdlcontent);
    if(fclose(file))
    {
      return false;
    }
    CString generated;
    generated.Format("WSDL File for service [%s] generated in: %s",m_serviceName,m_filename);
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
WSDLCache::GenerateTypes(CString& p_wsdlcontent)
{
  CString  temp;
  TypeDone done;

  p_wsdlcontent += "\n<!-- Abstract types -->\n";

  // Generate header
  temp.Format("<wsdl:types>\n"
              "  <s:schema elementFormDefault=\"qualified\" targetNamespace=\"%s\">\n",m_targetNamespace);
  p_wsdlcontent += temp;

  OperationMap::iterator it;
  for(it = m_operations.begin();it != m_operations.end(); ++it)
  {
    GenerateMessageTypes(p_wsdlcontent,it->second.m_input, done);
    GenerateMessageTypes(p_wsdlcontent,it->second.m_output,done);
  }
  // Footer types
  p_wsdlcontent += "  </s:schema>\n"
                   "</wsdl:types>\n";
}

// Generate types for one message
void
WSDLCache::GenerateMessageTypes(CString&      p_wsdlcontent
                               ,SOAPMessage*  p_msg
                               ,TypeDone&     p_gedaan)
{
  CString command = p_msg->GetSoapAction();

  // Put the parameters here
  GenerateParameterTypes(p_wsdlcontent
                        ,p_msg->GetSoapAction()
                        ,p_msg->m_paramObject->m_elements
                        ,p_gedaan
                        ,p_msg->GetWSDLOrder()
                        ,0);
}

// Generate types for all parameters
void
WSDLCache::GenerateParameterTypes(CString&       p_wsdlcontent
                                 ,CString        p_element
                                 ,XmlElementMap& p_map
                                 ,TypeDone&      p_done
                                 ,WsdlOrder      p_order
                                 ,int            p_level)
{
  CString temp;

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
    p_wsdlcontent.AppendFormat("    <s:element name=\"%s\">\n",p_element);
  }

  // Get the ordering
  CString order;
  switch(p_order)
  {
    default:                     // Fall through
    case WsdlOrder::WS_All:      order = "all";      break;
    case WsdlOrder::WS_Choice:   order = "choice";   break;
    case WsdlOrder::WS_Sequence: order = "sequence"; break;
  }

  // Header of the complex type
  if(p_level)
  {
    p_wsdlcontent.AppendFormat("    <s:complexType name=\"%s\">\n",p_element);
  }
  else
  {
    p_wsdlcontent.AppendFormat("    <s:complexType>\n");
  }
  p_wsdlcontent.AppendFormat("      <s:%s>\n",order);  // all, choice or sequence


  for(unsigned ind = 0;ind < p_map.size(); ++ind)
  {
    CString array;
    XMLElement* param = p_map[ind];

    // Begin element
    p_wsdlcontent += "        <s:element ";

    // Do occurrence type
    switch(param->m_type & WSDL_Mask)
    {
      default:             // Fall through
      case WSDL_ZeroOne:   // Fall through
      case WSDL_Optional:  temp  = "minOccurs=\"0\" maxOccurs=\"1\""; break;
      case WSDL_OnceOnly:  // Fall through
      case WSDL_Mandatory: temp  = "minOccurs=\"1\" maxOccurs=\"1\""; break;
      case WSDL_ZeroMany:  temp  = "minOccurs=\"0\" maxOccurs=\"unbounded\""; 
                           array = "ArrayOf";
                           break;
      case WSDL_OneMany:   temp  = "minOccurs=\"1\" maxOccurs=\"unbounded\""; 
                           array = "ArrayOf";
                           break;
    }
    p_wsdlcontent += temp;

    // Do name
    temp.Format(" name=\"%s\"",param->m_name);
    p_wsdlcontent += temp;

    // Do data type
    switch(param->m_type & XDT_Mask)
    {
      case XDT_CDATA:   // Fall through
      case (XDT_String|XDT_CDATA): // Fall through
      case XDT_String:  temp = " type=\"s:string\"";       break;
      case XDT_Integer: temp = " type=\"s:int\"";          break;
      case XDT_Double:  temp = " type=\"s:double\"";       break;
      case XDT_Boolean: temp = " type=\"s:boolean\"";      break;
      case XDT_Base64:  temp = " type=\"s:base64Binary\""; break;
      case XDT_DateTime:temp = " type=\"s:dateTime\"";     break;
      case XDT_Complex: temp.Format(" type=\"tns:ArrayOf%s\"",param->m_name);
                        break;
      default:          temp = " type=\"s:string\"";       
                        break;
    }
    p_wsdlcontent += temp;

    // End of element
    p_wsdlcontent += " />\n";
  }

  // Footer complex type sequence
  p_wsdlcontent.AppendFormat("      </s:%s>\n"
                             "    </s:complexType>\n",order);
  if(p_level == 0)
  {
    p_wsdlcontent += "    </s:element>\n";
  }

  // This type has been done. Do not do it again
  p_done.insert(std::make_pair(p_element,1));


  // Try recursion for complex type, or simply the added type definition
  for(unsigned ind = 0;ind < p_map.size(); ++ind)
  {
    XMLElement* param = p_map[ind];

    if(param->m_elements.size())
    {
      // Recurse WITHOUT postfix and target namespace
      CString name;
      if((param->m_type & WSDL_Mask) == WSDL_OneMany  ||
         (param->m_type & WSDL_Mask) == WSDL_ZeroMany ||
         (param->m_type & XDT_Mask)  == XDT_Complex   )
      {
        name += "ArrayOf";
      }
      name += param->m_name;
      GenerateParameterTypes(p_wsdlcontent,name,param->m_elements,p_done,p_order,p_level + 1);
    }
  }
}

// Generate messages
void
WSDLCache::GenerateMessages(CString& p_wsdlcontent)
{
  CString temp;

  p_wsdlcontent += "\n<!-- Abstract messages -->\n";

  OperationMap::iterator it;
  for(it = m_operations.begin();it != m_operations.end(); ++it)
  {
    // Input message
    temp.Format("<wsdl:message name=\"%sSoapIn\">\n",it->first);
    p_wsdlcontent += temp;
    temp.Format("  <wsdl:part name=\"parameters\" element=\"tns:%s\" />\n",it->second.m_input->GetSoapAction());
    p_wsdlcontent += temp;
    p_wsdlcontent += "</wsdl:message>\n";

    // Output message
    temp.Format("<wsdl:message name=\"%sSoapOut\">\n",it->first);
    p_wsdlcontent += temp;
    temp.Format("  <wsdl:part name=\"parameters\" element=\"tns:%s\" />\n",it->second.m_output->GetSoapAction());
    p_wsdlcontent += temp;
    p_wsdlcontent += "</wsdl:message>\n";
  }
}

// Generate porttype
void
WSDLCache::GeneratePortType(CString& p_wsdlcontent)
{
  CString temp;

  p_wsdlcontent += "\n<!-- Abstract porttypes -->\n";

  // Generate port type header
  temp.Format("<wsdl:portType name=\"%s\">\n",m_serviceName);
  p_wsdlcontent += temp;

  // Generate port type of messages
  OperationMap::iterator it;
  for(it = m_operations.begin();it != m_operations.end(); ++it)
  {
    // Operation header;
    temp.Format("  <wsdl:operation name=\"%s\">\n",it->first);
    p_wsdlcontent += temp;

    // input
    temp.Format("    <wsdl:input message=\"tns:%sSoapIn\" />\n",it->first);
    p_wsdlcontent += temp;

    // output
    temp.Format("    <wsdl:output message=\"tns:%sSoapOut\" />\n",it->first);
    p_wsdlcontent += temp;

    // operation footer
    p_wsdlcontent += "  </wsdl:operation>\n";
  }

  // Generate port type footer
  p_wsdlcontent += "</wsdl:portType>\n";
}

// Generate bindings
void
WSDLCache::GenerateBindings(CString& p_wsdlcontent)
{
  p_wsdlcontent += "\n<!-- Concrete binding with SOAP -->\n";

  if(m_performSoap10)
  {
    GenerateBinding(p_wsdlcontent,"Soap","soap");
  }
  if(m_performSoap12)
  {
    GenerateBinding(p_wsdlcontent,"Soap12","soap12");
  }
}

// Generate detailed binding
void
WSDLCache::GenerateBinding(CString& p_wsdlcontent,CString p_binding,CString p_soapNamespace)
{
  CString temp;

  // Generate binding header
  temp.Format("<wsdl:binding name=\"%s%s\" type=\"tns:%s\">\n",m_serviceName,p_binding,m_serviceName);
  p_wsdlcontent += temp;
  // Generate soap transport binding
  temp.Format("  <%s:binding transport=\"http://schemas.xmlsoap.org/soap/http/\" />\n",p_soapNamespace);
  p_wsdlcontent += temp;

  // Generate all operations
  OperationMap::iterator it;
  for(it = m_operations.begin();it != m_operations.end(); ++it)
  {
    // Operation
    temp.Format("  <wsdl:operation name=\"%s\">\n",it->first);
    p_wsdlcontent += temp;

    // Soap Operation
    temp.Format("    <%s:operation soapAction=\"%s/%s\" style=\"document\" />\n",p_soapNamespace,m_targetNamespace,it->first);
    p_wsdlcontent += temp;
    // literal input/output
    temp.Format("    <wsdl:input><%s:body use=\"literal\" /></wsdl:input>\n",p_soapNamespace);
    p_wsdlcontent += temp;
    temp.Format("    <wsdl:output><%s:body use=\"literal\" /></wsdl:output>\n",p_soapNamespace);
    p_wsdlcontent += temp;

    // End of operation
    p_wsdlcontent += "  </wsdl:operation>\n";
  }

  // Generate binding footer
  p_wsdlcontent += "</wsdl:binding>\n";
}

// Generate service bindings
void
WSDLCache::GenerateServiceBindings(CString& p_wsdlcontent)
{
  CString temp;
  CString path;
  path.Format("%s%s%s%s", m_url, m_url.GetAt(m_url.GetLength() - 1) == '/' ? "" : "/", m_serviceName, m_servicePostfix);
                         
  p_wsdlcontent += "\n<!-- Web service offering endpoints for bindings -->\n";

  // Begin envelope
  temp.Format("<wsdl:service name=\"%s\">\n",m_serviceName);
  p_wsdlcontent += temp;

  if(m_performSoap10)
  {
    temp.Format("  <wsdl:port name=\"%sSoap\" binding=\"tns:%sSoap\">\n",m_serviceName,m_serviceName);
    p_wsdlcontent += temp;
    temp.Format("    <soap:address location=\"%s\" />\n",path);
    p_wsdlcontent += temp;
    p_wsdlcontent += "  </wsdl:port>\n";
  }
  if(m_performSoap12)
  {
    temp.Format("  <wsdl:port name=\"%sSoap12\" binding=\"tns:%sSoap12\">\n",m_serviceName,m_serviceName);
    p_wsdlcontent += temp;
    temp.Format("    <soap12:address location=\"%s\" />\n",path);
    p_wsdlcontent += temp;
    p_wsdlcontent += "  </wsdl:port>\n";
  }
  // Generate envelope end
  p_wsdlcontent += "</wsdl:service>\n";
}

// Generate definitions envelope
// This is a WSDL 1.1 definition!!
// Microsoft C# .NET cannot handle any higher definitions!!
// And we do not want to lose compatibility with that platform
void
WSDLCache::GenerateDefinitions(CString& p_wsdlcontent)
{
  CString header("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");

  CString def = "<wsdl:definitions \n";
  def += "  xmlns:soap=\"http://schemas.xmlsoap.org/wsdl/soap/\"\n";
  def += "  xmlns:soap12=\"http://schemas.xmlsoap.org/wsdl/soap12/\"\n";
  def += "  xmlns:tm=\"http://microsoft.com/wsdl/mime/textMatching/\"\n";
  def += "  xmlns:soapenc=\"http://schemas.xmlsoap.org/soap/encoding/\"\n";
  def += "  xmlns:mime=\"http://schemas.xmlsoap.org/wsdl/mime/\"\n";
  def += "  xmlns:s=\"http://www.w3.org/2001/XMLSchema\"\n";
  def += "  xmlns:http=\"http://schemas.xmlsoap.org/wsdl/http/\"\n";
  def += "  xmlns:wsdl=\"http://schemas.xmlsoap.org/wsdl/\"\n";

  // BEWARE: In .NET 2.0 it used to be:
  // def += "  xmlns:http=\"http://schemas.xmlsoap.org/soap/http/\"\n";

  // Adding the real target namespace
  CString temp;
  temp.Format("  xmlns:tns=\"%s\"\n",m_targetNamespace);
  def += temp;
  temp.Format("  targetNamespace=\"%s\">\n",m_targetNamespace);
  def += temp;

  p_wsdlcontent = header + def + p_wsdlcontent + "</wsdl:definitions>\n";
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
  CString name = p_msg->GetSoapAction();
  OperationMap::iterator it = m_operations.find(name);

  if(it != m_operations.end())
  {
    return CheckMessage(it->second.m_input,p_msg,"Client",p_checkFields);
  }
  // No valid operation found
  p_msg->Reset();
  p_msg->SetFault("No operation","Client","No operation [" + p_msg->GetSoapAction() + "] found","While testing against WSDL");
  return false;
}

// Check outgoing SOAP message against WSDL
bool
WSDLCache::CheckOutgoingMessage(SOAPMessage* p_msg,bool p_checkFields)
{
  CString name = p_msg->GetSoapAction();
  OperationMap::iterator it;

  // Check if we are already in error state
  // So we can send already generated errors.
  if(p_msg->GetErrorState())
  {
    return true;
  }

  // See if it is an registered operation in this WSDL
  for(it = m_operations.begin();it != m_operations.end(); ++it)
  {
    if(it->second.m_output->GetSoapAction().Compare(name) == 0)
    {
      return CheckMessage(it->second.m_output,p_msg,"Server",p_checkFields);
    }
  }
  // No valid operation found
  p_msg->Reset();
  p_msg->SetFault("No operation","Server","No operation [" + p_msg->GetSoapAction() + "] found","While testing against WSDL");
  return false;
}

// Check message
bool
WSDLCache::CheckMessage(SOAPMessage* p_orig,SOAPMessage* p_tocheck,CString p_who,bool p_checkFields)
{
  if(p_orig == p_tocheck)
  {
    p_tocheck->Reset();
    p_tocheck->SetFault("No reuse",p_who,"Server cannot reuse registration messages","While testing against WSDL");
    return false;
  }
  // Check parameter object
  if((p_orig->GetParameterObject() != p_tocheck->GetParameterObject()) &&
     !p_orig->GetParameterObject().IsEmpty() &&
     !p_tocheck->GetParameterObject().IsEmpty())
  {
    CString msg;
    msg.Format("Request/Response object not the same. Expected '%s' got '%s'"
              ,p_orig->GetParameterObject()
              ,p_tocheck->GetParameterObject());

    p_tocheck->Reset();
    p_tocheck->SetFault("Request/Response object",p_who,msg,"While testing against WSDL");
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
                          ,CString      p_who
                          ,bool         p_fields)
{
  XmlDataType type = 0;
  XMLElement* orgParam   = p_orig->GetElementFirstChild(p_orgBase);
  XMLElement* checkParam = p_check->GetElementFirstChild(p_checkBase);
  while(orgParam)
  {
    // Name of node to find in the message to check
    CString orgName = orgParam->GetName();
    type = orgParam->GetType();
    // If the ordering is choice, instead of sequence: do a free search
    if(type & WSDL_Choice)
    {
      checkParam = p_check->FindElement(p_checkBase,orgName,false);
    }
    CString chkName = checkParam ? checkParam->GetName() : "";

    // DO CHECKS

    // Parameter is mandatory but not given in the definition
    if((orgName != chkName) || ((type & WSDL_Mandatory) && checkParam == nullptr))
    {
      p_check->Reset();
      p_check->SetFault("Mandatory field not found",p_who,"Message is missing a field",orgParam->GetName());
      return false;
    }

    // Only if parameter field found
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
      if(orgParam->m_elements.size())
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
        continue;
      }
    }

    // Next parameter in the template
    orgParam   = p_orig ->GetElementSibling(orgParam);
    checkParam = p_check->GetElementSibling(checkParam);
    type       = orgParam ? orgParam->GetType() : 0;
  }
  // Gotten to the end, it's OK
  return true;
}

// Check data field in depth
bool
WSDLCache::CheckFieldDatatypeValues(XMLElement*   p_origParam
                                   ,XMLElement*   p_checkParam
                                   ,SOAPMessage*  p_check
                                   ,CString       p_who)
{
  // Do datatype check
  CString result = XMLRestriction::CheckDatatype(p_origParam->GetType(),p_checkParam->GetValue());
  if(!result.IsEmpty())
  {
    CString details("Datatype check failed: ");
    details += result;

    p_check->Reset();
    p_check->SetFault("Datatype",p_who,p_checkParam->GetName(),details);
    return false;
  }

  // Variable XSD Restriction check
  if(p_checkParam->GetRestriction())
  {
    result = p_checkParam->GetRestriction()->CheckRestriction(p_origParam->GetType() & XDT_Mask,p_origParam->GetValue());
    if(!result.IsEmpty())
    {
      p_check->Reset();
      p_check->SetFault("Fieldvalue",p_who,"Restriction",result);
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
CString
WSDLCache::GetServiceBasePageName()
{
  return m_absPath + m_serviceName + m_servicePostfix; 
}

// Get service page for HTTP GET service
CString 
WSDLCache::GetServicePage()
{
  CString page = GetPageHeader();

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
CString 
WSDLCache::GetOperationPage(CString p_operation,CString p_hostname)
{
  CString temp;
  CString page;

  OperationMap::iterator it = m_operations.find(p_operation);

  if (it == m_operations.end())
  {
    return page;
  }
  WsdlOperation& operation = it->second;
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

CString
WSDLCache::GetPageHeader()
{
  CString header;
  header.Format("<html>\n"
                 "  <head>\n"
                 "    <style type=\"text/css\">"
                 "      BODY { color: #000000; background-color: white; font-family: Verdana; margin-left: 0px; margin-top: 0px; }\n"
                 "      #content { margin-left: 30px; font-size: .70em; padding-bottom: 2em; }\n"
                 "      A:link { color: #336699; font-weight: bold; text-decoration: underline; }\n"
                 "      A:visited { color: #6699cc; font-weight: bold; text-decoration: underline; }\n"
                 "      A:active { color: #336699; font-weight: bold; text-decoration: underline; }\n"
                 "      A:hover { color: cc3300; font-weight: bold; text-decoration: underline; }\n"
                 "      P { color: #000000; margin-top: 0px; margin-bottom: 12px; font-family: Verdana; }\n"
                 "      pre { background-color: #e5e5cc; padding: 5px; font-family: Courier New; font-size: 1.2em; margin-top: -5px; border: 1px #f0f0e0 solid; }\n"
                 "      td { color: #000000; font-family: Verdana; font-size: .7em; }\n"
                 "      h2 { font-size: 1.5em; font-weight: bold; margin-top: 25px; margin-bottom: 10px; border-top: 1px solid #003366; margin-left: -15px; color: #003366; }\n"
                 "      h3 { font-size: 1.1em; color: #000000; margin-left: -15px; margin-top: 10px; margin-bottom: 10px; }\n"
                 "      ul { margin-top: 10px; margin-left: 20px; }\n"
                 "      ol { margin-top: 10px; margin-left: 20px; }\n"
                 "      li { margin-top: 10px; color: #000000; }\n"
                 "      font.value { color: darkblue; font: bold; }\n"
                 "      font.key { color: darkgreen; font: bold; }\n"
                 "      font.error { color: darkred; font: bold; }\n"
                 "      .heading1 { color: #ffffff; font-family: Tahoma; font-size: 26px; font-weight: normal; background-color: #003366; margin-top: 0px; margin-bottom: 0px; margin-left: -30px; padding-top: 10px; padding-bottom: 3px; padding-left: 15px; width: 105%%; }\n"
                 "      .button { background-color: #dcdcdc; font-family: Verdana; font-size: 1em; border-top: #cccccc 1px solid; border-bottom: #666666 1px solid; border-left: #cccccc 1px solid; border-right: #666666 1px solid; }\n"
                 "      .frmheader { color: #000000; background: #dcdcdc; font-family: Verdana; font-size: .7em; font-weight: normal; border-bottom: 1px solid #dcdcdc; padding-top: 2px; padding-bottom: 2px; }\n"
                 "      .frmtext { font-family: Verdana; font-size: .7em; margin-top: 8px; margin-bottom: 0px; margin-left: 32px; }\n"
                 "      .frmInput { font-family: Verdana; font-size: 1em; }\n"
                 "      .intro { margin-left: -15px; }\n"
                 "    </style>\n"
                 "    <title>%s Web Service</title>\n"
                 "  </head>\n"
                 "  <body>\n"
                 "    <div id=\"content\">\n"
                 "    <p class=\"heading1\">%s</p><br>\n"
                ,m_serviceName
                ,m_serviceName);

  return header;
}

CString
WSDLCache::GetPageFooter()
{
  CString footer;
  footer = "  </body>\n"
            "</html>\n";

  return footer;
}

CString
WSDLCache::GetOperationWsdl()
{
  CString wsdl("<br>\n");
  CString filename = m_url + m_serviceName + m_servicePostfix;

  wsdl += "    <p class=\"intro\">This is the overview of the service.</p>";
  wsdl += "    <p class=\"intro\">To use this service you can create a client for it on the basis of the WSDL file below.</p>";
  
  wsdl.AppendFormat("    <pre><a href=\"%s?wsdl\">%s?wsdl</a></pre>",filename,filename);

  wsdl += "    <br>\n";
  wsdl += "    <h2>These are all the separate SOAP messages:</h2>\n";
  return wsdl;
}

CString
WSDLCache::GetOperationNameLink(CString p_operation)
{
  CString link;
  CString linkpage;
  linkpage = m_serviceName + "." + p_operation + ".html";

  link.Format("    <h3><a href=\"%s\">%s</a></h3>\n",linkpage,p_operation);
  return link;
}


CString
WSDLCache::GetOperationPageIntro(CString p_operation)
{
  CString intro;

  intro.Format("    <span>\n"
                "      <p class=\"intro\">Click <a href=\"%s%s\">here</a> for a complete list of operations.</p>\n"
                "      <h2>%s</h2>\n"
                "      <p class=\"intro\"></p>\n"
               ,m_serviceName
               ,m_servicePostfix
               ,p_operation);

  return intro;
}

CString
WSDLCache::GetOperationPageHttpI(CString p_operation,CString p_hostname,bool p_soapVersion)
{
  CString text; 
  text.Format("      <span>\n"
              "        <h3>SOAP %s</h3>\n"
              "        <p>The following is a sample SOAP %s request and response.  The <font class=value>placeholders</font> shown need to be replaced with actual values.</p>\n"
              "<pre>POST %s/%s%s HTTP/1.1\n"
              "Host: %s\n"
              "Content-Type: %s; charset=utf-8\n"
              "Content-Length: <font class=value>length</font>\n"
              ,p_soapVersion ? "1.2" : "1.1"
              ,p_soapVersion ? "1.2" : "1.1"
              ,m_absPath
              ,m_serviceName
              ,m_servicePostfix
              ,p_hostname
              ,p_soapVersion ? "application/soap+xml" : "text/xml");
  if(p_soapVersion == false)
  {
    CString extra;
    extra.Format("SOAPAction: \"%s/%s\"\n",m_targetNamespace,p_operation);
    text += extra;
  }
  text += "\n";

  return text;
}

CString
WSDLCache::GetOperationPageHttpO(bool p_soapVersion)
{
  CString text;
  text.Format("<pre>HTTP/1.1 200 OK\n"
              "Content-Type: %s; charset=utf-8\n"
              "Content-Length: <font class=value>length</font>\n\n"
              ,p_soapVersion ? "application/soap+xml" : "text/xml");
  return text;
}

CString
WSDLCache::GetOperationPageSoap(SOAPMessage* p_msg,bool p_soapVersion)
{
  // Set the SOAP Version
  p_msg->SetSoapVersion(p_soapVersion ? SoapVersion::SOAP_12 : SoapVersion::SOAP_11);
  p_msg->SetPrintRestrictions(true);

  CString message = p_msg->GetSoapMessage();
  message.Replace("<","&lt;");
  message.Replace(">","&gt;");

  // Datatypes replacements
  message.Replace("&gt;int&lt;",          "&gt;<font class=value>int</font>&lt;");
  message.Replace("&gt;string&lt;",       "&gt;<font class=value>string</font>&lt;");
  message.Replace("&gt;double&lt;",       "&gt;<font class=value>double</font>&lt;");
  message.Replace("&gt;boolean&lt;",      "&gt;<font class=value>boolean</font>&lt;");
  message.Replace("&gt;dateTime&lt;",     "&gt;<font class=value>dateTime</font>&lt;");
  message.Replace("&gt;base64Binary&lt;", "&gt;<font class=value>base64Binary</font>&lt;");

  // Ending of pre-formatted section
  message += "</pre>\n";

  return message;
}

CString
WSDLCache::GetOperationPageFooter()
{
  return "    </span>\n";
}


//////////////////////////////////////////////////////////////////////////
//
// READING THE WSDL CACHE FROM A WSDL FILE
//
//////////////////////////////////////////////////////////////////////////

struct _baseType
{
  char* m_name;
  int   m_type;
}
baseTypes[] =
{
  { "string",        XDT_String   }
 ,{ "char",          XDT_String   }
 ,{ "boolean",       XDT_Boolean  }
 ,{ "int",           XDT_Integer  }
 ,{ "long",          XDT_Integer  }
 ,{ "double",        XDT_Double   }
 ,{ "float",         XDT_Double   }
 ,{ "decimal",       XDT_Double   }
 ,{ "dateTime",      XDT_DateTime }
 ,{ "QName",         XDT_String   }
 ,{ "short",         XDT_Integer  }
 ,{ "base64Binary",  XDT_Base64   }  
 ,{ "anyURI",        XDT_String   }
 ,{ "anyType",       XDT_String   }
 ,{ "unsignedByte",  XDT_Integer  }
 ,{ "unsignedInt",   XDT_Integer  }
 ,{ "unsignedShort", XDT_Integer  }
 ,{ "unsignedLong",  XDT_Integer  }
 ,{ "guid",          XDT_String   }
 ,{ NULL,            0            }
};

// Reading a WSDL file is protected by a SEH handler
// Users could provide a WSDL definition with circular references
// causing the reading functions to overrun the stackspace
bool
WSDLCache::ReadWSDLFile(LPCTSTR p_filename)
{
  __try
  {
    return ReadWSDLFileSafe(p_filename);
  }
  __except(
#ifdef _DEBUG
    // See if we are live debugging in Visual Studio
    // IsDebuggerPresent() ? EXCEPTION_CONTINUE_SEARCH :
#endif // _DEBUG
    // After calling the Error::Send method, the Windows stack get unwinded
    // We need to detect the fact that a second exception can occur,
    // so we do **not** call the error report method again
    // Otherwise we would end into an infinite loop
    m_exception ? EXCEPTION_EXECUTE_HANDLER :
    m_exception = true,
    m_exception = ErrorReport::Report(GetExceptionCode(),GetExceptionInformation()),
    EXCEPTION_EXECUTE_HANDLER)
  {
    if(m_exception)
    {
      // Error while sending an error report
      // This error can originate from another thread, OR from the sending of this error report
      CRASHLOG(0xFFFF,"DOUBLE INTERNAL ERROR while making an error report.!!");
      m_exception = false;
    }
    else
    {
      CRASHLOG(WER_S_REPORT_UPLOADED,"CRASH: Errorreport while reading WSDL has been made");
    }
  }
  return false;
}

bool
WSDLCache::ReadWSDLFileSafe(LPCTSTR p_filename)
{
  bool result = false;

  CString filename(p_filename);
  CString protocol1 = filename.Left(8);
  CString protocol2 = filename.Left(7);
  protocol1.MakeLower();
  protocol2.MakeLower();
  if(protocol1 == "https://" || protocol2 == "http://")
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
  return true;
}

bool
WSDLCache::ReadWSDLFileFromURL(CString p_url)
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
      CString message((LPCTSTR)contents);
      XMLMessage wsdl;
      wsdl.ParseMessage(message);
      if(wsdl.GetInternalError() == XmlError::XE_NoError)
      {
        result = ReadWSDL(wsdl);
      }
    }
    delete [] contents;
  }
  return result;
}

bool
WSDLCache::ReadWSDLLocalFile(CString p_filename)
{
  CString message;
  message.Format("Reading WSDL file: %s",p_filename);
  DETAILLOG(message);

  // Reading the WSDL as a XML message
  XMLMessage wsdl;
  if(wsdl.LoadFile(p_filename) == false)
  {
    DETAILLOG("Abort reading WSDL file. Cannot load XML document");
    return false;
  }
  return ReadWSDL(wsdl);
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
    DETAILLOG("Abort reading WSDL file. Cannot load definition part");
    DETAILLOG(m_errormessage);
    return false;
  }
  // Step 2: Read Service bindings
  if(ReadServiceBindings(p_wsdl) == false)
  {
    Reset();
    DETAILLOG("Abort reading WSDL file. Cannot load service bindings");
    DETAILLOG(m_errormessage);
    return false;
  }

  // Step 3: Read bindings
  if(ReadBindings(p_wsdl) == false)
  {
    Reset();
    return false;
  }

  // Step 4: Read porttypes / messages / types
  if(ReadPortTypes(p_wsdl) == false)
  {
    Reset();
    DETAILLOG("Abort reading WSDL file. Cannot load port types and messages");
    DETAILLOG(m_errormessage);
    return false;
  }
  // Clear the temporary cache for the types
  m_types.clear();

  DETAILLOG("WSDL Read in completely");
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

  // Check rootname (= "definitions" in namespace "wsdl" / attrib xmlns:wsdl= ... etc
  if((root->GetName().Compare("definitions")) ||
     (root->GetNamespace().Compare("wsdl")))
  {
    // Not a WSDL defintion
    m_errormessage = "Not a WSDL file beginning with <wsdl:definitions...>";
    return false;
  }

  // Check attribute "targetNamespace" -> m_targetnamespace
  XMLAttribute* target = p_wsdl.FindAttribute(root,"targetNamespace");
  if(target && target->m_namespace.IsEmpty())
  {
    m_targetNamespace = target->m_value;
  }
  else
  {
    m_errormessage = "No targetNamespace found in the WSDL";
    return false;
  }

  // Check attribute soap
  XMLAttribute* soap = p_wsdl.FindAttribute(root,"soap");
  m_performSoap10 = (soap && soap->m_namespace.Compare("xmlns") == 0);
  
  // Check attribute soap12
  XMLAttribute* soap12 = p_wsdl.FindAttribute(root,"soap12");
  m_performSoap12 = (soap12 && soap12->m_namespace.Compare("xmlns") == 0);

  // optional "xmlns:tns"
  XMLAttribute* tns = p_wsdl.FindAttribute(root,"tns");
  if(tns)
  {
    if(tns->m_namespace.Compare("xmlns") == 0 && tns->m_value.CompareNoCase(target->m_value))
    {
      m_errormessage = "Namespaces 'tns' and 'targetNamespace' are not the same!";
      return false;
    }
  }
  return true;
}

bool
WSDLCache::ReadServiceBindings(XMLMessage& p_wsdl)
{
  // "service" is a first level node in the root of the WSDL
  XMLElement* service = p_wsdl.FindElement("service",false);
  if(!service)
  {
    m_errormessage = "No <wsdl:service> within the WSDL.";
    return false;
  }

  XMLAttribute* name = p_wsdl.FindAttribute(service,"name");
  if(name)
  {
    m_serviceName = name->m_value;
  }
  else
  {
    m_errormessage = "No service name attribute in <wsdl:service>";
    return false;
  }

  XMLElement* address10 = p_wsdl.FindElement(service,"soap:address");
  XMLElement* address12 = p_wsdl.FindElement(service,"soap12:address");

  if(address10 && !m_performSoap10)
  {
    m_errormessage = "SOAP 1.0 service binding found, but no SOAP 1.0 namespace.";
    return false;
  }

  if(address12 && !m_performSoap12)
  {
    m_errormessage = "SOAP 1.2 service binding found, but no SOAP 1.2 namespace.";
    return false;
  }

  // Use highest soap version
  XMLElement* address = address12 ? address12 : address10;
  XMLAttribute* location = p_wsdl.FindAttribute(address,"location");
  if(!location)
  {
    m_errormessage = "Missing <location> in <wsdl:service>";
    return false;
  }
  CString loc = location->m_value;
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
    CString locname = loc.Mid(posName + 1);
    if(locname.CompareNoCase(m_serviceName))
    {
      m_errormessage = "Address location binding not for service name.";
      return false;
    }
    m_url = loc.Left(posName);
  }
  else
  {
    m_errormessage = "Service address: url without a service";
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
  XMLElement* portType = p_wsdl.FindElement("portType");
  if(!portType)
  {
    m_errormessage = "No <wsdl:portType> within the WSDL.";
    return false;
  }

  // Number of the operation
  int index = 0;

  // Walk the list of operations
  XMLElement* operation = p_wsdl.FindElement(portType,"operation",false);
  while(operation)
  {
    CString name;
    CString msgInput;
    CString msgOutput;
    XMLAttribute* nameAtt = p_wsdl.FindAttribute(operation,"name");
    if(nameAtt)
    {
      name = nameAtt->m_value;
    }
    else
    {
      m_errormessage = "WSDL <portType>/<operation> without a name attribute";
      return false;
    }
    XMLElement* input  = p_wsdl.FindElement(operation,"input", false);
    XMLElement* output = p_wsdl.FindElement(operation,"output",false);
    if(input == nullptr || output == nullptr)
    {
      m_errormessage = "WSDL <portType> operation must have a <input> AND a <output> node!";
      return false;
    }
    msgInput  = p_wsdl.GetAttribute(input, "message");
    msgOutput = p_wsdl.GetAttribute(output,"message");
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
      m_errormessage.Format("Cannot add WS operation [%s] to the WSDL cache.",name);
      return false;
    }
    CString message;
    message.Format("Read WSDL service: %s", msgIn.GetSoapAction());
    DETAILLOG(message);

    // Find next operation
    operation = p_wsdl.GetElementSibling(operation);
  }

  // Check that we did read some operations
  if(m_operations.empty())
  {
    m_errormessage = "WSDL wany <portType> operations.";
    return false;
  }
  return true;
}

bool
WSDLCache::ReadMessage(XMLMessage& p_wsdl,SOAPMessage& p_message)
{
  // Find the correct message
  CString name = p_message.GetSoapAction();

  XMLElement* message = p_wsdl.FindElementWithAttribute(nullptr,"message","name",name);
  if(!message)
  {
    m_errormessage.Format("<message> with name=%s not found in the WSDL",p_message.GetSoapAction());
    return false;
  }

  // Find the parameters element
  XMLElement* part = p_wsdl.FindElement(message,"part");
  if(!part)
  {
    m_errormessage.Format("Message [%s] without a part",p_message.GetSoapAction());
    return false;
  }

  XMLAttribute* elem = p_wsdl.FindAttribute(part,"element");
  if(!elem)
  {
    m_errormessage.Format("Message [%s] without an element part",p_message.GetSoapAction());
    return false;
  }

  CString parameter(elem->m_value);
  SplitNamespace(parameter);

  p_message.SetSoapAction(parameter);
  p_message.SetParameterObject(parameter);

  // Record the target name space in the object node
  XMLElement* param = p_message.GetParameterObjectNode();
  if(param)
  {
    param->m_namespace = "tns";
    p_message.SetAttribute(param,"tns",m_targetNamespace);
  }

  return ReadParameters(p_wsdl,p_message,parameter);
}

bool
WSDLCache::ReadParameters(XMLMessage& p_wsdl,SOAPMessage& p_message,CString p_element)
{
  XMLElement* base = nullptr;

  XMLElement* element = ReadTypesElement(p_wsdl,p_element);
  if(element == nullptr)
  {
    m_errormessage.Format("<type><element> with name=%s not found in WSDL",p_element);
    return false;
  }
  XMLElement* complex = p_wsdl.FindElement(element,"complexType");
  if(complex == nullptr)
  {
    CString type = p_wsdl.GetAttribute(element,"type");
    if(!type.IsEmpty())
    {
      complex = ReadTypesElement(p_wsdl,type);
    }
    if(complex == nullptr)
    {
      m_errormessage.Format("<type><element> with name=%s without 'complex' definition in WSDL",p_element);
      return false;
    }
  }
  XMLElement* order = p_wsdl.GetElementFirstChild(complex);
  while(order && order->GetName() == "annotation")
  {
    order = p_wsdl.GetElementSibling(order);
  }
  if(!order)
  {
    m_errormessage.Format("<type><element> with name=%s without order node in WSDL",p_element);
    return false;
  }

  // Setting the WSDL order
  WsdlOrder ord = WsdlOrder::WS_All;
       if(order->GetName() == "all")      ord = WsdlOrder::WS_All;
  else if(order->GetName() == "choice")   ord = WsdlOrder::WS_Choice;
  else if(order->GetName() == "sequence") ord = WsdlOrder::WS_Sequence;
  else 
  {
    m_errormessage.Format("<type><element> with name=%s with illegal order node in WSDL",p_element);
    return false;
  }
  p_message.SetWSDLOrder(ord);

  // Read all elements within the order
  return ReadParametersInOrder(p_wsdl,p_message,base,order);
}

XMLElement* 
WSDLCache::ReadTypesType(XMLMessage& p_wsdl,CString p_element)
{
  // Earlier on already found
  TypeMap::iterator it = m_types.find(p_element);
  if(it != m_types.end())
  {
    return it->second;
  }

  // Findin the types node
  XMLElement* types = p_wsdl.FindElement("types");
  if(types == nullptr)
  {
    return nullptr;
  }

  // Search for complex type of this name (most common)
  XMLElement* complex = p_wsdl.FindElementWithAttribute(types,"complexType","name",p_element);
  if(complex)
  {
    m_types.insert(std::make_pair(p_element,complex));
    return complex;
  }

  // Search for simple type (with optional restrictions)
  XMLElement* simple = p_wsdl.FindElementWithAttribute(types,"simpleType","name",p_element);
  if(simple)
  {
    m_types.insert(std::make_pair(p_element,simple));
    return simple;
  }
  return nullptr;
}

XMLElement*
WSDLCache::ReadTypesElement(XMLMessage& p_wsdl,CString p_element)
{
  XMLElement* types = p_wsdl.FindElement("types");
  if(types == nullptr)
  {
    return nullptr;
  }

  // Find an element of this name
  XMLElement* elem = p_wsdl.FindElement(types,"element");
  while(elem)
  {
    XMLAttribute* nameAtt = p_wsdl.FindAttribute(elem,"name");
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
WSDLCache::ReadElementaryType(CString p_type)
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

int
WSDLCache::ReadWSDLOptions(XMLMessage& p_wsdl,XMLElement* p_element)
{
  int options = WSDL_Optional;
  CString minOccurs = p_wsdl.GetAttribute(p_element,"minOccurs");
  CString maxOccurs = p_wsdl.GetAttribute(p_element,"maxOccurs");
  int     minNum    = atoi(minOccurs);
  int     maxNum    = atoi(maxOccurs);


  if(minNum == 1 && maxNum == 1)
  {
    options = WSDL_Mandatory;
  }
  else if(minNum == 0 && maxNum == 1)
  {
    options = WSDL_Optional;
  }
  else if(maxOccurs == "unbounded")
  {
     options = minNum == 1 ? WSDL_OneMany : WSDL_ZeroMany;
  }
  return options;
}

int 
WSDLCache::ReadWSDLOrdering(XMLElement* p_order)
{
  if(p_order->GetName() == "choice")
  {
    return WSDL_Choice;
  }
  if(p_order->GetName() == "sequence")
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
    CString name = p_wsdl.GetAttribute(child,"name");
    CString type = p_wsdl.GetAttribute(child,"type");
    CString nspc = SplitNamespace(type);
    CString elName(name);
    CString nspcName;

    // Halfway a set of elements, we can encounter a new order
    if(child->GetName() == "all" || child->GetName() == "sequence" || child->GetName() == "choice")
    {
      // all/sequence/choice -> read complex type 
      ReadParametersInOrder(p_wsdl,p_message,p_base,child);
      child = p_wsdl.GetElementSibling(child);
      continue;
    }

    if(!nspc.IsEmpty())
    {
      nspcName = p_wsdl.GetAttribute(child,nspc);
      elName = nspc + ":" + name;
    }

    // Get minOccurs, maxOccurs (0,1,unbounded), nillable
    int nodeOptions = ReadWSDLOptions(p_wsdl,child);
    int options = typeOptions + nodeOptions;

    int elemtype = ReadElementaryType(type);
    if(elemtype)
    {
      XMLElement* newElm = p_message.AddElement(p_base,elName,(ushort)(options + elemtype),type);
      if(!nspcName.IsEmpty())
      {
        CString atName = "xmlns:" + nspc;
        p_message.SetAttribute(newElm,atName,nspcName);
      }
    }
    else
    {
      XMLElement* newelem = p_message.AddElement(p_base,elName,(ushort)(XDT_Complex + options),"");
      if(!nspcName.IsEmpty())
      {
        CString atName = "xmlns:" + nspc;
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
        if(detail->GetName() == "complexType" || detail->GetName() == "simpleType")
        {
          newtype = detail;
        }
      }
      XMLElement* order = p_wsdl.GetElementFirstChild(newtype);
      if(order->GetName() == "restriction")
      {
        ReadRestriction(p_wsdl,newelem,order,type,options);
      }
      else
      {
        while(order && (order->GetName() != "all" && order->GetName() != "sequence" && order->GetName() != "choice"))
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
          newelem->m_type = (ushort) (XDT_String + options);
          newelem->m_value = type;
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
                          ,CString     p_restriction
                          ,int         p_options)
{
  // Create the enumeration restriction
  XMLRestriction* restrict = m_restrictions.AddRestriction(p_restriction);
  p_newelem->m_restriction = restrict;

  // Setting the base type
  CString baseType = p_wsdl.GetAttribute(p_restrict,"base");
  if(!baseType.IsEmpty())
  {
    SplitNamespace(baseType);
    int elemBaseType   = ReadElementaryType(baseType);
    p_newelem->m_type  = (ushort)(elemBaseType + p_options);
    p_newelem->m_value = baseType;

    restrict->AddBaseType(baseType);
  }

  XMLElement* child = p_wsdl.GetElementFirstChild(p_restrict);
  while(child)
  {
    CString value = p_wsdl.GetAttribute(child,"value");

    if(child->GetName() == "enumeration")
    {
      // Reading an enumeration
      CString docum = ReadAnnoDocumentation(p_wsdl,child);
      restrict->AddEnumeration(value,docum);
    }
    else if(child->GetName() == "length")         restrict->AddLength(atoi(value));
    else if(child->GetName() == "maxLength")      restrict->AddMaxLength(atoi(value));
    else if(child->GetName() == "minLength")      restrict->AddMinLength(atoi(value));
    else if(child->GetName() == "totalDigits")    restrict->AddTotalDigits(atoi(value));
    else if(child->GetName() == "fractionDigits") restrict->AddFractionDigits(atoi(value));
    else if(child->GetName() == "pattern")        restrict->AddPattern(value);
    else if(child->GetName() == "maxExclusive")   restrict->AddMaxExclusive(value);
    else if(child->GetName() == "maxInclusive")   restrict->AddMaxInclusive(value);
    else if(child->GetName() == "minExclusive")   restrict->AddMinExclusive(value);
    else if(child->GetName() == "minInclusive")   restrict->AddMinInclusive(value);
    else if(child->GetName() == "whiteSpace")     restrict->AddWhitespace(ReadWhiteSpace(value));

    // Next child restriction
    child = p_wsdl.GetElementSibling(child);
  }
}

int
WSDLCache::ReadWhiteSpace(CString p_value)
{
  if(p_value.CompareNoCase("preserve") == 0) return 1;
  if(p_value.CompareNoCase("replace")  == 0) return 2;
  if(p_value.CompareNoCase("collapse") == 0) return 3;
  return 0;
}

// Read <annotation><documentation> of a enumeration
CString
WSDLCache::ReadAnnoDocumentation(XMLMessage& p_wsdl,XMLElement* enumeration)
{
  CString documentation;
  XMLElement* annotation = p_wsdl.FindElement(enumeration,"annotation");
  if(annotation)
  {
    XMLElement* docu = p_wsdl.FindElement(annotation,"documentation");
    if(docu)
    {
      return docu->GetValue();
    }
  }
  return documentation;
}
