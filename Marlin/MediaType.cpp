/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MediaType.cpp
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
#include "MediaType.h"
#include "EnsureFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

MediaType::MediaType(CString p_extension,CString p_contentType)
          :m_extension(p_extension)
          ,m_contentType(p_contentType)
{
}

MediaType::~MediaType()
{
}

bool
MediaType::IsVendorSpecific()
{
  CString content(m_contentType);
  content.MakeLower();
  return (m_contentType.Find("/vnd.") > 0);
}

// Return the base type
// e.g. "text/html" -> "text"
CString 
MediaType::BaseType()
{
  int pos = m_contentType.Find('/');
  if(pos > 0)
  {
    return m_contentType.Left(pos);
  }
  return "";
}

// Return the short content
// e.g. "text/html" -> "html"
CString 
MediaType::GetContent()
{
  int pos = m_contentType.Find('/');
  if(pos > 0)
  {
    CString content = m_contentType.Mid(pos + 1);
    pos = content.Find('+');
    if(pos > 0)
    {
      content = content.Left(pos);
    }
    return content;
  }
  return "";
}

// Return the extended type
// eg. "application/soap+xml" -> "xml"
CString 
MediaType::ExtendedType()
{
  int pos = m_contentType.Find('+');
  if(pos > 0)
  {
    return m_contentType.Mid(pos);
  }
  return "";
}

//////////////////////////////////////////////////////////////////////////
//
// Singleton MediaTypes object
//
//////////////////////////////////////////////////////////////////////////

static MediaTypes* g_mt_instance = nullptr;

// XTOR: Make sure it is a singleton
MediaTypes::MediaTypes()
{
  if(g_mt_instance == nullptr)
  {
    g_mt_instance = this;
    Init();
  }
  else
  {
    ASSERT("There can only be ONE mediatypes object");
  }
}

// DTOR: No more singleton in our address space
MediaTypes::~MediaTypes()
{
  g_mt_instance = nullptr;
}

void
MediaTypes::Init()
{
  // General media types
  m_types.insert(std::make_pair("txt",   MediaType("txt",    "text")));
  m_types.insert(std::make_pair("html",  MediaType("html",   "text/html")));
  m_types.insert(std::make_pair("htm",   MediaType("htm",    "text/html")));
  m_types.insert(std::make_pair("css",   MediaType("css",    "text/css")));
  m_types.insert(std::make_pair("rtf",   MediaType("rtf",    "text/rtf")));
  m_types.insert(std::make_pair("js",    MediaType("js",     "application/javascript")));
  m_types.insert(std::make_pair("xml",   MediaType("xml",    "application/soap+xml")));
  m_types.insert(std::make_pair("dtd",   MediaType("dtd",    "application/xml+dtd")));
  m_types.insert(std::make_pair("xslt",  MediaType("xslt",   "application/xslt+xml")));
  m_types.insert(std::make_pair("mathml",MediaType("mathml", "application/mathml+xml")));
  m_types.insert(std::make_pair("pdf",   MediaType("pdf",    "application/pdf")));
  m_types.insert(std::make_pair("ttf",   MediaType("ttf",    "application/octet-stream")));
  m_types.insert(std::make_pair("png",   MediaType("png",    "image/png")));
  m_types.insert(std::make_pair("jpg",   MediaType("jpg",    "image/jpg")));
  m_types.insert(std::make_pair("jpeg",  MediaType("jpeg",   "image/jpg")));
  m_types.insert(std::make_pair("gif",   MediaType("gif",    "image/gif")));
  m_types.insert(std::make_pair("svg",   MediaType("svg",    "image/svg+xml")));
  m_types.insert(std::make_pair("ico",   MediaType("ico",    "image/x-icon")));
  // Old style MS-Word upto version 2013/XP
  m_types.insert(std::make_pair("doc",   MediaType("doc",    "application/msword")));
  m_types.insert(std::make_pair("dot",   MediaType("doc",    "application/msword")));
  m_types.insert(std::make_pair("xls",   MediaType("xls",    "application/vnd.ms-excel")));
  m_types.insert(std::make_pair("xlt",   MediaType("xlt",    "application/vnd.ms-excel")));
  m_types.insert(std::make_pair("xla",   MediaType("xla",    "application/vnd.ms-excel")));
  // Old style Powerpoint documents                                                                                                                                                                                                                    m_types.insert(std::make_pair(".xlam     application/vnd.ms-excel.addin.macroEnabled.12
  m_types.insert(std::make_pair("ppt",   MediaType("ppt",    "application/vnd.ms-powerpoint")));
  m_types.insert(std::make_pair("pot",   MediaType("pot",    "application/vnd.ms-powerpoint")));
  m_types.insert(std::make_pair("pps",   MediaType("pps",    "application/vnd.ms-powerpoint")));
  m_types.insert(std::make_pair("ppa",   MediaType("ppa",    "application/vnd.ms-powerpoint")));
  // New MS-Word OOXML text documents
  m_types.insert(std::make_pair("docx",  MediaType("docx",   "application/vnd.openxmlformats-officedocument.wordprocessingml.document")));
  m_types.insert(std::make_pair("dotx",  MediaType("dotx",   "application/vnd.openxmlformats-officedocument.wordprocessingml.template")));
  m_types.insert(std::make_pair("docm",  MediaType("docm",   "application/vnd.ms-word.document.macroEnabled.12")));
  m_types.insert(std::make_pair("dotm",  MediaType("dotm",   "application/vnd.ms-word.template.macroEnabled.12")));
  // New MS-Word OOXML spreadsheet documents
  m_types.insert(std::make_pair("xlsx",  MediaType("xlsx",   "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet")));
  m_types.insert(std::make_pair("xltx",  MediaType("xltx",   "application/vnd.openxmlformats-officedocument.spreadsheetml.template")));
  m_types.insert(std::make_pair("xlsm",  MediaType("xlsm",   "application/vnd.ms-excel.sheet.macroEnabled.12")));
  m_types.insert(std::make_pair("xltm",  MediaType("xltm",   "application/vnd.ms-excel.template.macroEnabled.12")));
  // New MS-Word OOXML Powerpoint documents
  m_types.insert(std::make_pair("pptx",  MediaType("pptx",   "application/vnd.openxmlformats-officedocument.presentationml.presentation")));
  m_types.insert(std::make_pair("potx",  MediaType("potx",   "application/vnd.openxmlformats-officedocument.presentationml.template")));
  m_types.insert(std::make_pair("ppsx",  MediaType("ppsx",   "application/vnd.openxmlformats-officedocument.presentationml.slideshow")));
  m_types.insert(std::make_pair("ppam",  MediaType("ppam",   "application/vnd.ms-powerpoint.addin.macroEnabled.12")));
  m_types.insert(std::make_pair("pptm",  MediaType("pptm",   "application/vnd.ms-powerpoint.presentation.macroEnabled.12")));
  m_types.insert(std::make_pair("potm",  MediaType("potm",   "application/vnd.ms-powerpoint.template.macroEnabled.12")));
  m_types.insert(std::make_pair("ppsm",  MediaType("ppsm",   "application/vnd.ms-powerpoint.slideshow.macroEnabled.12")));
  // Open Office documents (OpenOffice 2.0 and later, StarOffice 8.0 and later)
  // For older mimetypes see: https://www.openoffice.org/framework/documentation/mimetypes/mimetypes.html
  m_types.insert(std::make_pair("odt",   MediaType("odt",    "application/vnd.oasis.opendocument.text")));
  m_types.insert(std::make_pair("ott",   MediaType("ott",    "application/vnd.oasis.opendocument.text-template")));
  m_types.insert(std::make_pair("oth",   MediaType("oth",    "application/vnd.oasis.opendocument.text-web")));
  m_types.insert(std::make_pair("odm",   MediaType("odm",    "application/vnd.oasis.opendocument.text-master")));
  m_types.insert(std::make_pair("odg",   MediaType("odg",    "application/vnd.oasis.opendocument.graphics")));
  m_types.insert(std::make_pair("otg",   MediaType("otg",    "application/vnd.oasis.opendocument.graphics-template")));
  m_types.insert(std::make_pair("odp",   MediaType("odp",    "application/vnd.oasis.opendocument.presentation")));
  m_types.insert(std::make_pair("otp",   MediaType("otp",    "application/vnd.oasis.opendocument.presentation-template")));
  m_types.insert(std::make_pair("ods",   MediaType("ods",    "application/vnd.oasis.opendocument.spreadsheet")));
  m_types.insert(std::make_pair("ots",   MediaType("ots",    "application/vnd.oasis.opendocument.spreadsheet-template")));
  m_types.insert(std::make_pair("odc",   MediaType("odc",    "application/vnd.oasis.opendocument.chart")));
  m_types.insert(std::make_pair("odf",   MediaType("odf",    "application/vnd.oasis.opendocument.formula")));
  m_types.insert(std::make_pair("odb",   MediaType("odb",    "application/vnd.oasis.opendocument.database")));
  m_types.insert(std::make_pair("odi",   MediaType("odi",    "application/vnd.oasis.opendocument.image")));
  m_types.insert(std::make_pair("oxt",   MediaType("oxt",    "application/vnd.openofficeorg.extension")));
  // The grand default of all MIME media types
  m_types.insert(std::make_pair("",      MediaType("",       "application/octet-stream")));
}

/* static*/ void
MediaTypes::CheckValid()
{
#ifdef _DEBUG
  if(g_mt_instance == nullptr)
  {
    throw StdException("INTERNAL: No MediaTypes object found/initialized");
  }
#endif
}

// Registering your own
void   
MediaTypes::AddContentType(CString p_extension,CString p_contentType)
{
  m_types.insert(std::make_pair(p_extension, MediaType(p_extension,p_contentType)));
}

/* static */ CString  
MediaTypes::FindContentTypeByExtension(CString p_extension)
{
  CheckValid();
  p_extension.TrimLeft('.');
  MediaTypeMap::iterator it = g_mt_instance->m_types.find(p_extension);
  if(it != g_mt_instance->m_types.end())
  {
    return it->second.GetContentType();
  }
  return "";
}

/* static */ CString
MediaTypes::FindContentTypeByResouceName(CString p_resource)
{
  CheckValid();
  EnsureFile ensure;
  CString extens = ensure.ExtensionPart(p_resource);
  return FindContentTypeByExtension(extens);
}

/* static */ MediaType*
MediaTypes::FindMediaTypeByExtension(CString p_extension)
{
  CheckValid();
  p_extension.TrimLeft('.');
  MediaTypeMap::iterator it = g_mt_instance->m_types.find(p_extension);
  if(it != g_mt_instance->m_types.end())
  {
    return &it->second;
  }
  return nullptr;
}

/* static */ CString  
MediaTypes::FindExtensionByContentType(CString p_contentType)
{
  for(auto& mt : g_mt_instance->m_types)
  {
    if(mt.second.GetContentType().CompareNoCase(p_contentType))
    {
      return mt.first;
    }
  }
  return "";
}

