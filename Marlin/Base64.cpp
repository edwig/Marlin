/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Base64.cpp
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
#include "stdafx.h"
#include "Base64.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#endif
const unsigned char B64_offset[256] =
{
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
  64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
  64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

const char base64_map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned char*
Base64::Encrypt(const unsigned char* srcp,int len,unsigned char* dstp)
{
  int i = 0;
  unsigned char *dst = dstp;

  for (i = 0; i < len - 2; i += 3)
  {
    *dstp++ = *(base64_map + ((*(srcp+i)   >> 2) & 0x3f));
    *dstp++ = *(base64_map + ((*(srcp+i)   << 4) & 0x30 | (*(srcp+i+1) >> 4) & 0x0f));
    *dstp++ = *(base64_map + ((*(srcp+i+1) << 2) & 0x3C | (*(srcp+i+2) >> 6) & 0x03));
    *dstp++ = *(base64_map + (*(srcp+i+2) & 0x3f));
  }
  srcp += i;
  len -= i;

  if(len & 0x02 ) /* (i==2) 2 bytes left,pad one byte of '=' */
  {      
    *dstp++ = *(base64_map + ((*srcp >> 2)   & 0x3f));
    *dstp++ = *(base64_map + ((*srcp << 4)   & 0x30 | (*(srcp+1)>>4) & 0x0f ));
    *dstp++ = *(base64_map + ((*(srcp+1)<<2) & 0x3C) );
    *dstp++ = '=';
  }
  else if(len & 0x01 )  /* (i==1) 1 byte left,pad two bytes of '='  */
  { 
    *dstp++ = *(base64_map + ((*srcp>>2) & 0x3f));
    *dstp++ = *(base64_map + ((*srcp<<4) & 0x30));
    *dstp++ = '=';
    *dstp++ = '=';
  }
  *dstp = '\0';
  return dst;
}

void*
Base64::Decrypt(const unsigned char* srcp,int len,unsigned char* dstp)
{
  int i = 0;
  void *dst = dstp;

  while(i < len)
  {
    *dstp++ = (B64_offset[*(srcp+i)]   << 2 | B64_offset[*(srcp+i+1)] >> 4);
    *dstp++ = (B64_offset[*(srcp+i+1)] << 4 | B64_offset[*(srcp+i+2)] >> 2);
    *dstp++ = (B64_offset[*(srcp+i+2)] << 6 | B64_offset[*(srcp+i+3)]     );
    i += 4;
  }
  srcp += i;

  if(*(srcp-2) == '=')  /* remove 2 bytes of '='  padded while encoding */
  {	 
    *(dstp--) = '\0';
    *(dstp--) = '\0';
  }
  else if(*(srcp-1) == '=') /* remove 1 byte of '='  padded while encoding */
  {
    *(dstp--) = '\0';
  }
  *dstp = '\0';

  return dst;
};

size_t 
Base64::B64_length(size_t len)
{
  size_t  npad = len%3;
  size_t  size = (npad > 0)? (len +3-npad ) : len; // padded for multiple of 3 bytes
  return  (size*8)/6;
}

size_t 
Base64::Ascii_length(size_t len)
{
  return  (len*6)/8;
}

//////////////////////////////////////////////////////////////////////////
//
// CRC4
//
//////////////////////////////////////////////////////////////////////////

char*
CRC4::Encrypt(char *pszText,const char *pszKey) 
{
  unsigned char sbox[256];      /* Encryption array             */
  unsigned char key [256];      /* Numeric key values           */

  memset(sbox,0,256);
  memset(key, 0,256);

  int i = 0;
  int j = 0;
  int n = 0;
  int m = 0;
  int ilen = (int)strlen(pszKey);

  for (m = 0;  m < 256; m++)  /* Initialize the key sequence */
  {
    *(key + m)= *(pszKey + (m % ilen));
    *(sbox + m) = (unsigned char)m;
  }

  for (m=0; m < 256; m++)
  {
    n = (n + *(sbox+m) + *(key + m)) &0xff;
    SWAP(*(sbox + m),*(sbox + n));
  }

  ilen = (int)strlen(pszText);
  for (m = 0; m < ilen; m++)
  {
    i = (i + 1) &0xff;
    j = (j + *(sbox + i)) &0xff;
    SWAP(*(sbox+i),*(sbox + j));  /* randomly Initialize the key sequence */
    unsigned char k = *(sbox + ((*(sbox + i) + *(sbox + j)) &0xff ));
    if(k == *(pszText + m))       /* avoid '\0' beween the decoded text; */
    {
      k = 0;
    }
    *(pszText + m) ^=  k;
  }
  // remove Key traces from memory
  memset(sbox,0,256);  
  memset(key, 0,256);   

  return pszText;
}

char*
CRC4::Decrypt(char *pszText,const char *pszKey)
{
  return Encrypt(pszText,pszKey) ;  /* using the same function as encoding */
}
