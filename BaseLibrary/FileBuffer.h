/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: FileBuffer.h
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
#pragma once
#include <vector>

// Max streaming serialize/deserialize limit
// Files bigger than this can only be putted/gotten by indirect file references
// This is also the streaming limit for OData / REST interfaces
#define STREAMING_LIMIT    (100*1024*1024)  // 100 Megabyte
// Minimum size required for zipping the contents of the buffer (4 * 4K buffer)
// Under this (arbitrary limit) it makes no sense to zip the buffer
#define COMPRESS_LIMIT     (16*1024)

// Static globals for the server as a whole
// Can be set through the Marlin.config reading of the HTTPServer
extern unsigned long g_streaming_limit; // = STREAMING_LIMIT;
extern unsigned long g_compress_limit;  // = COMPRESS_LIMIT;

typedef struct _bufferPart
{
  size_t m_length;
  uchar* m_buffer;
}
BufPart;

using Parts = std::vector<BufPart>;

class FileBuffer
{
public:
  FileBuffer();
  explicit FileBuffer(XString p_fileName);
  explicit FileBuffer(uchar* p_buffer,size_t p_length);
  explicit FileBuffer(FileBuffer& p_buffer);
 ~FileBuffer();

  // Clean the FileBuffer
  void    Reset();
  void    ResetFilename();

  // Read in from filename, fill buffer
  bool    ReadFile();
  // Write buffer out to filename
  bool    WriteFile();
  // Open file for reading or writing
  bool    OpenFile(bool p_reading = true);
  // Close file again
  bool    CloseFile();

  // SETTERS

  // Set the filename
  void    SetFileName(const XString& p_fileName);
  // Set the buffer in one go
  void    SetBuffer(uchar* p_buffer,size_t p_length);
  // Add buffer part
  void    AddBuffer(uchar* p_buffer,size_t p_length);
  void    AddBufferCRLF(uchar* p_buffer,size_t p_length);
  void    AddStringToBuffer(XString p_string,XString p_charset,bool p_crlf = true);
  // Allocate a one-buffer block
  bool    AllocateBuffer(size_t p_length);

  // GETTERS

  // Has buffer parts
  bool    GetHasBufferParts();
  // Number of buffer parts
  int     GetNumberOfParts();
  // Get the filename
  XString GetFileName();
  // Get the buffer in one go
  void    GetBuffer(uchar*& p_buffer,size_t& p_length);
  // Get a buffer part
  bool    GetBufferPart(unsigned p_index,uchar*& p_buffer,size_t& p_length);
  // Get a copy of the total buffer, call 'free' yourselves!
  bool    GetBufferCopy(uchar*& p_buffer,size_t& p_length);
  // Get the length of the buffer
  size_t  GetLength();
  // Get resulting file handle
  HANDLE  GetFileHandle();

  // OPERATORS & OPERATIONS

  // Assignment operator for a file buffer
  FileBuffer& operator=(FileBuffer& p_orig);
  // GZIP the contents of the buffer
  bool    ZipBuffer();
  // GunZIP the contents of the buffer
  bool    UnZipBuffer();
  // Chunked encoding of the primary memory buffer
  bool    ChunkedEncoding(bool p_final);

private:
  // Defragment the buffer
  bool    Defragment();

  // Data contents of the HTTP buffer
  XString  m_fileName;     // File to receive/send
  HANDLE   m_file          { NULL };
  size_t   m_binaryLength  { NULL };
  uchar*   m_buffer        { nullptr };
  Parts    m_parts;
};

inline bool
FileBuffer::GetHasBufferParts()
{
  return m_parts.size() > 0;
}

inline void    
FileBuffer::SetFileName(const XString& p_fileName)
{
  m_fileName = p_fileName;
}

inline XString
FileBuffer::GetFileName()
{
  return m_fileName;
}

inline HANDLE
FileBuffer::GetFileHandle()
{
  return m_file;
}

inline void
FileBuffer::ResetFilename()
{
  m_fileName.Empty();
}

inline int
FileBuffer::GetNumberOfParts()
{
  return (int)m_parts.size();
}
