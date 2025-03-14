/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MultiPartBuffer.h
//
// BaseLibrary: Indispensable general objects and functions
// 
// Copyright (c) 2014-2025 ir. W.E. Huisman
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

// IMPLEMENTATION of IETF RFC 7578: Returning Values from Forms: multipart/form-data
// Formal definition: https://tools.ietf.org/html/rfc7578
// Previous implementation: https://tools.ietf.org/html/rfc2388
//
#pragma once
#include <vector>
#include "FileBuffer.h"
#include "Headers.h"

class HTTPMessage;

// Implements the "Content-Type: multipart/form-data" type of HTTP messages
// or the default "Content-Type: application/x-www-form-urlencoded"

enum class FormDataType
{
  FD_UNKNOWN
 ,FD_URLENCODED
 ,FD_MULTIPART
 ,FD_MIXED
};

// The MultiPartBuffer consists of various MultiPart's
// Normally you should only use the MultiPart on incoming messages
// For outgoing messages normally use the next interface
class MultiPart
{
public: 
  MultiPart();
  explicit MultiPart(XString p_name,XString p_contentType);

  // SETTERS
  void    SetName(XString p_name)             { m_name             = p_name;    }
  void    SetData(XString p_data)             { m_data             = p_data;    }
  void    SetContentType(XString p_type)      { m_contentType      = p_type;    }
  void    SetCharset(XString p_charset)       { m_charset          = p_charset; }
  void    SetBoundary(XString p_boundary)     { m_boundary         = p_boundary;}
  void    SetFileName(XString p_file)         { m_shortFilename    = p_file;    }
  void    SetDateCreation(XString p_date)     { m_creationDate     = p_date;    }
  void    SetDateModification(XString p_date) { m_modificationDate = p_date;    }
  void    SetDateRead(XString p_date)         { m_readDate         = p_date;    }
  void    SetSize(size_t p_size)              { m_size             = p_size;    }
  // Setting filename and reading the new access times
  bool    SetFile(XString p_filename);

  // GETTERS
  XString GetName()             { return m_name;              }
  XString GetData()             { return m_data;              }
  XString GetContentType()      { return m_contentType;       }
  XString GetCharset()          { return m_charset;           }
  XString GetBoundary()         { return m_boundary;          }
  XString GetShortFileName()    { return m_shortFilename;     }
  XString GetLongFileName()     { return m_longFilename;      }
  XString GetDateCreation()     { return m_creationDate;      }
  XString GetDateModification() { return m_modificationDate;  }
  XString GetDateRead()         { return m_readDate;          }
  size_t  GetSize()             { return m_size;              }
  FileBuffer* GetBuffer()       { return &m_file;             }

  // Functions
  bool    WriteFile();
  bool    CheckBoundaryExists(XString p_boundary);
  XString CreateHeader(XString p_boundary,bool p_extensions = false);
  void    TrySettingFiletimes();
  void    AddHeader(XString p_header,XString p_value);
  XString GetHeader(XString p_header);
  void    DelHeader(XString p_header);

private:
  XString   FileTimeToString  (PFILETIME p_filetime);
  PFILETIME FileTimeFromString(PFILETIME p_filetime,const XString& p_time);

  // Content-Type: field
  XString m_contentType;
  XString m_charset;
  XString m_boundary;
  // Content-disposition: fields
  XString m_name;
  XString m_shortFilename;
  XString m_longFilename;
  XString m_creationDate;
  XString m_modificationDate;
  XString m_readDate;
  // Non file part
  XString m_data;
  // File part
  FileBuffer m_file;                // File contents
  size_t     m_size   { 0      };   // Indicative!!
  // Additional headers
  HeaderMap  m_headers;
};

using MultiPartMap = std::vector<MultiPart*>;
using uchar        = unsigned char;

// The MultiPartBuffer implements the RFC 7578 form-data multiparts
// So that you can add a Content-Disposition part in a HTTP message
//
// Normally you only use AddPart/AddFile to create it, and
// you use AddToHTTPMessage before you send one through the HTTPClient

class MultiPartBuffer
{
public:
  explicit MultiPartBuffer(FormDataType p_type);
 ~MultiPartBuffer();

  void         Reset();
  bool         SetFormDataType(FormDataType p_type);
  // Creating a MultiPartBuffer
  MultiPart*   AddPart(XString p_name,XString p_contentType,XString p_data,XString p_charset = _T(""),bool p_conversion = false);
  MultiPart*   AddFile(XString p_name,XString p_contentType,XString p_filename);
  // Delete a designated part
  bool         DeletePart(XString p_name);
  bool         DeletePart(const MultiPart* p_part);
  // Getting a part of the MultiPartBuffer
  MultiPart*   GetPart(XString p_name);
  MultiPart*   GetPart(int p_index);
  size_t       GetParts();
  FormDataType GetFormDataType();
  XString      GetContentType();
  XString      GetBoundary();

  // Functions for HTTPMessage
  XString      CalculateBoundary(XString p_special = _T("#"));
  XString      CalculateAcceptHeader();
  bool         SetBoundary(XString p_boundary);
  // Re-create from an existing (incoming!) buffer
  bool         ParseBuffer(XString     p_contentType          // Content type including the 'boundary'
                          ,FileBuffer* p_buffer               // Incoming buffer with body data
                          ,bool        p_conversion = false   // Perform UTF-8 to internal character conversion
                          ,bool        p_utf16      = false); // Incoming buffer is in UTF-16 format (rare!)

  // File times & size extensions used in the Content-Disposition header
  // BEWARE: Some servers do not respect the file-times attributes
  //         or will even crash on it (WCF .NET returns HTTP status 500)
  void         SetFileExtensions(bool p_extens)    { m_extensions = p_extens; };
  bool         GetFileExtensions()                 { return m_extensions;     };
  // Use "charset" attribute in a non file buffer part
  // BEWARE: Some servers do not respect the "charset" attribute in the Content-Type
  //         or will even crash on it (WCF .NET returns HTTP status 500 on the NEXT buffer part
  void         SetUseCharset(bool p_useCharset)    { m_useCharset = p_useCharset; };
  bool         GetUseCharset()                     { return m_useCharset;         };

private:
  // Find which type of formdata we are receiving
  FormDataType FindBufferType(XString p_contentType);
  // Find the boundary in the content-type header
  XString      FindBoundaryInContentType(XString p_contentType);
  // Parsing different types
  bool         ParseBufferFormData(XString p_contentType,FileBuffer* p_buffer,bool p_conversion);
  bool         ParseBufferUrlEncoded(FileBuffer* p_buffer);
  // Finding a new partial message
  void*        FindPartBuffer(uchar*& p_vinding,size_t& p_remaining,BYTE* p_boundary,unsigned p_boundaryLength);
  XString      GetLineFromBuffer(uchar*& p_begin,const uchar* p_end);
  bool         GetHeaderFromLine   (const XString& p_line,XString& p_header,XString& p_value);
  XString      GetAttributeFromLine(const XString& p_line,XString p_name);
  void         CalculateBinaryBoundary(XString p_boundary,BYTE*& p_binary,unsigned& p_length);
    // Adding a part from a raw buffer
  void         AddRawBufferPart(uchar* p_partialBegin,const uchar* p_partialEnd,bool p_conversion);
  // Check that name is in the ASCII range for a data part
  bool         CheckName(XString p_name);

  FormDataType m_type;                  // URL-encoded or form-data
  XString      m_boundary;              // Form-Data boundary string
  XString      m_incomingCharset;       // Character set from "_charset_" part
  MultiPartMap m_parts;                 // All parts
  bool         m_extensions { false };  // Show file times & size in the header
  bool         m_useCharset { false };  // Use 'charset' in the 'Content-Type' header
  int          m_charSize   { 1     };  // BEWARE: UTF-8/ANSI/MBCS = 1, UTF-16 = 2
};

inline size_t
MultiPartBuffer::GetParts()
{
  return m_parts.size();
};

inline FormDataType
MultiPartBuffer::GetFormDataType()
{
  return m_type;
}
