/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MultiPartBuffer.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2017 ir. W.E. Huisman
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
#include "MultiPartBuffer.h"
#include "GenerateGUID.h"
#include "EnsureFile.h"
#include "HTTPTime.h"
#include "HTTPMessage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
//
// MULTIPART
//
//////////////////////////////////////////////////////////////////////////

MultiPart::MultiPart()
{
}

MultiPart::MultiPart(CString p_name,CString p_contentType)
          :m_name(p_name)
          ,m_contentType(p_contentType)
{
}

// Setting the filename and all the times of the file
bool    
MultiPart::SetFile(CString p_filename)
{
  // Initially empty file and all the times (new file!)
  m_filename.Empty();
  m_creationDate.Empty();
  m_modificationDate.Empty();
  m_readDate.Empty();

  // New filetimes
  FILETIME creation;
  FILETIME modification;
  FILETIME readtime;

  // Open file to read the filetimes from the filesystem
  HANDLE fileHandle = CreateFile(p_filename
                                ,GENERIC_READ
                                ,FILE_SHARE_READ
                                ,NULL
                                ,OPEN_EXISTING  // !!
                                ,FILE_ATTRIBUTE_NORMAL
                                ,NULL);
  if(fileHandle)
  {
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

  // Record filename and size
  EnsureFile ensure;
  m_filename = ensure.FilenamePart(p_filename);
  m_size = m_file.GetLength();

  return true;
}

bool    
MultiPart::CheckBoundaryExists(CString p_boundary)
{
  // If data message, search in the form-data message
  if(!m_data.IsEmpty())
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
    int    bdlen  = p_boundary.GetLength();

    m_file.GetBuffer(buffer,length);
    if(buffer && length > (size_t)p_boundary.GetLength())
    {
      for(char* buf = (char*)buffer; buf < (char*)((char*)buffer + length - bdlen); ++buf)
      {
        if(memcmp(buf,p_boundary,bdlen) == 0)
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
CString
MultiPart::CreateHeader(CString p_boundary,bool p_extensions /*=false*/)
{
  CString header("--");
  header += p_boundary;
  header += "\r\nContent-Disposition: form-data; ";
  header.AppendFormat("name=\"%s\"",m_name);

  // Filename attributes
  if(!m_filename.IsEmpty())
  {
    header.AppendFormat("; filename=\"%s\"",m_filename);

    // Eventually do the extensions for filetimes and size
    if(p_extensions)
    {
      if(!m_creationDate.IsEmpty())
      {
        header.AppendFormat("; creation-date=\"%s\"",m_creationDate);
      }
      if(!m_modificationDate.IsEmpty())
      {
        header.AppendFormat("; modification-date=\"%s\"",m_modificationDate);
      }
      if(!m_readDate.IsEmpty())
      {
        header.AppendFormat("; read-date=\"%s\"",m_readDate);
      }
      size_t size = m_file.GetLength();
      if(size)
      {
        header.AppendFormat("; size=\"%u\"",(unsigned)m_size);
      }
    }
  }
  // Add content type
  header += "\r\nContent-type: ";
  header += m_contentType;
  header += "\r\n\r\n";
  return header;
}

// Physically writing the file to the filesystem
bool    
MultiPart::WriteFile()
{
  bool result = false;

  // Use our filename!
  m_file.SetFileName(m_filename);
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
  // Filetime dates, and pointers to it.
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
  HANDLE fileHandle = CreateFile(m_filename
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

CString
MultiPart::FileTimeToString(PFILETIME p_filetime)
{
  CString time;
  SYSTEMTIME sysTime;
  if(FileTimeToSystemTime(p_filetime,&sysTime))
  {
    HTTPTimeFromSystemTime(&sysTime,time);
  }
  return time;
}

PFILETIME
MultiPart::FileTimeFromString(PFILETIME p_filetime,CString& p_time)
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
  while(!m_parts.empty())
  {
    MultiPart* part = m_parts.back();
    delete part;
    m_parts.pop_back();
  }
}

CString
MultiPartBuffer::GetContentType()
{
  CString type;
  switch(m_type)
  {
    case FD_URLENCODED: type = "application/x-www-form-urlencoded"; break;
    case FD_MULTIPART:  type = "multipart/form-data";               break;
    case FD_UNKNOWN:    break;
  }
  return type;
}

bool
MultiPartBuffer::SetFormDataType(FormDataType p_type)
{
  // In case we 'reset'  to url encoded
  // we cannot allow to any files entered as parts
  if(p_type == FD_URLENCODED)
  {
    for(auto& part : m_parts)
    {
      if(!part->GetFileName().IsEmpty())
      {
        return false;
      }
    }
  }
  m_type = p_type;
  return true;
}

MultiPart*   
MultiPartBuffer::AddPart(CString p_name,CString p_contentType,CString p_data)
{
  MultiPart* part = GetPart(p_name);
  if(part)
  {
    return part;
  }
  part = new MultiPart(p_name,p_contentType);
  part->SetData(p_data);

  m_parts.push_back(part);
  return part;
}

MultiPart*   
MultiPartBuffer::AddFile(CString p_name,CString p_contentType,CString p_filename)
{
  MultiPart* part = GetPart(p_name);
  if(part)
  {
    return part;
  }
  part = new MultiPart(p_name,p_contentType);
  if(part->SetFile(p_filename) == false)
  {
    delete part;
    return nullptr;
  }
  m_parts.push_back(part);
  return part;
}

MultiPart*   
MultiPartBuffer::GetPart(CString p_name)
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

MultiPart*   
MultiPartBuffer::GetPart(int p_index)
{
  if(p_index >= 0 && p_index < (int)m_parts.size())
  {
    return m_parts[p_index];
  }
  return nullptr;
}

// Calculate a new part boundary and check that it does NOT 
// exists in any of the parts of the message
CString
MultiPartBuffer::CalculateBoundary()
{
  bool exists = false;
  do
  {
    m_boundary = GenerateGUID();
    m_boundary.Replace("-","");
    m_boundary = CString("#BOUNDARY#") + m_boundary + "#";

    for(auto& part : m_parts)
    {
      if(part->CheckBoundaryExists(m_boundary))
      {
        exists = true;
        break;
      }
    }
  }
  while(exists);

  return m_boundary;
}

CString 
MultiPartBuffer::CalculateAcceptHeader()
{
  if(m_type == FD_URLENCODED)
  {
    return "text/html, application/xhtml+xml";
  }
  else if(m_type == FD_MULTIPART)
  {
    // De-double all content types of all parts
    std::map<CString,bool> types;
    for(auto& part : m_parts)
    {
      types[part->GetContentType()] = true;
    }
    // Build accept types string
    CString accept;
    for(auto& type : types)
    {
      if(!accept.IsEmpty())
      {
        accept += ",";
      }
      accept += type.first;
    }
    return accept;
  }
  // Default unknown FormData type 
  return "text/html, application/xhtml+xml, */*";
}

//////////////////////////////////////////////////////////////////////////
//
// PARSE BUFFER BACK TO MultiPartBuffer
// GENERALLY ON INCOMING TRAFFIC
//
//////////////////////////////////////////////////////////////////////////

// Re-create from an existing (incoming!) buffer
bool         
MultiPartBuffer::ParseBuffer(CString p_contentType,FileBuffer* p_buffer)
{
  // Start anew
  Reset();

  // Check that we have a buffer
  if(p_buffer == nullptr)
  {
    return false;
  }

  FormDataType type = FindBufferType(p_contentType);
  switch(type)
  {
    case FD_URLENCODED: return ParseBufferUrlEncoded(p_buffer);
    case FD_MULTIPART:  return ParseBufferFormData(p_contentType,p_buffer);
    case FD_UNKNOWN:    // Fall through
    default:            return false;
  }
  return false;
}

bool
MultiPartBuffer::ParseBufferFormData(CString p_contentType,FileBuffer* p_buffer)
{
  bool   result = false;
  uchar* buffer = nullptr;
  size_t length = 0;

  // Find the boundary between the parts
  CString boundary = FindBoundaryInContentType(p_contentType);
  if(boundary.IsEmpty())
  {
    return false;
  }

  // Get a copy of the buffer
  if(p_buffer->GetBufferCopy(buffer,length) == false)
  {
    return false;
  }

  // Cycle through the raw buffer from the HTTP message
  uchar* finding = static_cast<uchar*>(buffer);
  size_t remain  = length;
  while(true)
  {
    void* partBuffer = FindPartBuffer(finding,remain,boundary);
    if(partBuffer == nullptr)
    {
      break;
    }
    AddRawBufferPart((uchar*)partBuffer,finding);
    result = true;
  }
  // Release the buffer copy 
  delete[] buffer;
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
  CString parameters;
  char* pnt = parameters.GetBufferSetLength((int)length + 1);
  strcpy_s(pnt,length + 1,(const char*)buffer);
  pnt[length] = 0;
  parameters.ReleaseBuffer();
  delete[] buffer;

  // Find all query parameters
  int query = 1;
  while(query > 0)
  {
    // FindNext query
    query = parameters.Find('&');
    CString part;
    if(query > 0)
    {
      part = parameters.Left(query);
      parameters = parameters.Mid(query + 1);
    }
    else
    {
      part = parameters;
    }

    CString name,value;
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
    // Save as bufferpart
    AddPart(name,"text",value);
  }
  return true;
}

// Finding a new partial message
void*
MultiPartBuffer::FindPartBuffer(uchar*& p_finding,size_t& p_remaining,CString& p_boundary)
{
  void* result = nullptr;

  // Message buffer must end in "<BOUNDARY>--", so no need to seek to the exact end of the buffer
  int length = p_boundary.GetLength();
  while(p_remaining-- > (size_t)(length + 1))
  {
    if(memcmp(p_finding,(char*)p_boundary.GetString(),length) == 0)
    {
      // Positioning of the boundary found
      p_finding   += length + 2; // 2 is for cr/lf of the HTTP protocol 
      p_remaining -= length + 2; // 2 is for cr/lf of the HTTP protocol 
      result       = p_finding;

      while(p_remaining-- > (size_t)(length + 1))
      {
        if(memcmp(p_finding,(char*) p_boundary.GetString(),length) == 0)
        {
          // Two '-' signs before the boundary  
          if((*(p_finding - 1)) == '-') --p_finding;
          if((*(p_finding - 1)) == '-') --p_finding;
          // Ending of the part found
          break;
        }
        ++p_finding;
      }
      break;
    }
    ++p_finding;
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
MultiPartBuffer::AddRawBufferPart(uchar* p_partialBegin,uchar* p_partialEnd)
{
  MultiPart* part = new MultiPart();

  while(true)
  {
    CString line = GetLineFromBuffer(p_partialBegin,p_partialEnd);
    if(line.IsEmpty()) break;

    // Getting the header/value
    CString header,value;
    GetHeaderFromLine(line,header,value);

    // Finding the result for the header lines of the buffer part
    if(header.CompareNoCase("Content-Type") == 0)
    {
      part->SetContentType(value);
    }
    else if(header.CompareNoCase("Content-Disposition") == 0)
    {
      // Getting the attributes
      part->SetName            (GetAttributeFromLine(line,"name"));
      part->SetFileName        (GetAttributeFromLine(line,"filename"));
      part->SetSize       (atoi(GetAttributeFromLine(line,"size")));
      part->SetDateCreation    (GetAttributeFromLine(line,"creation-date"));
      part->SetDateModification(GetAttributeFromLine(line,"modification-date"));
      part->SetDateRead        (GetAttributeFromLine(line,"read-date"));
    }
  }

  // Getting the contents
  if(part->GetFileName().IsEmpty())
  {
    // Buffer is the data component
    CString data;
    size_t length = p_partialEnd - p_partialBegin;
    char*  buffer = data.GetBufferSetLength((int)length + 1);
    strncpy_s(buffer,length + 1,(const char*)p_partialBegin,length);
    buffer[length] = 0;
    data.ReleaseBuffer((int)length);
    part->SetData(data);
  }
  else
  {
    // Add buffer as my filebuffer in one go
    FileBuffer* buffer = part->GetBuffer();
    size_t length = p_partialEnd - p_partialBegin;
    buffer->SetBuffer(p_partialBegin,length);
  }
  // Do not forget to save this part
  m_parts.push_back(part);
}

CString
MultiPartBuffer::GetLineFromBuffer(uchar*& p_begin,uchar* p_end)
{
  CString line;

  const char* end = strchr((const char*)p_begin,'\n');
  if(end > (const char*)p_end) return line;
  size_t length = end - (const char*)p_begin;
  char* buf = line.GetBufferSetLength((int)length + 1);
  strncpy_s(buf,length+1,(const char*)p_begin,length);
  buf[length] = 0;
  line.ReleaseBuffer((int)length);
  line.TrimRight('\r');

  // Position after the end
  p_begin = (uchar*)end + 1;
  return line;
}

// Splitting a header/value pair from a line
// e.g. "Content-type: application/json;"
bool
MultiPartBuffer::GetHeaderFromLine(CString& p_line,CString& p_header,CString& p_value)
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
    p_line   = p_line.Mid(pos + 1);
    p_line.TrimLeft(' ');
    // Find header value ending (if any)
    pos = p_line.Find(';');
    if(pos > 0)
    {
      p_value = p_line.Left(pos);
    }
    else
    {
      p_value = p_line;
    }
  }
  return result;
}

CString
MultiPartBuffer::GetAttributeFromLine(CString& p_line,CString p_name)
{
  CString attribute;
  CString line(p_line);
  CString find(p_name);
  line.MakeLower();
  find += "=";

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
    attribute.Trim("\"");
  }
  return attribute;
}

// Find which type of formdata we are receiving
// content-type: multipart/form-data; boundary="--#BOUNDARY#12345678901234"
// content-type: application/x-www-form-urlencoded
FormDataType
MultiPartBuffer::FindBufferType(CString p_contentType)
{
  if(p_contentType.Find("urlencoded") > 0)
  {
    return FD_URLENCODED;
  }
  if(p_contentType.Find("form-data") > 0)
  {
    return FD_MULTIPART;
  }
  return FD_UNKNOWN;
}

// Find the boundary in the content-type header
// content-type: multipart/form-data; boundary="--#BOUNDARY#12345678901234"
CString
MultiPartBuffer::FindBoundaryInContentType(CString p_contentType)
{
  CString boundary;
  int length = p_contentType.GetLength();
  CString content(p_contentType);
  content.MakeLower();
  int pos = content.Find("boundary");
  if(pos >= 0)
  {
    pos += 8; // skip the word 'boundary"
    while(pos < length && isspace(content.GetAt(pos))) ++pos;
    if(content.GetAt(pos) == '=')
    {
      ++pos;
      while(pos < length && isspace(content.GetAt(pos))) ++pos;
      boundary = p_contentType.Mid(pos);
      boundary.Trim("\"");
    }
  }
  return boundary;
}
