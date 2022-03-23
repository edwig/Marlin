/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: FileBuffer.cpp
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
#include "FileBuffer.h"
#include "gzip.h"

unsigned long g_streaming_limit = STREAMING_LIMIT;
unsigned long g_compress_limit  = COMPRESS_LIMIT;

//////////////////////////////////////////////////////////////////////////
//
// The FileBuffer class
//
//////////////////////////////////////////////////////////////////////////

FileBuffer::FileBuffer()
{
}

FileBuffer::FileBuffer(XString p_fileName)
           :m_fileName(p_fileName)
{
}

FileBuffer::FileBuffer(uchar* p_buffer,size_t p_length)
{
  SetBuffer(p_buffer,p_length);
}

FileBuffer::FileBuffer(FileBuffer& p_orig)
{
  // Copy the members
  m_fileName     = p_orig.m_fileName;
  m_binaryLength = p_orig.m_binaryLength;

  // If it had a handle, duplicate it
  if(p_orig.m_file)
  {
    DuplicateHandle(GetCurrentProcess()
                    ,p_orig.m_file
                    ,GetCurrentProcess()
                    ,&m_file
                    ,0
                    ,FALSE
                    ,DUPLICATE_SAME_ACCESS);
  }

  // If it had a buffer, duplicate it
  if(m_binaryLength)
  {
    m_buffer = new uchar[m_binaryLength + 1];
    memcpy(m_buffer,p_orig.m_buffer,m_binaryLength);
    m_buffer[m_binaryLength] = 0;
  }

  // If it had buffer parts, duplicate them
  for(auto& part : p_orig.m_parts)
  {
    BufPart dupli;
    dupli.m_length = part.m_length;
    dupli.m_buffer = new uchar[dupli.m_length];
    memcpy(dupli.m_buffer,part.m_buffer,dupli.m_length);
    dupli.m_buffer[part.m_length] = 0;
    m_parts.push_back(dupli);
  }
}

FileBuffer::~FileBuffer()
{
  Reset();
}

// Clean the buffer
void    
FileBuffer::Reset()
{
  // Free one buffer
  if(m_buffer)
  {
    delete [] m_buffer;
    m_buffer = NULL;
  }
  // Free buffer parts
  Parts::iterator it;
  for(it = m_parts.begin();it != m_parts.end(); ++it)
  {
    delete [] it->m_buffer;
  }
  m_parts.clear();
  // Close file handle
  if(m_file && (m_file != INVALID_HANDLE_VALUE))
  {
    CloseHandle(m_file);
    m_file = NULL;
  }
  // Reset length
  m_binaryLength = 0;

  // Reset the filename
  // NO: Don't do this,  so it can be reopened!
  // m_fileName.Empty();
}

size_t
FileBuffer::GetLength()
{
  if(!m_fileName.IsEmpty())
  {
    m_binaryLength = 0;
    if(OpenFile(true))
    {
      DWORD high = 0;
      m_binaryLength = GetFileSize(m_file,&high);
#ifdef _M_X64
      if(high)
      {
        m_binaryLength |= (size_t)high << 32;
      }
#endif
      CloseFile();
    }
    return m_binaryLength;
  }

  // See if we do it in parts
  if(m_parts.size())
  {
    size_t length = 0;
    Parts::iterator it;
    for(it = m_parts.begin();it != m_parts.end(); ++it)
    {
      length += it->m_length;
    }
    return length;
  }
  // Return in one go
  return m_binaryLength;
}

void    
FileBuffer::SetBuffer(uchar* p_buffer,size_t p_length)
{
  Reset();
  if(p_buffer)
  {
    m_binaryLength = p_length;
    m_buffer = new uchar[p_length + 1];
    memcpy(m_buffer,p_buffer,p_length);
    m_buffer[p_length] = 0;
  }
}

// Add buffer part
// Handy for the HTTP protocol
void    
FileBuffer::AddBuffer(uchar* p_buffer,size_t p_length)
{
  BufPart part;
  part.m_buffer = new uchar[p_length + 1];
  part.m_length = p_length;

  memcpy(part.m_buffer,p_buffer,p_length);
  part.m_buffer[p_length] = 0;
  // Keep the buffer part
  m_parts.push_back(part);
}

// Add buffer part + CRLF
// Handy for the FormData HTTP protocol
void
FileBuffer::AddBufferCRLF(uchar* p_buffer,size_t p_length)
{
  BufPart part;
  part.m_buffer = new uchar[p_length + 3];
  part.m_length = p_length + 2;

  if(part.m_buffer)
  {
    memcpy(part.m_buffer,p_buffer,p_length);
    *((char*)part.m_buffer + p_length + 0) = '\r';
    *((char*)part.m_buffer + p_length + 1) = '\n';
    *((char*)part.m_buffer + p_length + 2) = 0;
  }
  // Keep the buffer part
  m_parts.push_back(part);
}

// Allocate a one-buffer block
bool    
FileBuffer::AllocateBuffer(size_t p_length)
{
  if(m_parts.empty())
  {
    if(m_binaryLength == 0)
    {
      m_buffer = new uchar[p_length + 1];
      m_binaryLength = p_length;
      m_buffer[0] = 0;
      return true;
    }
  }
  return false;
}

void
FileBuffer::GetBuffer(uchar*& p_buffer,size_t& p_length)
{
  if(m_parts.size())
  {
    // CANNOT GET IT. Indicate the length
    p_buffer = NULL;
    p_length = GetLength();
    return;
  }
  // Get the buffer in one go
  p_buffer = m_buffer;
  p_length = m_binaryLength;
}

// Get a buffer part
bool    
FileBuffer::GetBufferPart(unsigned p_index,uchar*& p_buffer,size_t& p_length)
{
  // Read past end?
  if(p_index >= m_parts.size())
  {
    return false;
  }
  // Get the part
  BufPart& part = m_parts[p_index];

  p_buffer = part.m_buffer;
  p_length = part.m_length;

  return true;
}

// Getting a copy of the total buffer
// ready for conversion to a XString / std::string
bool
FileBuffer::GetBufferCopy(uchar*& p_buffer,size_t& p_length)
{
  // Calculate length and getting a buffer
  p_length = GetLength();
  p_buffer = new uchar[p_length + 1];

  // Optimize in one go
  if(m_parts.empty())
  {
    if(m_buffer == nullptr) 
    {
      delete [] p_buffer;
      p_buffer = nullptr;
      return false;
    }
    memcpy(p_buffer,m_buffer,p_length + 1);
  }
  else 
  {
    DWORD_PTR dest = (DWORD_PTR) p_buffer;
    for(unsigned ind = 0;ind < m_parts.size(); ++ind)
    {
      BufPart& part = m_parts[ind];
      memcpy((void*)dest,part.m_buffer,part.m_length);
      dest += part.m_length;
    }
  }
  // Zero delimiter of the buffer
  p_buffer[p_length] = 0;

  return true;
}

// Open file for reading or writing
bool
FileBuffer::OpenFile(bool p_reading)
{
  // Make sure previous handle is closed
  if(m_file)
  {
    CloseHandle(m_file);
    m_file = NULL;
  }

  // Get our file modes
  DWORD readWrite   = p_reading ? GENERIC_READ    : GENERIC_WRITE;
  DWORD sharing     =             FILE_SHARE_READ | FILE_SHARE_WRITE;
  DWORD disposition = p_reading ? OPEN_EXISTING   : CREATE_ALWAYS;

  // Create or open the file
  m_file = CreateFile(m_fileName            // Filename
                     ,readWrite             // Access mode
                     ,sharing               // Share mode
                     ,NULL                  // Security
                     ,disposition           // Create or open
                     ,FILE_ATTRIBUTE_NORMAL // Attributes
                     ,NULL);                // Template file

  if(m_file == INVALID_HANDLE_VALUE)
  {
    m_file = NULL;
    return false;
  }
  return true;
}

// Close file again
bool
FileBuffer::CloseFile()
{
  bool result = false;
  if(m_file)
  {
    result = (CloseHandle(m_file) == TRUE);
    m_file = NULL;
  }
  else
  {
    // Already closed!
    result = true;
  }
  return result;
}

// Read in from filename, fill buffer
bool    
FileBuffer::ReadFile()
{
  bool result = false;
  if(OpenFile() == false)
  {
    return false;
  }
  // Get the size of the file so we can create an internal buffer
  DWORD sizeHigh  = NULL;
  DWORD sizeLow   = GetFileSize(m_file,&sizeHigh);
  DWORD readBytes = 0;

  if(sizeLow == INVALID_FILE_SIZE || sizeHigh)
  {
    // Cannot get the file size, or file bigger than 4GB
    goto cleanup;
  }
  // get size
  m_binaryLength = sizeLow;

  // File too big
  if(m_binaryLength > g_streaming_limit || sizeHigh > 0)
  {
    goto cleanup;
  }
  // Delete old buffer and create a new one
  if(m_buffer)
  {
    delete [] m_buffer;
  }
  m_buffer = new uchar[m_binaryLength + 1];

  // Read file from disk in one (1) go
  if(::ReadFile(m_file,m_buffer,(DWORD)m_binaryLength,&readBytes,NULL))
  {
    if(readBytes >= (DWORD)m_binaryLength)
    {
      result = true;
    }
    // End buffer to be sure
    m_buffer[m_binaryLength] = 0;
    if(readBytes < (DWORD)m_binaryLength)
    {
      m_buffer[readBytes] = 0;
    }
  }
cleanup:
  // Close the file handle
  if(!CloseHandle(m_file))
  {
    result = false;
  }
  // Always reset to NULL, so this cannot happen again.
  // Did crash a few functions in the server!!
  m_file = NULL;

  // In case of error: free again
  if(!result && m_buffer)
  {
    delete [] m_buffer;
    m_buffer = NULL;
  }
  return result;
}

// Write buffer out to filename
// Writing goes in special FILE_FLAG_WRITE_THROUGH mode
// so clients of the server can see the files right away!
bool    
FileBuffer::WriteFile()
{
  // Create file to write to
  bool result = false;

  if(OpenFile(false) == false)
  {
    return false;
  }

  DWORD written = 0;

  if(m_parts.size())
  {
    // Write the parts
    DWORD part = 0;
    Parts::iterator it;
    for(it = m_parts.begin();it != m_parts.end(); ++it)
    {
      if(::WriteFile(m_file,it->m_buffer,(DWORD)it->m_length,&part,NULL))
      {
        written += part;
      }
    }
    // Check if everything is written
    if(written >= (DWORD)GetLength())
    {
      result = true;
    }
  }
  else
  {
    // Write the file to disk in one (1) go
    if(::WriteFile(m_file,m_buffer,(DWORD)m_binaryLength,&written,NULL))
    {
      if(written >= (DWORD)m_binaryLength)
      {
        result = true;
      }
    }
  }
  // close again
  return CloseFile() && result;
}

// Assignment operator for a file buffer
FileBuffer& 
FileBuffer::operator=(FileBuffer& p_orig)
{
  // Check against self
  if(this == &p_orig)
  {
    return *this;
  }
  Reset();

  // Copy the members
  m_fileName     = p_orig.m_fileName;
  m_binaryLength = p_orig.m_binaryLength;

  // If it had a file handle, duplicate it
  if(p_orig.m_file)
  {
    DuplicateHandle(GetCurrentProcess()
                   ,p_orig.m_file
                   ,GetCurrentProcess()
                   ,&m_file
                   ,0
                   ,FALSE
                   ,DUPLICATE_SAME_ACCESS);
  }
  
  // If it had a buffer, duplicate it
  if(m_binaryLength)
  {
    m_buffer = new uchar[m_binaryLength + 1];
    memcpy(m_buffer,p_orig.m_buffer,m_binaryLength);
    m_buffer[m_binaryLength] = 0;
  }
  // If it had buffer parts, duplicate them
  Parts::iterator it;
  for(it = p_orig.m_parts.begin();it != p_orig.m_parts.end(); ++it)
  {
    BufPart& part = *it;
    BufPart  dupli;
    dupli.m_length = part.m_length;
    dupli.m_buffer = new uchar[dupli.m_length];
    memcpy(dupli.m_buffer,part.m_buffer,dupli.m_length);
    dupli.m_buffer[part.m_length] = 0;
    m_parts.push_back(dupli);
  }

  // Return ourselves
  return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// ZIPPING and UNZIPPING the buffer
//
//////////////////////////////////////////////////////////////////////////

// GZIP the contents of the buffer
bool
FileBuffer::ZipBuffer()
{
  unsigned size = (unsigned) GetLength();

  // Do not ZIP the buffer under the compression limit
  // Under 4 buffer messages, it makes no sense to waste time on compression/decompression
  if(size < g_compress_limit)
  {
    return false;
  }

  // First see if we must defragment the buffer
  if(Defragment() == false)
  {
    return false;
  }

  // Be sure we have the file in the buffer
  if(!m_fileName.IsEmpty())
  {
    if(ReadFile())
    {
      m_fileName.Empty();
    }
    else
    {
      return false;
    }
  }

  // Compress in-memory with ZLib to a 'gzip' buffer for HTTP
  std::vector<uint8_t> out_data;
  if(gzip_compress_memory(m_buffer,m_binaryLength,out_data))
  {
    m_binaryLength = out_data.size();
  
    delete [] m_buffer;
    m_buffer = new uchar[m_binaryLength + 1];
    for(size_t ind = 0;ind < m_binaryLength; ++ind)
    {
      ((uint8_t *)m_buffer)[ind] = out_data[ind];
    }
    m_buffer[m_binaryLength] = 0;
    return true;
  }
  return false;
}

// GunZIP the contents of the buffer
// Works unconditionally. No way to detect a 'real' zipped content buffer
bool
FileBuffer::UnZipBuffer()
{
  // First see if we must defragment the file buffer
  if(GetHasBufferParts())
  {
    uchar* buffer = nullptr;
    size_t size = 0L;
    if(GetBufferCopy(buffer,size))
    {
      Reset();
      m_buffer = buffer;
      m_binaryLength = size;
    }
    else
    {
      return false;
    }
  }
  // DECompress in-memory with ZLib to a 'gzip' buffer for HTTP
  std::vector<uint8_t> out_data;
  if(gzip_decompress_memory(m_buffer,m_binaryLength,out_data))
  {
    m_binaryLength = out_data.size();

    delete [] m_buffer;
    m_buffer = new uchar[m_binaryLength + 1];
    for(size_t ind = 0; ind < m_binaryLength; ++ind)
    {
      ((uint8_t *)m_buffer)[ind] = out_data[ind];
    }
    m_buffer[m_binaryLength] = 0;
    return true;
  }
  return false;
}

// Make sure the filebuffer is defragemented
bool
FileBuffer::Defragment()
{
  // First see if we must defragment the file buffer
  if(GetHasBufferParts())
  {
    uchar* buffer = nullptr;
    size_t size   = 0L;
    if(GetBufferCopy(buffer,size))
    {
      Reset();
      m_buffer = buffer;
      m_binaryLength = size;
    }
    else
    {
      return false;
    }
  }
  return true;
}

#define CHUNKED_OVERHEAD 20

// Chunked encoding of the primary memory buffer
bool
FileBuffer::ChunkedEncoding(bool p_final)
{
  if(!Defragment())
  {
    return false;
  }
  // Encode the buffer
  uchar* buffer = new uchar[m_binaryLength + CHUNKED_OVERHEAD];
  int pos = sprintf_s((char*)buffer,CHUNKED_OVERHEAD,"%X\r\n",(int)m_binaryLength);
  memcpy_s(buffer+pos,m_binaryLength,m_buffer,m_binaryLength);
  memcpy_s(buffer+pos+m_binaryLength,3,"\r\n\0",3);
  size_t newLength = m_binaryLength + pos + 2;
  if(p_final)
  {
    memcpy_s(buffer + pos + m_binaryLength + 2,6,"0\r\n\r\n\0",6);
    newLength += 5;
  }
  // Retain the result;
  Reset();
  m_buffer = buffer;
  m_binaryLength = newLength;

  return true;
}
