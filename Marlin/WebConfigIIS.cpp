#include "stdafx.h"
#include "WebConfigIIS.h"
#include "XMLMessage.h"
#include <io.h>

WebConfigIIS::WebConfigIIS(CString p_application /*=""*/)
             :m_application(p_application)
{
}

WebConfigIIS::~WebConfigIIS()
{
}

bool
WebConfigIIS::ReadConfig()
{
  bool result = false;

  result = ReadConfig("%windir%\\system32\\inetsrv\\config\\ApplicationHost.Config");
  if(!m_application.IsEmpty())
  {
    IISSite* site = GetSite(m_application);
    if(site)
    {
      CString virtualDir;
      // Get virtual dir
      CString config = site->m_path + "\\Web.Config";
      result = ReadConfig(config);
    }
  }
  return result;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE METHODS
//
//////////////////////////////////////////////////////////////////////////

// Replace environment variables in a string
bool 
WebConfigIIS::ReplaceEnvironVars(CString& p_string)
{
  bool replaced = false;
  CString local(p_string);

  while(true)
  {
    // Find first and second '%' marker
    int beginPos = local.Find('%');
    if(beginPos < 0)
    {
      return replaced;
    }
    int endPos = local.Find('%',beginPos+1);
    if(endPos <= beginPos)
    {
      break;
    }

    // Two markers found
    CString var = local.Mid(beginPos + 1, (endPos - beginPos - 1));
    CString value;
    value.GetEnvironmentVariable(var);
    if(!value.IsEmpty())
    {
      local = local.Left(beginPos) + value + local.Mid(endPos + 1);
      replaced = true;
    }
    else
    {
      break;
    }
  }
  return false;
}

bool
WebConfigIIS::ReadConfig(CString p_configFile)
{
  // Find the file, see if we have read access at least
  ReplaceEnvironVars(p_configFile);
  if(_access(p_configFile,4) != 0)
  {
    return false;
  }

  // Parse the incoming file
  XMLMessage msg;
  msg.LoadFile(p_configFile);

  ReadLogPath(msg);
  ReadSites(msg);


  return true;
}

void
WebConfigIIS::ReadLogPath(XMLMessage& p_msg)
{
  XMLElement* log = p_msg.FindElement("log");
  if(log)
  {
    XMLElement* central = p_msg.FindElement(log,"centralW3CLogFile");
    if(central)
    {
      CString enabled   = p_msg.GetAttribute(central,"enabled");
      CString directory = p_msg.GetAttribute(central,"directory");

      // Remember this
      m_logging = enabled.CompareNoCase("true") == 0;
      m_logpath = directory;
    }
  }
}

void
WebConfigIIS::ReadSites(XMLMessage& p_msg)
{
  XMLElement* sites = p_msg.FindElement("Sites");
  if(!sites) return;

  XMLElement* site = p_msg.FindElement(sites,"Site");
  while(site)
  {
    IISSite theSite;
    CString name = p_msg.GetAttribute(site,"name");
    theSite.m_name = name;

    // Virtual path
    XMLElement* virtdir = p_msg.FindElement(site,"virtualDirectory");
    if(virtdir)
    {
      theSite.m_path = p_msg.GetAttribute(virtdir,"physicalPath");
    }

    // First binding
    XMLElement* binding = p_msg.FindElement(site,"binding");
    if(binding)
    {
      theSite.m_binding  = p_msg.GetAttribute(binding,"bindingInformation");
      theSite.m_protocol = p_msg.GetAttribute(binding,"protocol");
      theSite.m_secure   = p_msg.GetAttribute(binding,"sslFlags").GetLength() > 0;
    }

    // Port is part of URLACL binding "*:80" or "+:443" or something
    theSite.m_port = atoi(theSite.m_binding.Mid(2));

    // Add to sites
    name.MakeLower();
    m_sites.insert(std::make_pair(name,theSite));
    // Next site
    site = p_msg.GetElementSibling(site);
  }
}

IISSite*
WebConfigIIS::GetSite(CString p_site)
{
  p_site.MakeLower();
  IISSites::iterator it = m_sites.find(p_site);
  if(it != m_sites.end())
  {
    return &it->second;
  }
  return nullptr;
}
