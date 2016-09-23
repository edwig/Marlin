#pragma once
#include "HTTPSite.h"

class HTTPServerMarlin;

class HTTPSiteMarlin : public HTTPSite
{
public:
  HTTPSiteMarlin(HTTPServerMarlin*  p_server
                ,int                p_port
                ,CString            p_site
                ,CString            p_prefix
                ,HTTPSite*          p_mainSite = nullptr
                ,LPFN_CALLBACK      p_callback = nullptr);

  // MANDATORY: Explicitly starting after configuration of the site
  virtual bool StartSite();

  // OPTIONAL: Set the webroot of the site
  virtual bool SetWebroot(CString p_webroot);
  // OPTIONAL: Set XFrame options on server answer
  virtual void SetXFrameOptions(XFrameOption p_option,CString p_uri);
  // OPTIONAL: Set Strict Transport Security (HSTS)
  virtual void SetStrictTransportSecurity(unsigned p_maxAge,bool p_subDomains);
  // OPTIONAL: Set X-Content-Type options
  virtual void SetXContentTypeOptions(bool p_nosniff);
  // OPTIONAL: Set protection against X-Site scripting
  virtual void SetXSSProtection(bool p_on,bool p_block);
  // OPTIONAL: Set cache control
  virtual void SetBlockCacheControl(bool p_block);

  // FUNCTIONS

  // Add all optional extra headers of this site
  virtual void AddSiteOptionalHeaders(UKHeaders& p_headers);

private:
  // Initialize the site from automatic settings in the config
  void         InitSite(WebConfig& p_config);
  void         LogSettings();
  // Set automatic headers upon starting site
  void         SetAutomaticHeaders(WebConfig& p_config);
};
