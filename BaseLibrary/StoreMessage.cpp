/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: StoreMessage.cpp
//
// BaseLibrary: Indispensable general objects and functions
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
#include "pch.h"
#include "StoreMessage.h"
#include "HTTPMessage.h"
#include "GetLastErrorAsString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

StoreMessage::StoreMessage()
{
}

StoreMessage::StoreMessage(XString p_filename)
             :m_filename(p_filename)
{
}

StoreMessage::~StoreMessage()
{
  CloseFile();
}

void
StoreMessage::SetFilename(XString p_filename)
{
  m_filename = p_filename;
}

XString 
StoreMessage::GetFilename()
{
  return m_filename;
}

errno_t
StoreMessage::GetError()
{
  return m_error;
}

bool
StoreMessage::StoreIncomingMessage(HTTPMessage* p_message)
{
  bool result = false;
  m_error = fopen_s(&m_file,m_filename,"wb+");
  if(m_error)
  {
    return result;
  }

  try
  {
    WriteVersion();
    WriteResponseOffset(0);
    StoreMessagePart(p_message);
    // Now we know the length and rewrite the offset
    ReWriteOffset();

    // Reached the end successfully
    result = true;
  }
  catch(StdException& e)
  {
    if(m_error == 0)
    {
      m_error = e.GetApplicationCode();
    }
  }
  CloseFile();
  return result;
}

bool
StoreMessage::StoreResponseMessage(HTTPMessage* p_message)
{
  bool result = false;
  m_error = fopen_s(&m_file,m_filename,"rb+");
  if(m_error)
  {
    return result;
  }

  try
  {
    ReadVersion();
    SkipToResponse(false);
    StoreMessagePart(p_message);

    result = true;
  }
  catch(StdException& e)
  {
    if(m_error == 0)
    {
      m_error = e.GetApplicationCode();
    }
  }
  CloseFile();
  return result;
}

HTTPMessage* 
StoreMessage::ReadIncomingMessage()
{
  m_error = fopen_s(&m_file,m_filename,"rb");
  if(m_error)
  {
    return nullptr;
  }
  HTTPMessage* msg = new HTTPMessage();

  try
  {
    // Simply read past these fields
    ReadVersion();
    ReadResponseOffset();
    // Read the message
    ReadMessagePart(msg);
  }
  catch(StdException& e)
  {
    if(m_error == 0)
    {
      m_error = e.GetApplicationCode();
    }
    delete msg;
    msg = nullptr;
  }
  CloseFile();
  return msg;
}

HTTPMessage* 
StoreMessage::ReadResponseMessage()
{
  m_error = fopen_s(&m_file,m_filename,"rb+");
  if(m_error)
  {
    return nullptr;
  }
  HTTPMessage* msg = new HTTPMessage();

  try
  {
    // Skip past the incoming message part
    ReadVersion();
    SkipToResponse(true);
    // Read the response
    ReadMessagePart(msg);
  }
  catch(StdException& e)
  {
    if(m_error == 0)
    {
      m_error = e.GetApplicationCode();
    }
    delete msg;
    msg = nullptr;
  }
  CloseFile();
  return msg;
}

XString 
StoreMessage::GetErrorMessage()
{
  XString error;
  if(m_error < 0 && m_error > ERROR_FT_LAST)
  {
    switch(m_error)
    {
      case ERROR_FT_VERSION:        error = "Not a HTTPMessage storage version number";       break;
      case ERROR_FT_WRONGVERSION:   error = "Wrong HTTPMessage storage version number";       break;
      case ERROR_FT_RESPONSEOFFSET: error = "Not a HTTPMessage storage response offset field";break;
      case ERROR_FT_UNKNOWNFIELD:   error = "Not a known HTTP field member (higher version?)";break;
      case ERROR_FT_BODY:           error = "While reading the HTTP body (wrong length?)";    break;
      case ERROR_FT_ENDMARKER:      error = "Not a HTTPMessage ending marker";                break;
      case ERROR_FT_RESPONSE:       error = "Cannot skip to HTTPMessage response";            break;
      case ERROR_FT_NOFILE:         error = "Cannot skip to EOF of HTTPMessage";              break;
      case ERROR_FT_NORESPONSE:     error = "HTTPMessage file has no response part";          break;
    }
  }
  else
  {
    error = GetLastErrorAsString(m_error);
  }
  return error;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

void
StoreMessage::CloseFile()
{
  if(m_file)
  {
    fclose(m_file);
    m_file = nullptr;
  }
}

// Go back to after the version number and re-write the offset 
// where the response message will be written
void
StoreMessage::ReWriteOffset()
{
  long  position = ftell(m_file);
  __int64 offset = STORE_HTTP_RESPONSE_OFFSET;
  const fpos_t* off = &offset;
  if(fsetpos(m_file,off))
  {
    throw StdException(GetLastError());
  }
  WriteResponseOffset(position);
}

// Find the offset for the incoming message
// And continue reading there
void
StoreMessage::SkipToResponse(bool p_checkPresence)
{
  // Reread the offset first
  __int64 offset = 0L;
  const fpos_t* off = &offset;
  offset = ReadResponseOffset();

  // See if we are *NOT* at the EOF
  if(p_checkPresence)
  {
    // Check the length of the file
    if(fseek(m_file,0,SEEK_END))
    {
      throw StdException(ERROR_FT_NOFILE);
    }
    long total = ftell(m_file);

    // Check if offset is not at the EOF
    if((long)offset >= total)
    {
      throw StdException(ERROR_FT_NORESPONSE);
    }
  }

  // Skipping to the response part
  if(fsetpos(m_file,off))
  {
    throw StdException(ERROR_FT_RESPONSE);
  }
}

// Called from within StoreIncomingMessage and StoreResponseMessage
// Called from within an try..catch
void
StoreMessage::StoreMessagePart(HTTPMessage* p_message)
{
  // In version 1.1 we simply write all fields
  WriteHTTPCommand   (p_message->GetCommand());
  WriteURL           (p_message->GetCrackedURL().SafeURL());
  WriteStatus        (p_message->GetStatus());
  WriteContentType   (p_message->GetContentType());
  WriteContentLength (p_message->GetContentLength());
  WriteAcceptEncoding(p_message->GetAcceptEncoding());
  WriteVerbTunnel    (p_message->GetVerbTunneling());
  WriteSendBOM       (p_message->GetSendBOM());
  WriteCookies       (p_message->GetCookies());
  WriteReferrer      (p_message->GetReferrer());
  WriteDesktop       (p_message->GetRemoteDesktop());
  WriteHeaders       (p_message->GetHeaderMap());
  WriteRouting       (p_message->GetRouting());
  WriteIsModified    (p_message->GetUseIfModified());
  WriteSystemTime    (p_message->GetSystemTime());
  WriteBody          (p_message->GetFileBuffer());
  WriteEndMarker();
}

// Called from within ReadIncomingMessage and ReadResponseMessage
void
StoreMessage::ReadMessagePart(HTTPMessage* p_message)
{
  MSGFieldType type = MSGFieldType::FT_VERSION;
  do 
  {
    type = ReadHeader();
    switch(type)
    {
      case MSGFieldType::FT_HTTPCOMMAND:      ReadHTTPCommand(p_message);     break;
      case MSGFieldType::FT_URL:              ReadURL(p_message);             break;
      case MSGFieldType::FT_HTTPSTATUS:       ReadStatus(p_message);          break;
      case MSGFieldType::FT_CONTENTTYPE:      ReadContentType(p_message);     break;
      case MSGFieldType::FT_CONTENTLENGTH:    ReadContentLength(p_message);   break;
      case MSGFieldType::FT_ACCEPTENCODING:   ReadAcceptEncoding(p_message);  break;
      case MSGFieldType::FT_VERBTUNNEL:       ReadVerbTunnel(p_message);      break;
      case MSGFieldType::FT_SENDBOM:          ReadSendBOM(p_message);         break;
      case MSGFieldType::FT_COOKIES:          ReadCookies(p_message);         break;
      case MSGFieldType::FT_REFERRER:         ReadReferrer(p_message);        break;
      case MSGFieldType::FT_DESKTOP:          ReadDesktop(p_message);         break;
      case MSGFieldType::FT_HEADERS:          ReadHeaders(p_message);         break;
      case MSGFieldType::FT_ROUTING:          ReadRouting(p_message);         break;
      case MSGFieldType::FT_ISMODIFIED:       ReadIsModified(p_message);      break;
      case MSGFieldType::FT_SYSTEMTIME:       ReadSystemTime(p_message);      break;
      case MSGFieldType::FT_BODY:             ReadBody(p_message);            break;
      case MSGFieldType::FT_ENDMARKER:        ReadEndMarker();                break;
      default: throw StdException(ERROR_FT_UNKNOWNFIELD);
    }
  } 
  while(type != MSGFieldType::FT_ENDMARKER);
}

//////////////////////////////////////////////////////////////////////////
//
// WRITING
//
void
StoreMessage::WriteHeader(MSGFieldType p_type)
{
  uchar buffer = (uchar)p_type;
  if(fwrite(&buffer,1,sizeof(uchar),m_file) != sizeof(uchar))
  {
    throw StdException(GetLastError());
  }
}

void
StoreMessage::WriteNumber8(unsigned char p_number)
{
  if(fwrite(&p_number,1,sizeof(uchar),m_file) != sizeof(uchar))
  {
    throw StdException(GetLastError());
  }
}

void
StoreMessage::WriteNumber16(unsigned int p_number)
{
  ushort buffer = (ushort)p_number;
  if(fwrite(&buffer,1,sizeof(ushort),m_file) != sizeof(ushort))
  {
    throw StdException(GetLastError());
  }
}

void
StoreMessage::WriteNumber32(unsigned int p_number)
{
  if(fwrite(&p_number,1,sizeof(unsigned),m_file) != sizeof(unsigned))
  {
    throw StdException(GetLastError());
  }
}

void
StoreMessage::WriteNumber64(unsigned __int64 p_number)
{
  if(fwrite(&p_number,1,sizeof(unsigned __int64),m_file) != sizeof(unsigned __int64))
  {
    throw StdException(GetLastError());
  }
}

void
StoreMessage::WriteString(XString p_string)
{
  WriteNumber32((unsigned)p_string.GetLength());
  if(p_string.GetLength())
  {
    if(fwrite(p_string.GetString(),1,p_string.GetLength(),m_file) != (unsigned) p_string.GetLength())
    {
      throw StdException(GetLastError());
    }
  }
}

void
StoreMessage::WriteVersion()
{
  WriteHeader(MSGFieldType::FT_VERSION);
  WriteNumber16(HTTP_FILE_VERSION);
}

void
StoreMessage::WriteResponseOffset(DWORD p_offset)
{
  WriteHeader(MSGFieldType::FT_RESPONSE_OFFSET);
  WriteNumber32(p_offset);
}

void
StoreMessage::WriteHTTPCommand(HTTPCommand p_command)
{
  WriteHeader(MSGFieldType::FT_HTTPCOMMAND);
  WriteNumber16((unsigned)p_command);
}

void
StoreMessage::WriteURL(XString p_url)
{
  WriteHeader(MSGFieldType::FT_URL);
  WriteString(p_url);
}

void
StoreMessage::WriteStatus(unsigned p_status)
{
  WriteHeader(MSGFieldType::FT_HTTPSTATUS);
  WriteNumber16(p_status);
}

void    
StoreMessage::WriteContentType(XString p_type)
{
  WriteHeader(MSGFieldType::FT_CONTENTTYPE);
  WriteString(p_type);
}

void
StoreMessage::WriteContentLength(size_t p_length)
{
  WriteHeader(MSGFieldType::FT_CONTENTLENGTH);
  WriteNumber64(p_length);
}

void
StoreMessage::WriteAcceptEncoding(XString p_encoding)
{
  WriteHeader(MSGFieldType::FT_ACCEPTENCODING);
  WriteString(p_encoding);
}

void
StoreMessage::WriteVerbTunnel(bool p_verb)
{
  WriteHeader(MSGFieldType::FT_VERBTUNNEL);
  WriteNumber8(p_verb);
}

void
StoreMessage::WriteSendBOM(bool p_bom)
{
  WriteHeader(MSGFieldType::FT_SENDBOM);
  WriteNumber8(p_bom);
}

void
StoreMessage::WriteCookies(Cookies& p_cookies)
{
  WriteHeader(MSGFieldType::FT_COOKIES);
  WriteNumber16((short)p_cookies.GetSize());
  for(size_t index = 0;index < p_cookies.GetSize();++index)
  {
    Cookie* biscuit = p_cookies.GetCookie((unsigned)index);
    WriteString(biscuit->GetSetCookieText());
  }
}

void
StoreMessage::WriteReferrer(XString p_referrer)
{
  WriteHeader(MSGFieldType::FT_REFERRER);
  WriteString(p_referrer);
}

void
StoreMessage::WriteDesktop(unsigned p_desktop)
{
  WriteHeader(MSGFieldType::FT_DESKTOP);
  WriteNumber32(p_desktop);
}

void
StoreMessage::WriteHeaders(HeaderMap* p_headers)
{
  WriteHeader(MSGFieldType::FT_HEADERS);
  WriteNumber16((short)p_headers->size());
  for(auto& header : *p_headers)
  {
    WriteString(header.first);
    WriteString(header.second);
  }
}

void
StoreMessage::WriteRouting(Routing& p_routing)
{
  WriteHeader(MSGFieldType::FT_ROUTING);
  WriteNumber16((short)p_routing.size());
  for(auto& route : p_routing)
  {
    WriteString(route);
  }
}

void
StoreMessage::WriteIsModified(bool p_isModified)
{
  WriteHeader(MSGFieldType::FT_ISMODIFIED);
  WriteNumber8(p_isModified);
}

void
StoreMessage::WriteSystemTime(PSYSTEMTIME p_systemtime)
{
  WriteHeader(MSGFieldType::FT_SYSTEMTIME);
  WriteNumber16(p_systemtime->wYear);
  WriteNumber16(p_systemtime->wMonth);
  WriteNumber16(p_systemtime->wDayOfWeek);
  WriteNumber16(p_systemtime->wDay);
  WriteNumber16(p_systemtime->wHour);
  WriteNumber16(p_systemtime->wMinute);
  WriteNumber16(p_systemtime->wSecond);
  WriteNumber16(p_systemtime->wMilliseconds);
}

void
StoreMessage::WriteBody(FileBuffer* p_buffer)
{
  WriteHeader(MSGFieldType::FT_BODY);
  
  uchar* buffer = nullptr;
  size_t length = 0;

  // Get a copy to overcome buffer defragmentation!
  if(p_buffer->GetBufferCopy(buffer,length))
  {
    WriteNumber64(length);
    if(length > 0)
    {
      if(fwrite(buffer,1,length,m_file) != length)
      {
        delete[] buffer;
        throw StdException(GetLastError());
      }
    }
  }
  else
  {
    WriteNumber64(0L);
  }
  // Dispose of the buffer
  delete [] buffer;
}

void
StoreMessage::WriteEndMarker()
{
  WriteHeader(MSGFieldType::FT_ENDMARKER);
  WriteNumber16(0xFFFF);
}

//////////////////////////////////////////////////////////////////////////
//
// READING
//
MSGFieldType
StoreMessage::ReadHeader()
{
  uchar buffer = 0;
  if(fread(&buffer,1,sizeof(uchar),m_file) != sizeof(uchar))
  {
    throw StdException(GetLastError());
  }
  return (MSGFieldType)buffer;
}

unsigned char
StoreMessage::ReadNumber8()
{
  uchar buffer = 0;
  if(fread(&buffer,1,sizeof(uchar),m_file) != sizeof(uchar))
  {
    throw StdException(GetLastError());
  }
  return buffer;
}

unsigned int
StoreMessage::ReadNumber16()
{
  ushort buffer = 0;
  if(fread(&buffer,1,sizeof(ushort),m_file) != sizeof(ushort))
  {
    throw StdException(GetLastError());
  }
  return (int) buffer;
}

unsigned int
StoreMessage::ReadNumber32()
{
  unsigned int buffer = 0;
  if(fread(&buffer,1,sizeof(unsigned),m_file) != sizeof(unsigned))
  {
    throw StdException(GetLastError());
  }
  return buffer;
}

unsigned __int64
StoreMessage::ReadNumber64()
{
  unsigned __int64 buffer = 0;
  if(fread(&buffer,1,sizeof(unsigned __int64),m_file) != sizeof(unsigned __int64))
  {
    throw StdException(GetLastError());
  }
  return buffer;
}

XString
StoreMessage::ReadString()
{
  XString string;
  int length = ReadNumber32();
  if(length)
  {
    char* buffer = string.GetBufferSetLength(length + 1);
    if(fread(buffer,1,length,m_file) != (unsigned) length)
    {
      throw StdException(GetLastError());
    }
    buffer[length] = 0;
    string.ReleaseBufferSetLength(length + 1);
  }
  return string;
}

int
StoreMessage::ReadVersion()
{
  MSGFieldType type = ReadHeader();
  if(type != MSGFieldType::FT_VERSION)
  {
    throw StdException(ERROR_FT_VERSION);
  }
  int version = ReadNumber16();
  if(version != HTTP_FILE_VERSION)
  {
    throw StdException(ERROR_FT_WRONGVERSION);
  }
  return version;
}

DWORD
StoreMessage::ReadResponseOffset()
{
  MSGFieldType type = ReadHeader();
  if(type != MSGFieldType::FT_RESPONSE_OFFSET)
  {
    throw StdException(ERROR_FT_RESPONSEOFFSET);
  }
  return (DWORD)ReadNumber32();
}

void
StoreMessage::ReadHTTPCommand(HTTPMessage* p_msg)
{
  HTTPCommand command = (HTTPCommand) ReadNumber16();
  p_msg->SetCommand(command);
}

void
StoreMessage::ReadURL(HTTPMessage* p_msg)
{
  XString url = ReadString();
  p_msg->SetURL(url);
}

void
StoreMessage::ReadStatus(HTTPMessage* p_msg)
{
  int status = ReadNumber16();
  p_msg->SetStatus(status);
}

void
StoreMessage::ReadContentType(HTTPMessage* p_msg)
{
  XString type = ReadString();
  p_msg->SetContentType(type);
}

void 
StoreMessage::ReadContentLength(HTTPMessage* p_msg)
{
  size_t length = (size_t) ReadNumber64();
  p_msg->SetContentLength(length);
}

void
StoreMessage::ReadAcceptEncoding(HTTPMessage* p_msg)
{
  XString accept = ReadString();
  p_msg->SetAcceptEncoding(accept);
}

void
StoreMessage::ReadVerbTunnel(HTTPMessage* p_msg)
{
  bool verbTunneling = (bool) ReadNumber8();
  p_msg->SetVerbTunneling(verbTunneling);
}

void
StoreMessage::ReadSendBOM(HTTPMessage* p_msg)
{
  bool sendBom = (bool)ReadNumber8();
  p_msg->SetSendBOM(sendBom);
}

void
StoreMessage::ReadCookies(HTTPMessage* p_msg)
{
  int size = ReadNumber16();
  for(int index = 0;index < size; ++index)
  {
    XString biscuit = ReadString();
    p_msg->SetCookie(biscuit);
  }
}

void
StoreMessage::ReadReferrer(HTTPMessage* p_msg)
{
  XString referrer = ReadString();
  p_msg->SetReferrer(referrer);
}

void
StoreMessage::ReadDesktop(HTTPMessage* p_msg)
{
  int desktop = ReadNumber32();
  p_msg->SetRemoteDesktop(desktop);
}

void
StoreMessage::ReadHeaders(HTTPMessage* p_msg)
{
  int size = ReadNumber16();
  for(int index = 0;index < size; ++index)
  {
    XString name  = ReadString();
    XString value = ReadString();
    p_msg->AddHeader(name,value);
  }
}

void
StoreMessage::ReadRouting(HTTPMessage* p_msg)
{
  int size = ReadNumber16();
  for(int index = 0;index < size; ++index)
  {
    XString route = ReadString();
    p_msg->AddRoute(route);
  }
}

void
StoreMessage::ReadIsModified(HTTPMessage* p_msg)
{
  bool modified = (bool)ReadNumber8();
  p_msg->SetUseIfModified(modified);
}

void
StoreMessage::ReadSystemTime(HTTPMessage* p_msg)
{
  SYSTEMTIME systime;
  systime.wYear         = (WORD)ReadNumber16();
  systime.wMonth        = (WORD)ReadNumber16();
  systime.wDayOfWeek    = (WORD)ReadNumber16();
  systime.wDay          = (WORD)ReadNumber16();
  systime.wHour         = (WORD)ReadNumber16();
  systime.wMinute       = (WORD)ReadNumber16();
  systime.wSecond       = (WORD)ReadNumber16();
  systime.wMilliseconds = (WORD)ReadNumber16();
  p_msg->SetSystemTime(systime);
}

void
StoreMessage::ReadBody(HTTPMessage* p_msg)
{
  size_t length = (size_t) ReadNumber64();
  if(length)
  {
    unsigned char* buffer = new unsigned char[length];
    if(fread(buffer,1,length,m_file) != length)
    {
      throw StdException(ERROR_FT_BODY);
    }
    p_msg->SetBody(buffer,(unsigned)length);
    delete[] buffer;
  }
}

void
StoreMessage::ReadEndMarker()
{
  unsigned check = ReadNumber16();
  if(check != 0xFFFF)
  {
    throw StdException(ERROR_FT_ENDMARKER);
  }
}
