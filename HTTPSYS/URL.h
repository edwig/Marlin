//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 - 2024 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#pragma once

class UrlGroup;

// Type of a URL prefix string
typedef enum class _http_PrefixType
{
   URLPRE_Nothing  // Not set (yet)
  ,URLPRE_Strong   // Strong binding      "+"
  ,URLPRE_Named    // Short server name   "marlin"
  ,URLPRE_FQN      // Full qualified name "marlin.organization.lan"
  ,URLPRE_Address  // Direct IP address   "p.q.r.s"
  ,URLPRE_Weak     // Weak binding        "*"
}
UrlPrefixType;

// All elements of a URL prefix and secure HTTPS connection
class URL
{
public:
  bool          m_secure;             // HTTP (false) or HTTPS (true)
  UrlPrefixType m_type;               // +, *, IP, name or full-name
  XString       m_host;               // Hostname
  USHORT        m_port;               // port number
  XString       m_abspath;            // Absolute URL path
  ULONGLONG     m_context;            // Registered context number
  UrlGroup*     m_urlGroup;           // UrlGroup we where added to
  // SSL/TLS settings
  XString       m_certStoreName;                        // Certificate store
  BYTE          m_thumbprint[CERT_THUMBPRINT_SIZE + 1]; // SSL/TLS certificate used for HTTPS
  bool          m_requestClientCert;                    // Do request a client certificate
};

typedef URL* PURL;


