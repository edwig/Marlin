/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MediaType.h
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
#pragma once
#include <map>

class MediaType
{
public:
  MediaType(bool p_logging,XString p_extension,XString p_contentType);
 ~MediaType();

  // Getters
  bool    GetDoLogging()   { return m_logging;     }
  XString GetExtension()   { return m_extension;   }
  XString GetContentType() { return m_contentType; }
  XString GetContent();

  // Setters
  void    SetDoLogging(bool p_logging)      { m_logging     = p_logging;   }
  void    SetExtension(XString p_extension) { m_extension   = p_extension; }
  void    SetContentType(XString p_type)    { m_contentType = p_type;      }

  bool    IsVendorSpecific();
  XString BaseType();
  XString ExtendedType();
private:
  bool    m_logging;
  XString m_extension;
  XString m_contentType;
};

using MediaTypeMap = std::multimap<XString,MediaType>;

class MediaTypes
{
public:
  MediaTypes();
 ~MediaTypes();
  // Check for a valid object
  static void       CheckValid();
  // Registering your own content types
         void       AddContentType(bool p_logging,XString p_extension,XString p_contentType);
  // Finding the content types
  static XString    FindContentTypeByExtension  (XString p_extension);
  static XString    FindContentTypeByResouceName(XString p_resource);
  static MediaType* FindMediaTypeByExtension    (XString p_extension);
  static MediaType* FindMediaTypeByContentType  (XString p_contentType);
  static XString    FindExtensionByContentType  (XString p_contentType);

private:
  void Init();
  MediaTypeMap m_types;
  MediaTypeMap m_content;
};