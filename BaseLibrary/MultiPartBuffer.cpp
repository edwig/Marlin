/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MultiPartBuffer.cpp
//
// BaseLibrary: Indispensable general objects and functions
// 
// Created: 2014-2025 ir. W.E. Huisman
// MIT License
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
#include "pch.h"
#include "MultiPartBuffer.h"
#include "GenerateGUID.h"
#include "HTTPTime.h"
#include "HTTPMessage.h"
#include "ConvertWideString.h"

//////////////////////////////////////////////////////////////////////////
//
// MULTIPART
//
//////////////////////////////////////////////////////////////////////////

MultiPart::MultiPart()
{
}

MultiPart::MultiPart(const XString& p_name,const XString& p_contentType)
          :m_name(p_name)
          ,m_contentType(p_contentType)
{
}

// Setting the filename and all the times of the file
bool    
MultiPart::SetFile(const XString& p_filename)
{
  // Initially empty file and all the times (new file!)
  m_shortFilename.Empty();
  m_creationDate.Empty();
  m_modificationDate.Empty();
  m_readDate.Empty();

  // Open file to read the file times from the filesystem
  HANDLE fileHandle = CreateFile(p_filename
                                ,GENERIC_READ
                                ,FILE_SHARE_READ
                                ,NULL
                                ,OPEN_EXISTING  // !!
                                ,FILE_ATTRIBUTE_NORMAL
                                ,NULL);
  if(fileHandle)
  {
    // New file times
    FILETIME creation     { 0, 0 };
    FILETIME modification { 0, 0 };
    FILETIME readtime     { 0, 0 };

    if(GetFileTime(fileHandle,&creation,&readtime,&modification))
    {
      m_creationDate     = FileTimeToString(&creation);
      m_modificationDate = FileTimeToString(&modification);
      m_readDate         = FileTimeToString(&readtime);
    }
    CloseHandle(fileHandle);
  }
  else
  {
    return false;
  }

  // Now read the contents of the file in the FileBuffer
  m_file.SetFileName(p_filename);
  if(m_file.ReadFile() == false)
  {
    return false;
  }
  m_longFilename = p_filename;

  // Record filename and size in this MultiPart
  WinFile filenm(p_filename);
  m_shortFilename = XString(filenm.GetFilenamePartFilename());
  m_size = m_file.GetLength();
  filenm.SetFilename(m_shortFilename);
  m_shortFilename = filenm.GetNamePercentEncoded();
  return true;
}

bool    
MultiPart::CheckBoundaryExists(const XString& p_boundary)
{
  // If data message, search in the form-data message
  if(m_data.GetLength())
  {
    if(m_data.Find(p_boundary) >= 0)
    {
      return true;
    }
  }
  else
  {
    // Holy shit: Sequentially scan our buffer for the boundary
    uchar* buffer = nullptr;
    size_t length = 0L;

    m_file.GetBuffer(buffer,length);
    if(buffer && length > (size_t)p_boundary.GetLength())
    {
      for(char* buf = (char*)buffer; buf < (char*)((char*)buffer + length - p_boundary.GetLength()); ++buf)
      {
        if(memcmp(buf,p_boundary,p_boundary.GetLength()) == 0)
        {
          return true;
        }
      }
    }
  }
  return false;
}

// The content-disposition header is part of the content
// That's way we supply "\r\n" line breaks instead of "\n"
XString
MultiPart::CreateHeader(const XString& p_boundary,bool p_extensions /*=false*/)
{
  XString header(_T("--"));
  header += p_boundary;
  header += _T("\r\nContent-Disposition: form-data; ");
  header.AppendFormat(_T("name=\"%s\""),m_name.GetString());

  // Filename attributes
  if(!m_shortFilename.IsEmpty())
  {
    header.AppendFormat(_T("; filename=\"%s\""),m_shortFilename.GetString());

    // Eventually do the extensions for filetimes and size
    if(p_extensions)
    {
      if(!m_creationDate.IsEmpty())
      {
        header.AppendFormat(_T("; creation-date=\"%s\""),m_creationDate.GetString());
      }
      if(!m_modificationDate.IsEmpty())
      {
        header.AppendFormat(_T("; modification-date=\"%s\""),m_modificationDate.GetString());
      }
      if(!m_readDate.IsEmpty())
      {
        header.AppendFormat(_T("; read-date=\"%s\""),m_readDate.GetString());
      }
      size_t size = m_file.GetLength();
      if(size)
      {
        header.AppendFormat(_T("; size=\"%u\""),(unsigned)m_size);
      }
    }
  }
  // Add content type
  header += _T("\r\nContent-type: ");
  header += FindMimeTypeInContentType(m_contentType);
  if(!m_boundary.IsEmpty())
  {
    header += _T("; boundary=");
    header += m_boundary;
  }
  if(!m_charset.IsEmpty())
  {
    header += _T("; charset=\"");
    header += m_charset;
    header += _T("\"");
  }
  header += _T("\r\n");
  // Add variable headers
  for(const auto& head : m_headers)
  {
    header += head.first;
    header += _T(": ");
    header += head.second;
    header += _T("\r\n");
  }
  // Empty line for the body
  header += _T("\r\n");
  return header;
}

// Physically writing the file to the filesystem
bool    
MultiPart::WriteFile()
{
  bool result = false;

  // Use our filename!
  m_file.SetFileName(m_shortFilename);
  // Try to physically write the file
  result = m_file.WriteFile();
  // Forward our times
  TrySettingFiletimes();
  return result;
}

void
MultiPart::TrySettingFiletimes()
{
  // Optimize: See if we have dates
  if(m_creationDate.IsEmpty() && m_modificationDate.IsEmpty() && m_readDate.IsEmpty())
  {
    return;
  }
  // File time dates, and pointers to it.
  FILETIME  creation;
  FILETIME  modifcation;
  FILETIME  readTime;
  PFILETIME pCreation     = &creation;
  PFILETIME pModification = &modifcation;
  PFILETIME pReadTime     = &readTime;

  // Reading the filetimes from the disposition string
  pCreation     = FileTimeFromString(pCreation,    m_creationDate);
  pModification = FileTimeFromString(pModification,m_modificationDate);
  pReadTime     = FileTimeFromString(pReadTime,    m_readDate);

  // Open file to set the filetimes in the filesystem
  HANDLE fileHandle = CreateFile(m_shortFilename
                                ,FILE_WRITE_ATTRIBUTES
                                ,FILE_SHARE_READ 
                                ,NULL
                                ,OPEN_EXISTING  // !!
                                ,FILE_ATTRIBUTE_NORMAL
                                ,NULL);
  if(fileHandle)
  {
    SetFileTime(fileHandle,pCreation,pReadTime,pModification);
    CloseHandle(fileHandle);
  }
}

XString
MultiPart::FileTimeToString(PFILETIME p_filetime)
{
  XString time;
  SYSTEMTIME sysTime;
  if(FileTimeToSystemTime(p_filetime,&sysTime))
  {
    HTTPTimeFromSystemTime(&sysTime,time);
  }
  return time;
}

PFILETIME
MultiPart::FileTimeFromString(PFILETIME p_filetime,const XString& p_time)
{
  if(!p_time.IsEmpty())
  {
    SYSTEMTIME sysTime;
    if(HTTPTimeToSystemTime(p_time,&sysTime))
    {
      if(SystemTimeToFileTime(&sysTime,p_filetime))
      {
        return p_filetime;
      }
    }
  }
  return nullptr;
}

void
MultiPart::AddHeader(const XString& p_header,const XString& p_value)
{
  // Case-insensitive search!
  HeaderMap::iterator it = m_headers.find(p_header);
  if(it != m_headers.end())
  {
    // Check if we set it a duplicate time
    // If appended, we do not append it a second time
    if(it->second.Find(p_value) >= 0)
    {
      return;
    }
    // New value of the header
    it->second = p_value;
  }
  else
  {
    // Insert as a new header
    m_headers.insert(std::make_pair(p_header,p_value));
  }
}

XString 
MultiPart::GetHeader(const XString& p_header)
{
  HeaderMap::iterator it = m_headers.find(p_header);
  if(it != m_headers.end())
  {
    return it->second;
  }
  return _T("");
}

void
MultiPart::DelHeader(const XString& p_header)
{
  HeaderMap::iterator it = m_headers.find(p_header);
  if(it != m_headers.end())
  {
    m_headers.erase(it);
    return;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// MULTIPART BUFFER
//
//////////////////////////////////////////////////////////////////////////

MultiPartBuffer::MultiPartBuffer(FormDataType p_type)
                :m_type(p_type)
{
}

MultiPartBuffer::~MultiPartBuffer()
{
  Reset();
}

void
MultiPartBuffer::Reset()
{
  // Deallocate all parts
  for(auto& part : m_parts)
  {
    delete part;
  }
  m_parts.clear();
  m_boundary.Empty();
  m_incomingCharset.Empty();
  m_extensions = false;
  m_useCharset = false;
  m_charSize   = 1;
  // Leave m_type alone: otherwise create another MultiPartBuffer
}

XString
MultiPartBuffer::GetContentType()
{
  XString type;
  switch(m_type)
  {
    case FormDataType::FD_URLENCODED: type = _T("application/x-www-form-urlencoded"); break;
    case FormDataType::FD_MULTIPART:  type = _T("multipart/form-data");               break;
    case FormDataType::FD_MIXED:      type = _T("multipart/mixed");                   break;
    case FormDataType::FD_UNKNOWN:    break;
  }
  return type;
}

XString
MultiPartBuffer::GetBoundary()
{
  if(m_type == FormDataType::FD_MULTIPART ||
     m_type == FormDataType::FD_MIXED     )
  {
    return m_boundary;
  }
  return _T("");
}

bool
MultiPartBuffer::SetFormDataType(FormDataType p_type)
{
  // In case we 'reset'  to URL encoded
  // we cannot allow to any files entered as parts
  if(p_type == FormDataType::FD_URLENCODED)
  {
    for(auto& part : m_parts)
    {
      if(!part->GetShortFileName().IsEmpty())
      {
        return false;
      }
    }
  }
  m_type = p_type;
  return true;
}

// Adding a part to the multi-part buffer object
// 1) p_name        : Name of the multi-part
// 2) p_contentType : Content-type the part is in, or will be after conversion
// 3) p_data        : The actual data string to be stored
// 4) p_charset     : Character set to send it in (or is already in). Empty default = utf-8
// 5) p_conversion  : If data is not delivered in designated charset, perform the conversion
//
MultiPart*   
MultiPartBuffer::AddPart(const XString& p_name
                        ,const XString& p_contentType
                        ,const XString& p_data
                        ,const XString& p_charset    /* = ""    */
                        ,bool           p_conversion /* = false */)
{
  // Check that the name is a printable ASCII character string
  if(!CheckName(p_name))
  {
    return nullptr;
  }

  // Add the part
  MultiPart* part = alloc_new MultiPart(p_name,p_contentType);

  // See to data conversion
  XString data;
  const XString charset = p_charset;
  if(p_charset.CompareNoCase(_T("windows-1252")) && p_conversion)
  {
    data = EncodeStringForTheWire(p_data,charset);
  }
  else
  {
    data = p_data;
  }

  // Add the data to the MultiPart
  part->SetData(data);

  // Loose string parts are UTF-8 by default
  // Revert from version 6.01
  // REASON: Microsoft .NET stacks cannot handle the charset attribute and WILL crash!
  if(!charset.IsEmpty() && charset.CompareNoCase(_T("utf-8")) && m_useCharset)
  {
    part->SetCharset(charset);
  }

  // Store the MultiPart
  m_parts.push_back(part);
  return part;
}

MultiPart*   
MultiPartBuffer::AddFile(const XString& p_name,const XString& p_contentType,const XString& p_filename)
{
  // Check that the name is a printable ASCII character string
  if(!CheckName(p_name))
  {
    return nullptr;
  }

  // Create file part
  MultiPart* part = alloc_new MultiPart(p_name,p_contentType);
  if(part->SetFile(p_filename) == false)
  {
    delete part;
    return nullptr;
  }
  // Store the MultiPart
  m_parts.push_back(part);
  return part;
}

// Get part by name. Can fail as names may be duplicate
// In case of duplicate names, you WILL get the FIRST one!
MultiPart*   
MultiPartBuffer::GetPart(const XString& p_name)
{
  for(auto& part : m_parts)
  {
    if(part->GetName().CompareNoCase(p_name) == 0)
    {
      return part;
    }
  }
  return nullptr;
}

// Get part by index number.
// Can NOT fail in case of duplicate name parts
MultiPart*   
MultiPartBuffer::GetPart(int p_index)
{
  if(p_index >= 0 && p_index < (int)m_parts.size())
  {
    return m_parts[p_index];
  }
  return nullptr;
}

// Delete a designated part
bool
MultiPartBuffer::DeletePart(const XString& p_name)
{
  MultiPartMap::iterator it = m_parts.begin();
  while(it != m_parts.end())
  {
    if((*it)->GetName().Compare(p_name) == 0)
    {
      m_parts.erase(it);
      return true;
    }
    ++it;
  }
  return false;
}

bool
MultiPartBuffer::DeletePart(const MultiPart* p_part)
{
  MultiPartMap::iterator it = m_parts.begin();
  while(it != m_parts.end())
  {
    if (*it == p_part)
    {
      m_parts.erase(it);
      return true;
    }
    ++it;
  }
  return false;
}

// Calculate a new part boundary and check that it does NOT 
// exists in any of the parts of the message
XString
MultiPartBuffer::CalculateBoundary(XString p_special /*= "#" */)
{
  bool exists = false;
  do
  {
    // Create boundary by using a globally unique identifier
    m_boundary = GenerateGUID();
    m_boundary.Replace(_T("-"),_T(""));
    m_boundary = p_special + XString(_T("BOUNDARY")) + p_special + m_boundary + p_special;

    // Search all parts for the existence of the boundary
    exists = !SetBoundary(m_boundary);
  }
  while(exists);

  return m_boundary;
}

// Check that the newly found boundary does *NOT* exist
// within any of the parts of this MultiPartBuffer
bool
MultiPartBuffer::SetBoundary(const XString& p_boundary)
{
  // Search all parts for the existence of the boundary
  for(auto& part : m_parts)
  {
    if(part->CheckBoundaryExists(p_boundary))
    {
      return false;
    }
  }
  m_boundary = p_boundary;
  return true;
}

XString
MultiPartBuffer::CalculateAcceptHeader()
{
  if(m_type == FormDataType::FD_URLENCODED)
  {
    return _T("text/html, application/xhtml+xml");
  }
  else if(m_type == FormDataType::FD_MULTIPART || m_type == FormDataType::FD_MIXED)
  {
    // De-double all content types of all parts
    std::map<XString,bool> types;
    for(auto& part : m_parts)
    {
      types[part->GetContentType()] = true;
    }
    // Build accept types string
    XString accept;
    for(const auto& type : types)
    {
      if(!accept.IsEmpty())
      {
        accept += _T(",");
      }
      accept += type.first;
    }
    return accept;
  }
  // Default unknown FormData type 
  return _T("text/html, application/xhtml+xml, */*");
}

//////////////////////////////////////////////////////////////////////////
//
// PARSE BUFFER BACK TO MultiPartBuffer
// GENERALLY ON INCOMING TRAFFIC
//
//////////////////////////////////////////////////////////////////////////

// Re-create from an existing (incoming!) buffer
bool         
MultiPartBuffer::ParseBuffer(const XString& p_contentType
                            ,FileBuffer*    p_buffer
                            ,bool           p_conversion /*=false*/
                            ,bool           p_utf16      /*=false*/)
{
  // Start anew
  Reset();

  // Incoming buffer encoding
  m_charSize = p_utf16 ? 2 : 1;

  // Check that we have a buffer
  if(p_buffer == nullptr)
  {
    return false;
  }

  m_type = FindBufferType(p_contentType);
  switch(m_type)
  {
    case FormDataType::FD_URLENCODED: return ParseBufferUrlEncoded(p_buffer);
    case FormDataType::FD_MULTIPART:  [[fallthrough]];
    case FormDataType::FD_MIXED:      return ParseBufferFormData(p_contentType,p_buffer,p_conversion);
    case FormDataType::FD_UNKNOWN:    [[fallthrough]];
    default:                          return false;
  }
}

bool
MultiPartBuffer::ParseBufferFormData(const XString& p_contentType,FileBuffer* p_buffer,bool p_conversion)
{
  bool   result = false;
  uchar* buffer = nullptr;
  size_t length = 0;
  // Binary boundary
  BYTE*    boundaryBinary = nullptr;
  unsigned boundaryLength = 0;

  // Find the boundary between the parts
  m_boundary = FindFieldInHTTPHeader(p_contentType,_T("boundary"));
  if(m_boundary.IsEmpty())
  {
    return false;
  }

  // Get a copy of the buffer
  if(p_buffer->GetBufferCopy(buffer,length) == false)
  {
    return false;
  }

  // Getting a binary boundary in MBCS/UTF-16 configuration
  CalculateBinaryBoundary(m_boundary,boundaryBinary,boundaryLength);

  // Cycle through the raw buffer from the HTTP message
  uchar* finding = buffer;
  size_t  remain = length;
  while(true)
  {
    void* partBuffer = FindPartBuffer(finding,remain,boundaryBinary,boundaryLength);
    if(partBuffer == nullptr)
    {
      break;
    }
    AddRawBufferPart(reinterpret_cast<uchar*>(partBuffer),finding,p_conversion);
    result = true;
  }
  // Release the buffer copy and the boundary
  delete[] buffer;
  delete[] boundaryBinary;
  return result;
}

bool
MultiPartBuffer::ParseBufferUrlEncoded(FileBuffer* p_buffer)
{
  uchar* buffer = nullptr;
  size_t length = 0;

  // Getting the payload as a string
  if(p_buffer->GetBufferCopy(buffer,length) == false)
  {
    return false;
  }
  XString parameters;
  TCHAR* pnt = parameters.GetBufferSetLength((int)length + 1);
  _tcscpy_s(pnt,length + 1,reinterpret_cast<const TCHAR*>(buffer));
  pnt[length] = 0;
  parameters.ReleaseBuffer();
  delete[] buffer;

  // Find all query parameters
  int query = 1;
  while(query > 0)
  {
    // FindNext query
    query = parameters.Find('&');
    XString part;
    if(query > 0)
    {
      part = parameters.Left(query);
      parameters = parameters.Mid(query + 1);
    }
    else
    {
      part = parameters;
    }

    XString name,value;
    int pos = part.Find('=');
    if(pos > 0)
    {
      name  = CrackedURL::DecodeURLChars(part.Left(pos));
      value = CrackedURL::DecodeURLChars(part.Mid(pos + 1),true);
    }
    else
    {
      // No value. Use as key only
      name = CrackedURL::DecodeURLChars(part);
    }
    // Save as buffer part
    AddPart(name,_T("text"),value);
  }
  return true;
}

// Create a boundary representation in a new allocated BYTE buffer
// So we can scan for UTF-16 or ANSI/MBCS in either configuration
void
MultiPartBuffer::CalculateBinaryBoundary(XString p_boundary,BYTE*& p_binary,unsigned& p_length)
{
  // Message buffer must end in "<BOUNDARY>--", so no need to seek to the exact end of the buffer
  // Beware of WebKit (Chrome) that will send "<BOUNDARY>--\r\n" at the end of the buffer
  int length = p_boundary.GetLength();
  p_binary = nullptr;
#ifdef _UNICODE
  if(m_charSize == 2)
  {
    length *= 2;
    p_binary = alloc_new BYTE[length + 2];
    memcpy(p_binary,p_boundary.GetString(),length + 2);
  }
  else // m_charSize == 1
  {
    // Implode
    p_binary = alloc_new BYTE[length + 2];
    ImplodeString(p_boundary,p_binary,(unsigned) length);
  }
#else
  if(m_charSize == 2)
  {
    // Explode
    length *= 2;
    p_binary = alloc_new BYTE[length + 2];
    ExplodeString(p_boundary,p_binary,(unsigned) (length + 2));
  }
  else // m_charSize == 1
  {
    p_binary = alloc_new BYTE[length + 2];
    memcpy(p_binary,p_boundary.GetString(),length);
  }
#endif
  // Zero terminate the string (to be sure) also for UTF-16
  p_binary[length    ] = 0;
  p_binary[length + 1] = 0;
  // New length of the binary (could be doubled)
  p_length = length;
}

// Finding a new partial message in the buffer
// Looks for the NEXT occurrence of the boundary
void*
MultiPartBuffer::FindPartBuffer(uchar*& p_finding,size_t& p_remaining,BYTE* p_boundary,unsigned p_boundaryLength)
{
  void* result = nullptr;

  while(p_remaining > (p_boundaryLength + 4 * m_charSize))
  {
    p_remaining -= m_charSize;
    if(memcmp(p_finding,p_boundary,p_boundaryLength) == 0)
    {
      // Positioning of the boundary found
      p_finding   += p_boundaryLength + 2 * m_charSize; // 2 is for CR/LF of the HTTP protocol 
      p_remaining -= p_boundaryLength + 2 * m_charSize; // 2 is for CR/LF of the HTTP protocol 
      result       = p_finding;

      while(p_remaining)
      {
        if(memcmp(p_finding,p_boundary,p_boundaryLength) == 0)
        {
          // Two '-' signs before the boundary  
          if((*(p_finding - m_charSize)) == '-') p_finding -= m_charSize;
          if((*(p_finding - m_charSize)) == '-') p_finding -= m_charSize;
          // Ending of the part found
          break;
        }
        p_finding   += m_charSize;
        p_remaining -= m_charSize;
      }
      break;
    }
    p_finding += m_charSize;
  }
  return result;
}

// Adding a part from a raw buffer:
// --#BOUNDARY#12345678901234
// Content-Disposition: form-data; name="json"
// Content-type: application/json
// 
// {
//   ....JSON Message in here
// }
// --#BOUNDARY#12345678901234
// Content-Disposition: form-data; name="file"; filename="012345678A.rtf"; size=50123; creation-date="2016-03-17T07:42:12+0100"; modification-date=""; read-date="";
// Content-Type: text/rtf
// 
// {rtf\p abcdefghijklmnop ......   
// ... rtf file contents here
// }
// --#BOUNDARY#12345678901234
void
MultiPartBuffer::AddRawBufferPart(uchar* p_partialBegin,const uchar* p_partialEnd,bool p_conversion)
{
  MultiPart* part = alloc_new MultiPart();
  XString charset,boundary;

  while(true)
  {
    XString line = GetLineFromBuffer(p_partialBegin,p_partialEnd);
    if(line.IsEmpty()) break;

    // Getting the header/value
    XString header,value;
    GetHeaderFromLine(line,header,value);

    // Finding the result for the header lines of the buffer part
    if(header.Left(12).CompareNoCase(_T("Content-Type")) == 0)
    {
      charset  = FindFieldInHTTPHeader(value,_T("charset"));
      boundary = FindFieldInHTTPHeader(value,_T("boundary"));
      part->SetContentType(value);
      part->SetCharset(charset);
      part->SetBoundary(boundary);

      // In case we have no charset in the Content-Type and we already
      // saw a incoming MultiPart with the name "_charset_"
      if(charset.IsEmpty() && !m_incomingCharset.IsEmpty())
      {
        charset = m_incomingCharset;
      }
    }
    else if(header.CompareNoCase(_T("Content-Disposition")) == 0)
    {
      // Getting the attributes
      part->SetName            (GetAttributeFromLine(line,_T("name")));
      part->SetFileName        (GetAttributeFromLine(line,_T("filename")));
      part->SetSize      (_ttoi(GetAttributeFromLine(line,_T("size"))));
      part->SetDateCreation    (GetAttributeFromLine(line,_T("creation-date")));
      part->SetDateModification(GetAttributeFromLine(line,_T("modification-date")));
      part->SetDateRead        (GetAttributeFromLine(line,_T("read-date")));
    }
    else
    {
      // Retain the header. Not directly known in this protocol
      part->AddHeader(header,value);
    }
  }

  // RFC 7578: No charset = conversion to UTF-8
  // UTF-8 is the default conversion of the conversion routines
  // so an empty charset means: convert to UTF-8
  if(charset.CompareNoCase(_T("windows-1252")) != 0)
  {
    p_conversion = true;
  }

  // Getting the contents
  if(part->GetShortFileName().IsEmpty())
  {
    // PART
    // Buffer is the data component

    XString data;

    if(m_charSize == 1)
    {
#ifdef _UNICODE
      size_t length = (p_partialEnd - p_partialBegin);
      data = ExplodeString(p_partialBegin,(unsigned)length);
#else
      size_t length = (p_partialEnd - p_partialBegin);
      char* pnt = data.GetBufferSetLength((int)length + 1);
      strncpy_s(pnt,length + 1,(char*)p_partialBegin,length);
      data.ReleaseBufferSetLength((int)length);
#endif
    }
    else
    {
#ifdef _UNICODE
      size_t length = (p_partialEnd - p_partialBegin) / m_charSize;
      PTCHAR buffer = data.GetBufferSetLength((int) length + 1);
      _tcsncpy_s(buffer,length + 1,reinterpret_cast<const PTCHAR>(p_partialBegin),length);
      buffer[length] = 0;
      data.ReleaseBuffer((int) length);
#else
      size_t length = (p_partialEnd - p_partialBegin);
      data = ImplodeString(p_partialBegin,(unsigned)length);
#endif
    }
    // Decoding the string, possible changing the length
    if(p_conversion)
    {
      data = DecodeStringFromTheWire(data,charset);
    }
    // Place in MultiPart
    part->SetData(data);

    // Special charset convention on incoming messages
    if(part->GetName().CompareNoCase(_T("_charset_")) == 0)
    {
      m_incomingCharset = data;
    }
  }
  else
  {
    // FILE
    // Add buffer as my file buffer in one go
    FileBuffer* buffer = part->GetBuffer();
    size_t length = p_partialEnd - p_partialBegin;

    // Place in file buffer
    buffer->SetBuffer(p_partialBegin,length);
  }
  // Do not forget to save this part
  m_parts.push_back(part);
}

XString
MultiPartBuffer::GetLineFromBuffer(uchar*& p_begin,const uchar* p_end)
{
  XString line;
  const uchar* end = nullptr;

  if(m_charSize == 1)
  {
    end = (uchar*) strchr((const char*) p_begin,'\n');
    if(!end || end > p_end)
    {
      return line;
    }
    size_t length = end - p_begin;

#ifdef _UNICODE
    line = ExplodeString(p_begin,(unsigned)length);
#else
    char* buf = line.GetBufferSetLength((int) length + 1);
    strncpy_s(buf,length + 1,reinterpret_cast<const char*>(p_begin),length);
    buf[length] = 0;
    line.ReleaseBuffer(static_cast<int>(length));
#endif
  }
  else // UTF-16
  {
    end = (uchar*) wcschr((const wchar_t*) p_begin,_T('\n'));
    if(!end || end > p_end)
    {
      return line;
    }
#ifdef _UNICODE
    size_t length = (end - p_begin) / m_charSize;
    PTCHAR buf = line.GetBufferSetLength((int) length + 1);
    _tcsncpy_s(buf,length + 1,reinterpret_cast<const PTCHAR>(p_begin),length);
    buf[length] = 0;
    line.ReleaseBuffer(static_cast<int>(length));
#else
    size_t length = end - p_begin;
    line = ImplodeString(p_begin,(unsigned)length);
#endif
  }
  line = line.TrimRight(_T('\r'));

  // Position after the end
  p_begin = (uchar*)end + m_charSize;
  return line;
}

// Splitting a header/value pair from a line
// e.g. "Content-type: application/json;"
bool
MultiPartBuffer::GetHeaderFromLine(const XString& p_line,XString& p_header,XString& p_value)
{
  // Reset
  bool result = false;
  p_header.Empty();
  p_value.Empty();

  // Find header field
  int pos = p_line.Find(':');
  if(pos > 0)
  {
    p_header = p_line.Left(pos);
    p_value  = p_line.Mid(pos + 1);
    p_value.TrimLeft(' ');
//     // Find header value ending (if any)
//     pos = p_line.Find(';');
//     if(pos > 0)
//     {
//       p_value = p_line.Left(pos);
//     }
//     else
//     {
//       p_value = p_line;
//     }
  }
  return result;
}

XString
MultiPartBuffer::GetAttributeFromLine(const XString& p_line,const XString& p_name)
{
  XString attribute;
  XString line(p_line);
  XString find(p_name);
  line.MakeLower();
  find += _T("=");

  int pos = line.Find(find);
  if(pos >= 0)
  {
    int from = pos + find.GetLength();
    int end  = line.Find(';',from);
    if(end > 0)
    {
      attribute = p_line.Mid(from,end - from);
    }
    else
    {
      attribute = p_line.Mid(from);
    }
    attribute.Trim(_T("\""));
  }
  return attribute;
}

// Find which type of FormData we are receiving
// content-type: multipart/form-data; boundary="--#BOUNDARY#12345678901234"
// content-type: multipart/mixed; boundary="--#BOUNDARY#12345678901234"
// content-type: application/x-www-form-urlencoded
FormDataType
MultiPartBuffer::FindBufferType(const XString& p_contentType)
{
  if(p_contentType.Find(_T("urlencoded")) > 0)
  {
    return FormDataType::FD_URLENCODED;
  }
  if(p_contentType.Find(_T("form-data")) > 0)
  {
    return FormDataType::FD_MULTIPART;
  }
  if(p_contentType.Find(_T("mixed")) > 0)
  {
    return FormDataType::FD_MIXED;
  }
  return FormDataType::FD_UNKNOWN;
}

// Find the boundary in the content-type header
// content-type: multipart/form-data; boundary="--#BOUNDARY#12345678901234"
XString
MultiPartBuffer::FindBoundaryInContentType(const XString& p_contentType)
{
  XString boundary;
  XString content(p_contentType);
  content.MakeLower();
  int pos = content.Find(_T("boundary"));
  if(pos >= 0)
  {
    pos += 8; // skip the word 'boundary"
    int length = p_contentType.GetLength();
    while(pos < length && isspace(content.GetAt(pos))) ++pos;
    if(content.GetAt(pos) == '=')
    {
      ++pos;
      while(pos < length && isspace(content.GetAt(pos))) ++pos;
      boundary = p_contentType.Mid(pos);
      boundary.Trim(_T("\""));
    }
  }
  return boundary;
}

// Check that name is in the printable ASCII range for a data part
bool
MultiPartBuffer::CheckName(const XString& p_name)
{
  for(int index = 0;index < p_name.GetLength(); ++index)
  {
    if(!isprint(p_name.GetAt(index)))
    {
      return false;
    }
  }
  return true;
}
