//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#pragma once

class CodeBase64
{
public:
  unsigned char*  Encrypt(const unsigned char* srcp, int len, unsigned char * dstp);
  void*           Decrypt(const unsigned char* srcp, int len, unsigned char * dstp);
  size_t          B64_length(size_t len);
  size_t          Ascii_length(size_t len);
};

