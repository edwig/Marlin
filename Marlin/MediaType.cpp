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
#include <assert.h>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
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
    assert(_T("There can only be ONE mediatypes object"));
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
  AddContentType(true, _T("txt"),    _T("text"));
  AddContentType(true, _T("html"),   _T("text/html"));
  AddContentType(true, _T("htm"),    _T("text/html"));
  AddContentType(true, _T("css"),    _T("text/css"));
  AddContentType(true, _T("rtf"),    _T("text/rtf"));
  AddContentType(true, _T("js"),     _T("application/javascript"));
  AddContentType(true, _T("xml"),    _T("application/soap+xml"));
  AddContentType(true, _T("dtd"),    _T("application/xml+dtd"));
  AddContentType(true, _T("xslt"),   _T("application/xslt+xml"));
  AddContentType(true, _T("mathml"), _T("application/mathml+xml"));
  AddContentType(true, _T("json"),   _T("application/json"));
  AddContentType(true, _T("svg"),    _T("image/svg+xml"));
  AddContentType(false,_T("pdf"),    _T("application/pdf"));
  AddContentType(false,_T("ttf"),    _T("application/octet-stream"));
  AddContentType(false,_T("png"),    _T("image/png"));
  AddContentType(false,_T("jpg"),    _T("image/jpg"));
  AddContentType(false,_T("jpeg"),   _T("image/jpg"));
  AddContentType(false,_T("gif"),    _T("image/gif"));
  AddContentType(false,_T("ico"),    _T("image/x-icon"));
  // Old style MS-Word upto version 2013/XP
  AddContentType(false,_T("doc"),    _T("application/msword"));
  AddContentType(false,_T("doc"),    _T("application/msword"));
  AddContentType(false,_T("xls"),    _T("application/vnd.ms-excel"));
  AddContentType(false,_T("xlt"),    _T("application/vnd.ms-excel"));
  AddContentType(false,_T("xla"),    _T("application/vnd.ms-excel"));
  // Old style Powerpoint documents                                                                                                                                                                                                                    m_types.insert(std::make_pair(".xlam     application/vnd.ms-excel.addin.macroEnabled.12
  AddContentType(false,_T("ppt"),    _T("application/vnd.ms-powerpoint"));
  AddContentType(false,_T("pot"),    _T("application/vnd.ms-powerpoint"));
  AddContentType(false,_T("pps"),    _T("application/vnd.ms-powerpoint"));
  AddContentType(false,_T("ppa"),    _T("application/vnd.ms-powerpoint"));
  // New MS-Word OOXML text documents
  AddContentType(false,_T("docx"),   _T("application/vnd.openxmlformats-officedocument.wordprocessingml.document"));
  AddContentType(false,_T("dotx"),   _T("application/vnd.openxmlformats-officedocument.wordprocessingml.template"));
  AddContentType(false,_T("docm"),   _T("application/vnd.ms-word.document.macroEnabled.12"));
  AddContentType(false,_T("dotm"),   _T("application/vnd.ms-word.template.macroEnabled.12"));
  // New MS-Word OOXML spreadsheet documents
  AddContentType(false,_T("xlsx"),   _T("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"));
  AddContentType(false,_T("xltx"),   _T("application/vnd.openxmlformats-officedocument.spreadsheetml.template"));
  AddContentType(false,_T("xlsm"),   _T("application/vnd.ms-excel.sheet.macroEnabled.12"));
  AddContentType(false,_T("xltm"),   _T("application/vnd.ms-excel.template.macroEnabled.12"));
  // New MS-Word OOXML Powerpoint documents
  AddContentType(false,_T("pptx"),   _T("application/vnd.openxmlformats-officedocument.presentationml.presentation"));
  AddContentType(false,_T("potx"),   _T("application/vnd.openxmlformats-officedocument.presentationml.template"));
  AddContentType(false,_T("ppsx"),   _T("application/vnd.openxmlformats-officedocument.presentationml.slideshow"));
  AddContentType(false,_T("ppam"),   _T("application/vnd.ms-powerpoint.addin.macroEnabled.12"));
  AddContentType(false,_T("pptm"),   _T("application/vnd.ms-powerpoint.presentation.macroEnabled.12"));
  AddContentType(false,_T("potm"),   _T("application/vnd.ms-powerpoint.template.macroEnabled.12"));
  AddContentType(false,_T("ppsm"),   _T("application/vnd.ms-powerpoint.slideshow.macroEnabled.12"));
  // Open Office documents (OpenOffice 2.0 and later, StarOffice 8.0 and later)
  // For older mimetypes see: https://www.openoffice.org/framework/documentation/mimetypes/mimetypes.html
  AddContentType(false,_T("odt"),    _T("application/vnd.oasis.opendocument.text"));
  AddContentType(false,_T("ott"),    _T("application/vnd.oasis.opendocument.text-template"));
  AddContentType(false,_T("oth"),    _T("application/vnd.oasis.opendocument.text-web"));
  AddContentType(false,_T("odm"),    _T("application/vnd.oasis.opendocument.text-master"));
  AddContentType(false,_T("odg"),    _T("application/vnd.oasis.opendocument.graphics"));
  AddContentType(false,_T("otg"),    _T("application/vnd.oasis.opendocument.graphics-template"));
  AddContentType(false,_T("odp"),    _T("application/vnd.oasis.opendocument.presentation"));
  AddContentType(false,_T("otp"),    _T("application/vnd.oasis.opendocument.presentation-template"));
  AddContentType(false,_T("ods"),    _T("application/vnd.oasis.opendocument.spreadsheet"));
  AddContentType(false,_T("ots"),    _T("application/vnd.oasis.opendocument.spreadsheet-template"));
  AddContentType(false,_T("odc"),    _T("application/vnd.oasis.opendocument.chart"));
  AddContentType(false,_T("odf"),    _T("application/vnd.oasis.opendocument.formula"));
  AddContentType(false,_T("odb"),    _T("application/vnd.oasis.opendocument.database"));
  AddContentType(false,_T("odi"),    _T("application/vnd.oasis.opendocument.image"));
  AddContentType(false,_T("oxt"),    _T("application/vnd.openofficeorg.extension"));
  // The grand default of all MIME media types
  AddContentType(false,_T(""),       _T("application/octet-stream"));
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
  return _T("");
}

