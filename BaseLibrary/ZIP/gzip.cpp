/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: gzip.cpp
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

#include "pch.h"
#include "zutil.h"
#include "inftrees.h"
#include "deflate.h"
#include "inflate.h"
#include "gzip.h"

// Compression example taken from
// http://stackoverflow.com/questions/4538586/how-to-compress-a-buffer-with-zlib
// But optimized for production situations, avoiding an extra buffer copy
// And changed to use GZIP encoding instead of ZLIB encoding
// Adapted for production like situations
//
bool gzip_compress_memory(void *in_data,size_t in_data_size,std::vector<uint8_t>& buffer)
{
  const size_t BUFSIZE = 128 * 1024;
  uint8_t* temp_buffer = new uint8_t[BUFSIZE];

  z_stream strm;
  memset(&strm,0,sizeof(z_stream));
  strm.next_in   = reinterpret_cast<uint8_t *>(in_data);
  strm.avail_in  = (uInt)in_data_size; // [EH]
  strm.next_out  = temp_buffer;
  strm.avail_out = BUFSIZE;

  int windowBits = 15;
  int GZIP_ENCODING = 16;

  deflateInit2(&strm,Z_DEFAULT_COMPRESSION,Z_DEFLATED,
               windowBits | GZIP_ENCODING,8,
               Z_DEFAULT_STRATEGY);

  while(strm.avail_in != 0)
  {
    int res = deflate(&strm,Z_NO_FLUSH);
    if(res != Z_OK)
    {
      delete [] temp_buffer;
      return false;
    }
    if(strm.avail_out == 0)
    {
      buffer.insert(buffer.end(),temp_buffer,temp_buffer + BUFSIZE);
      strm.next_out = temp_buffer;
      strm.avail_out = BUFSIZE;
    }
  }

  int deflate_res = Z_OK;
  while(deflate_res == Z_OK)
  {
    if(strm.avail_out == 0)
    {
      buffer.insert(buffer.end(),temp_buffer,temp_buffer + BUFSIZE);
      strm.next_out = temp_buffer;
      strm.avail_out = BUFSIZE;
    }
    deflate_res = deflate(&strm,Z_FINISH);
  }

  if(deflate_res != Z_STREAM_END)
  {
    delete [] temp_buffer;
    return false;
  }
  buffer.insert(buffer.end(),temp_buffer,temp_buffer + BUFSIZE - strm.avail_out);
  deflateEnd(&strm);

  delete [] temp_buffer;
  return true;
}

bool gzip_decompress_memory(void *in_data,size_t in_data_size,std::vector<uint8_t>& buffer)
{
  const size_t BUFSIZE = 128 * 1024;
  uint8_t* temp_buffer = new uint8_t[BUFSIZE];

  z_stream strm;
  memset(&strm,0,sizeof(z_stream));
  strm.next_in   = reinterpret_cast<uint8_t *>(in_data);
  strm.avail_in  = (uInt)in_data_size;
  strm.next_out  = temp_buffer;
  strm.avail_out = BUFSIZE;

  int windowBits    = 15;
  int GZIP_ENCODING = 16;

  // inflateInit(&strm);
  inflateInit2(&strm,windowBits + GZIP_ENCODING);

  while(strm.avail_in != 0)
  {
    int res = inflate(&strm,Z_NO_FLUSH);
    if(res != Z_OK && res != Z_STREAM_END)
    {
      delete [] temp_buffer;
      return false;
    }
    if(strm.avail_out == 0)
    {
      buffer.insert(buffer.end(),temp_buffer,temp_buffer + BUFSIZE);
      strm.next_out  = temp_buffer;
      strm.avail_out = BUFSIZE;
    }
  }

  int inflate_res = Z_OK;
  while(inflate_res == Z_OK)
  {
    if(strm.avail_out == 0)
    {
      buffer.insert(buffer.end(),temp_buffer,temp_buffer + BUFSIZE);
      strm.next_out  = temp_buffer;
      strm.avail_out = BUFSIZE;
    }
    inflate_res = inflate(&strm,Z_FINISH);
  }

  if(inflate_res != Z_STREAM_END)
  {
    delete [] temp_buffer;
    return false;
  }
  buffer.insert(buffer.end(),temp_buffer,temp_buffer + BUFSIZE - strm.avail_out);
  inflateEnd(&strm);

  delete [] temp_buffer;
  return true;
}
