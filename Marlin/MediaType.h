/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MediaType.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2021 ir. W.E. Huisman
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
  MediaType(CString p_extension,CString p_contentType);
 ~MediaType();

  // Getters
  CString GetExtension()   { return m_extension;   };
  CString GetContentType() { return m_contentType; };
  CString GetContent();

  // Setters
  void    SetExtension(CString p_extension) { m_extension   = p_extension; };
  void    SetContentType(CString p_type)    { m_contentType = p_type;      };

  bool    IsVendorSpecific();
  CString BaseType();
  CString ExtendedType();
private:
  CString m_extension;
  CString m_contentType;
};

using MediaTypeMap = std::map<CString,MediaType>;

class MediaTypes
{
public:
  MediaTypes();
 ~MediaTypes();
  // Check for a valid object
  static void       CheckValid();
  // Registering your own content types
         void       AddContentType(CString p_extension,CString p_contentType);
  // Finding the content types
  static CString    FindContentTypeByExtension  (CString p_extension);
  static CString    FindContentTypeByResouceName(CString p_resource);
  static MediaType* FindMediaTypeByExtension    (CString p_extension);
  static CString    FindExtensionByContentType  (CString p_contentType);

private:
  void Init();
  MediaTypeMap m_types;
};