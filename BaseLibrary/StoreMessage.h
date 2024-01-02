/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: StoreMessage.h
//
// BaseLibrary: Indispensable general objects and functions
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
#include "HTTPMessage.h"
#include "WinFile.h"

#define HTTP_FILE_VERSION 0x0101  // Effectively version 1.1

// Field Type in the binary file
enum class MSGFieldType
{
  FT_VERSION = 1      // Field is the binary file type version number (2 bytes)
 ,FT_RESPONSE_OFFSET  // Field is the response offset,     size 4 byte
 ,FT_HTTPCOMMAND      // Field is a HTTPCommand.           size 1 byte
 ,FT_URL              // Field is the designated URL,      string (2 bytes + x)
 ,FT_HTTPSTATUS       // Field is the HTTP status.         size 2 bytes
 ,FT_CONTENTTYPE      // Field is content-type header,     string (2 bytes + x)
 ,FT_CONTENTLENGTH    // Field is content-length,          size 8 bytes
 ,FT_ACCEPTENCODING   // Field is accept encoding header,  string (2 bytes + x)
 ,FT_VERBTUNNEL       // Field is verb-tunnel status,      size 1 byte
 ,FT_SENDBOM          // Field is send-BOM status,         size 1 byte
 ,FT_COOKIES          // Field is cookies map,             size 2 byte mapsize, 2 * (2 bytes + x)
 ,FT_REFERRER         // Field is the referrer header,     string (2 bytes + x)
 ,FT_DESKTOP          // Field is the desktop number,      size 2 bytes
 ,FT_HEADERS          // Field is the headers map,         size 2 byte mapsize, 2 * (2 bytes + x)
 ,FT_ROUTING          // Field is the routing map,         size 2 byte mapsize, 2 * (2 bytes + x)
 ,FT_ISMODIFIED       // Field is the if-modified-since,   size 1 byte
 ,FT_SYSTEMTIME       // Field is the modified header,     size 16 bytes
 ,FT_BODY             // Field is the incoming body ,      buffer (4 bytes + x)
 ,FT_ENDMARKER        // Field is the end-of-message       buffer 2 bytes (0xFFFF)
};

// Error numbers while reading a storage file
// GetError will return OS errors (>= 0) or our errors (< 0)
#define ERROR_FT_VERSION         -1    // Not a version number
#define ERROR_FT_WRONGVERSION    -2    // Wrong version storage file
#define ERROR_FT_RESPONSEOFFSET  -3    // Not a response offset field
#define ERROR_FT_UNKNOWNFIELD    -4    // Not a known field (higher version?)
#define ERROR_FT_BODY            -5    // Reading a body (wrong length?)
#define ERROR_FT_ENDMARKER       -6    // Not a end of message marker
#define ERROR_FT_RESPONSE        -7    // Cannot skip to response
#define ERROR_FT_NOFILE          -8    // Cannot skip to EOF
#define ERROR_FT_NORESPONSE      -9    // No response part in this file
#define ERROR_FT_LAST           -10

// The response offset is just after the version number (3 bytes long)
#define STORE_HTTP_RESPONSE_OFFSET 3

class StoreMessage
{
public:
  StoreMessage();
  explicit StoreMessage(XString p_filename);
  virtual ~StoreMessage();

  // OUR MAIN WORK
  virtual bool         StoreIncomingMessage(HTTPMessage* p_message);
  virtual bool         StoreResponseMessage(HTTPMessage* p_message);
  virtual HTTPMessage* ReadIncomingMessage();
  virtual HTTPMessage* ReadResponseMessage();

  // SETTERS
  void    SetFilename(XString p_filename);
  // GETTERS
  XString GetFilename();
  errno_t GetError();
  XString GetErrorMessage();

private:
  // File methods
  void    CloseFile();
  void    ReWriteOffset();
  void    SkipToResponse(bool p_checkPresence);
  void    StoreMessagePart(HTTPMessage* p_message);
  void     ReadMessagePart(HTTPMessage* p_message);

  // READING: Base type primitives
  MSGFieldType      ReadHeader();
  unsigned char     ReadNumber8();
  unsigned int      ReadNumber16();
  unsigned int      ReadNumber32();
  unsigned __int64  ReadNumber64();
  XString           ReadString();
  // READING: Logic fields
  int     ReadVersion();
  DWORD   ReadResponseOffset();
  void    ReadHTTPCommand   (HTTPMessage* p_msg);
  void    ReadURL           (HTTPMessage* p_msg);
  void    ReadStatus        (HTTPMessage* p_msg);
  void    ReadContentType   (HTTPMessage* p_msg);
  void    ReadContentLength (HTTPMessage* p_msg);
  void    ReadAcceptEncoding(HTTPMessage* p_msg);
  void    ReadVerbTunnel    (HTTPMessage* p_msg);
  void    ReadSendBOM       (HTTPMessage* p_msg);
  void    ReadCookies       (HTTPMessage* p_msg);
  void    ReadReferrer      (HTTPMessage* p_msg);
  void    ReadDesktop       (HTTPMessage* p_msg);
  void    ReadHeaders       (HTTPMessage* p_msg);
  void    ReadRouting       (HTTPMessage* p_msg);
  void    ReadIsModified    (HTTPMessage* p_msg);
  void    ReadSystemTime    (HTTPMessage* p_msg);
  void    ReadBody          (HTTPMessage* p_msg);
  void    ReadEndMarker();

  // WRITING: Base type primitives
  void    WriteHeader(MSGFieldType p_type);
  void    WriteNumber8(unsigned char p_number);
  void    WriteNumber16(unsigned int p_number);
  void    WriteNumber32(unsigned int p_number);
  void    WriteNumber64(unsigned __int64 p_number);
  void    WriteString(XString p_string);
  // WRITING: Logic fields
  void    WriteVersion();
  void    WriteResponseOffset(DWORD p_offset);
  void    WriteHTTPCommand(HTTPCommand p_command);
  void    WriteURL(XString p_url);
  void    WriteStatus(unsigned p_status);
  void    WriteContentType(XString p_type);
  void    WriteContentLength(size_t p_length);
  void    WriteAcceptEncoding(XString p_encoding);
  void    WriteVerbTunnel(bool p_verb);
  void    WriteSendBOM(bool p_bom);
  void    WriteCookies(Cookies& p_cookies);
  void    WriteReferrer(XString p_referrer);
  void    WriteDesktop(unsigned p_desktop);
  void    WriteHeaders(const HeaderMap* p_headers);
  void    WriteRouting(Routing& p_routing);
  void    WriteIsModified(bool p_isModified);
  void    WriteSystemTime(PSYSTEMTIME p_systemtime);
  void    WriteBody(FileBuffer* p_buffer);
  void    WriteEndMarker();

  XString m_filename;
  WinFile m_file;
  errno_t m_error { 0 };
};
