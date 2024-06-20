/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MediaType.cpp
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
#include "stdafx.h"
#include "MediaType.h"
#include <WinFile.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

MediaType::MediaType(bool p_logging,XString p_extension,XString p_contentType)
          :m_logging(p_logging)
          ,m_extension(p_extension)
          ,m_contentType(p_contentType)
{
}

MediaType::~MediaType()
{
}

bool
MediaType::IsVendorSpecific()
{
  XString content(m_contentType);
  content.MakeLower();
  return (m_contentType.Find(_T("/vnd.")) > 0);
}

// Return the base type
// e.g. "text/html" -> "text"
XString 
MediaType::BaseType()
{
  int pos = m_contentType.Find('/');
  if(pos > 0)
  {
    return m_contentType.Left(pos);
  }
  return _T("");
}

// Return the short content
// e.g. "text/html" -> "html"
XString 
MediaType::GetContent()
{
  int pos = m_contentType.Find('/');
  if(pos > 0)
  {
    XString content = m_contentType.Mid(pos + 1);
    pos = content.Find('+');
    if(pos > 0)
    {
      content = content.Left(pos);
    }
    return content;
  }
  return _T("");
}

// Return the extended type
// eg. "application/soap+xml" -> "xml"
XString 
MediaType::ExtendedType()
{
  int pos = m_contentType.Find('+');
  if(pos > 0)
  {
    return m_contentType.Mid(pos);
  }
  return _T("");
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
    ASSERT(_T("There can only be ONE mediatypes object"));
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
  m_types.insert(std::make_pair(_T("txt"),   MediaType(true, _T("txt"),    _T("text"))));
  m_types.insert(std::make_pair(_T("html"),  MediaType(true, _T("html"),   _T("text/html"))));
  m_types.insert(std::make_pair(_T("htm"),   MediaType(true, _T("htm"),    _T("text/html"))));
  m_types.insert(std::make_pair(_T("css"),   MediaType(true, _T("css"),    _T("text/css"))));
  m_types.insert(std::make_pair(_T("rtf"),   MediaType(true, _T("rtf"),    _T("text/rtf"))));
  m_types.insert(std::make_pair(_T("js"),    MediaType(true, _T("js"),     _T("application/javascript"))));
  m_types.insert(std::make_pair(_T("xml"),   MediaType(true, _T("xml"),    _T("application/soap+xml"))));
  m_types.insert(std::make_pair(_T("dtd"),   MediaType(true, _T("dtd"),    _T("application/xml+dtd"))));
  m_types.insert(std::make_pair(_T("xslt"),  MediaType(true, _T("xslt"),   _T("application/xslt+xml"))));
  m_types.insert(std::make_pair(_T("mathml"),MediaType(true, _T("mathml"), _T("application/mathml+xml"))));
  m_types.insert(std::make_pair(_T("json"),  MediaType(true, _T("json"),   _T("application/json"))));
  m_types.insert(std::make_pair(_T("svg"),   MediaType(true, _T("svg"),    _T("image/svg+xml"))));
  m_types.insert(std::make_pair(_T("pdf"),   MediaType(false,_T("pdf"),    _T("application/pdf"))));
  m_types.insert(std::make_pair(_T("ttf"),   MediaType(false,_T("ttf"),    _T("application/octet-stream"))));
  m_types.insert(std::make_pair(_T("png"),   MediaType(false,_T("png"),    _T("image/png"))));
  m_types.insert(std::make_pair(_T("jpg"),   MediaType(false,_T("jpg"),    _T("image/jpg"))));
  m_types.insert(std::make_pair(_T("jpeg"),  MediaType(false,_T("jpeg"),   _T("image/jpg"))));
  m_types.insert(std::make_pair(_T("gif"),   MediaType(false,_T("gif"),    _T("image/gif"))));
  m_types.insert(std::make_pair(_T("ico"),   MediaType(false,_T("ico"),    _T("image/x-icon"))));
  // Old style MS-Word upto version 2013/XP
  m_types.insert(std::make_pair(_T("doc"),   MediaType(false,_T("doc"),    _T("application/msword"))));
  m_types.insert(std::make_pair(_T("dot"),   MediaType(false,_T("doc"),    _T("application/msword"))));
  m_types.insert(std::make_pair(_T("xls"),   MediaType(false,_T("xls"),    _T("application/vnd.ms-excel"))));
  m_types.insert(std::make_pair(_T("xlt"),   MediaType(false,_T("xlt"),    _T("application/vnd.ms-excel"))));
  m_types.insert(std::make_pair(_T("xla"),   MediaType(false,_T("xla"),    _T("application/vnd.ms-excel"))));
  // Old style Powerpoint documents                                                                                                                                                                                                                    m_types.insert(std::make_pair(".xlam     application/vnd.ms-excel.addin.macroEnabled.12
  m_types.insert(std::make_pair(_T("ppt"),   MediaType(false,_T("ppt"),    _T("application/vnd.ms-powerpoint"))));
  m_types.insert(std::make_pair(_T("pot"),   MediaType(false,_T("pot"),    _T("application/vnd.ms-powerpoint"))));
  m_types.insert(std::make_pair(_T("pps"),   MediaType(false,_T("pps"),    _T("application/vnd.ms-powerpoint"))));
  m_types.insert(std::make_pair(_T("ppa"),   MediaType(false,_T("ppa"),    _T("application/vnd.ms-powerpoint"))));
  // New MS-Word OOXML text documents
  m_types.insert(std::make_pair(_T("docx"),  MediaType(false,_T("docx"),   _T("application/vnd.openxmlformats-officedocument.wordprocessingml.document"))));
  m_types.insert(std::make_pair(_T("dotx"),  MediaType(false,_T("dotx"),   _T("application/vnd.openxmlformats-officedocument.wordprocessingml.template"))));
  m_types.insert(std::make_pair(_T("docm"),  MediaType(false,_T("docm"),   _T("application/vnd.ms-word.document.macroEnabled.12"))));
  m_types.insert(std::make_pair(_T("dotm"),  MediaType(false,_T("dotm"),   _T("application/vnd.ms-word.template.macroEnabled.12"))));
  // New MS-Word OOXML spreadsheet documents
  m_types.insert(std::make_pair(_T("xlsx"),  MediaType(false,_T("xlsx"),   _T("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"))));
  m_types.insert(std::make_pair(_T("xltx"),  MediaType(false,_T("xltx"),   _T("application/vnd.openxmlformats-officedocument.spreadsheetml.template"))));
  m_types.insert(std::make_pair(_T("xlsm"),  MediaType(false,_T("xlsm"),   _T("application/vnd.ms-excel.sheet.macroEnabled.12"))));
  m_types.insert(std::make_pair(_T("xltm"),  MediaType(false,_T("xltm"),   _T("application/vnd.ms-excel.template.macroEnabled.12"))));
  // New MS-Word OOXML Powerpoint documents
  m_types.insert(std::make_pair(_T("pptx"),  MediaType(false,_T("pptx"),   _T("application/vnd.openxmlformats-officedocument.presentationml.presentation"))));
  m_types.insert(std::make_pair(_T("potx"),  MediaType(false,_T("potx"),   _T("application/vnd.openxmlformats-officedocument.presentationml.template"))));
  m_types.insert(std::make_pair(_T("ppsx"),  MediaType(false,_T("ppsx"),   _T("application/vnd.openxmlformats-officedocument.presentationml.slideshow"))));
  m_types.insert(std::make_pair(_T("ppam"),  MediaType(false,_T("ppam"),   _T("application/vnd.ms-powerpoint.addin.macroEnabled.12"))));
  m_types.insert(std::make_pair(_T("pptm"),  MediaType(false,_T("pptm"),   _T("application/vnd.ms-powerpoint.presentation.macroEnabled.12"))));
  m_types.insert(std::make_pair(_T("potm"),  MediaType(false,_T("potm"),   _T("application/vnd.ms-powerpoint.template.macroEnabled.12"))));
  m_types.insert(std::make_pair(_T("ppsm"),  MediaType(false,_T("ppsm"),   _T("application/vnd.ms-powerpoint.slideshow.macroEnabled.12"))));
  // Open Office documents (OpenOffice 2.0 and later, StarOffice 8.0 and later)
  // For older mimetypes see: https://www.openoffice.org/framework/documentation/mimetypes/mimetypes.html
  m_types.insert(std::make_pair(_T("odt"),   MediaType(false,_T("odt"),    _T("application/vnd.oasis.opendocument.text"))));
  m_types.insert(std::make_pair(_T("ott"),   MediaType(false,_T("ott"),    _T("application/vnd.oasis.opendocument.text-template"))));
  m_types.insert(std::make_pair(_T("oth"),   MediaType(false,_T("oth"),    _T("application/vnd.oasis.opendocument.text-web"))));
  m_types.insert(std::make_pair(_T("odm"),   MediaType(false,_T("odm"),    _T("application/vnd.oasis.opendocument.text-master"))));
  m_types.insert(std::make_pair(_T("odg"),   MediaType(false,_T("odg"),    _T("application/vnd.oasis.opendocument.graphics"))));
  m_types.insert(std::make_pair(_T("otg"),   MediaType(false,_T("otg"),    _T("application/vnd.oasis.opendocument.graphics-template"))));
  m_types.insert(std::make_pair(_T("odp"),   MediaType(false,_T("odp"),    _T("application/vnd.oasis.opendocument.presentation"))));
  m_types.insert(std::make_pair(_T("otp"),   MediaType(false,_T("otp"),    _T("application/vnd.oasis.opendocument.presentation-template"))));
  m_types.insert(std::make_pair(_T("ods"),   MediaType(false,_T("ods"),    _T("application/vnd.oasis.opendocument.spreadsheet"))));
  m_types.insert(std::make_pair(_T("ots"),   MediaType(false,_T("ots"),    _T("application/vnd.oasis.opendocument.spreadsheet-template"))));
  m_types.insert(std::make_pair(_T("odc"),   MediaType(false,_T("odc"),    _T("application/vnd.oasis.opendocument.chart"))));
  m_types.insert(std::make_pair(_T("odf"),   MediaType(false,_T("odf"),    _T("application/vnd.oasis.opendocument.formula"))));
  m_types.insert(std::make_pair(_T("odb"),   MediaType(false,_T("odb"),    _T("application/vnd.oasis.opendocument.database"))));
  m_types.insert(std::make_pair(_T("odi"),   MediaType(false,_T("odi"),    _T("application/vnd.oasis.opendocument.image"))));
  m_types.insert(std::make_pair(_T("oxt"),   MediaType(false,_T("oxt"),    _T("application/vnd.openofficeorg.extension"))));
  // The grand default of all MIME media types
  m_types.insert(std::make_pair(_T(""),      MediaType(false,_T(""),       _T("application/octet-stream"))));
}

/* static*/ void
MediaTypes::CheckValid()
{
#ifdef _DEBUG
  if(g_mt_instance == nullptr)
  {
    throw StdException(_T("INTERNAL: No MediaTypes object found/initialized"));
  }
#endif
}

// Registering your own
void   
MediaTypes::AddContentType(bool p_logging,XString p_extension,XString p_contentType)
{
  m_types  .insert(std::make_pair(p_extension,  MediaType(p_logging,p_extension,p_contentType)));
  m_content.insert(std::make_pair(p_contentType,MediaType(p_logging,p_extension,p_contentType)));
}

/* static */ XString  
MediaTypes::FindContentTypeByExtension(XString p_extension)
{
  CheckValid();
  p_extension.TrimLeft('.');
  MediaTypeMap::iterator it = g_mt_instance->m_types.find(p_extension);
  if(it != g_mt_instance->m_types.end())
  {
    return it->second.GetContentType();
  }
  return _T("");
}

/* static */ XString
MediaTypes::FindContentTypeByResouceName(XString p_resource)
{
  CheckValid();
  WinFile ensure(p_resource);
  XString extens = ensure.GetFilenamePartExtension();
  return FindContentTypeByExtension(extens);
}

/* static */ MediaType*
MediaTypes::FindMediaTypeByExtension(XString p_extension)
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

/*static */ MediaType*
MediaTypes::FindMediaTypeByContentType(XString p_contentType)
{
  p_contentType.MakeLower();
  MediaTypeMap::iterator it = g_mt_instance->m_content.find(p_contentType);

  if(it != g_mt_instance->m_content.end())
  {
    return &it->second;
  }
  return nullptr;
}

/* static */ XString  
MediaTypes::FindExtensionByContentType(XString p_contentType)
{
  p_contentType.MakeLower();
  MediaTypeMap::iterator it = g_mt_instance->m_content.find(p_contentType);

  if (it != g_mt_instance->m_content.end())
  {
    return it->first;
  }
  return "";
}

