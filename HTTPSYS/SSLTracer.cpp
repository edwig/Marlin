/////////////////////////////////////////////////////////////////////////////
// 
// SSL Tracer functions
//
// Original idea:
// David Maw: https://www.codeproject.com/Articles/1000189/A-Working-TCP-Client-and-Server-With-SSL
// License:   https://www.codeproject.com/info/cpol10.aspx
//
#include "stdafx.h"
#include "SSLTracer.h"
#include "SecureServerSocket.h"
#include "Logging.h"
#include <LogAnalysis.h>
#include "SSLUtilities.h"
#include <memory>
#include <algorithm>

#pragma comment(lib, "dnsapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// General purpose helper class for SSL, decodes buffers for diagnostics, handles SNI

SSLTracer::SSLTracer(const byte* BufPtr,const int BufBytes)
          :contentType(0),
           major(0),
           minor(0),
           length(0),
           handshakeType(0),
           handshakeLength(0),
           OriginalBufPtr(BufPtr),
           DataPtr(BufPtr),
           MaxBufBytes(BufBytes)
{
  decoded = (BufPtr != nullptr) && CanDecode();
}

SSLTracer::~SSLTracer() = default;

// Decode a buffer
bool SSLTracer::CanDecode()
{
  if(MaxBufBytes < 5)
  {
    return false;
  }
  else
  {
    contentType = *(DataPtr++);
    major       = *(DataPtr++);
    minor       = *(DataPtr++);
    length      = (*(DataPtr) << 8) + *(DataPtr + 1);
    DataPtr    += 2;

    if(length + 5 > MaxBufBytes)
    {
      return false;
    }
    // This is a version we recognize
    if(contentType != 22)
    {
      return false;
    }
    // This is a handshake message (content type 22)
    handshakeType   = *(DataPtr++);
    handshakeLength = (*DataPtr << 16) + (*(DataPtr + 1) << 8) + *(DataPtr + 2);
    DataPtr += 3;
    if(handshakeType != 1)
    {
      return false;
    }
    BufEnd = OriginalBufPtr + 5 + 4 + handshakeLength;
    return true;
  }
}

// Trace handshake buffer
void SSLTracer::TraceHandshake()
{
  if(MaxBufBytes < 5)
  {
    LogError(_T("Buffer space too small"));
  }
  else
  {
    const byte * BufPtr = DataPtr;
    if(length + 5 == MaxBufBytes)
    {
      DebugMsg(_T("Exactly one buffer is present"));
    }
    else if(length + 5 <= MaxBufBytes)
    {
      DebugMsg(_T("Whole buffer is present"));
    }
    else
    {
      DebugMsg(_T("Only part of the buffer is present"));
    }
    if (major == 3)
    {
           if (minor == 0)  DebugMsg(_T("SSL version 3.0"));
      else if (minor == 1)  DebugMsg(_T("TLS version 1.0"));
      else if (minor == 2)  DebugMsg(_T("TLS version 1.1"));
      else if (minor == 3)  DebugMsg(_T("TLS version 1.2"));
      else                  DebugMsg(_T("TLS version after 1.2"));
    }
    else
    {
        DebugMsg(_T("Content Type = %d, Major.Minor Version = %d.%d, length %d (0x%04X)"), contentType, major, minor, length, length);
        DebugMsg(_T("This version is not recognized so no more information is available"));
        PrintHexDump(MaxBufBytes, OriginalBufPtr);
        return;
    }
    // This is a version we recognize
    if (contentType != 22)
    {
        DebugMsg(_T("This content type (%d) is not recognized"), contentType);
        PrintHexDump(MaxBufBytes, OriginalBufPtr);
        return;
    }
    // This is a handshake message (content type 22)
    if (handshakeType != 1)
    {
        DebugMsg(_T("This handshake type (%d) is not recognized"), handshakeType);
        PrintHexDump(MaxBufBytes, OriginalBufPtr);
        return;
    }
    // This is a client hello message (handshake type 1)
    DebugMsg(_T("client_hello"));
    BufPtr += 2; // Skip ClientVersion
    BufPtr += 32; // Skip Random
    UINT8 sessionidLength = *BufPtr;
    BufPtr += 1 + sessionidLength; // Skip SessionID
    UINT16 cipherSuitesLength = (*(BufPtr) << 8) + *(BufPtr + 1);
    BufPtr += 2 + cipherSuitesLength; // Skip CipherSuites
    UINT8 compressionMethodsLength = *BufPtr;
    BufPtr += 1 + compressionMethodsLength; // Skip Compression methods
    bool extensionsPresent = BufPtr < BufEnd;
    UINT16 extensionsLength = (*(BufPtr) << 8) + *(BufPtr + 1);
    BufPtr += 2;
    if(extensionsLength == BufEnd - BufPtr)
    {
      DebugMsg(_T("There are %d bytes of extension data"),extensionsLength);
    }
    while (BufPtr < BufEnd)
    {
      UINT16 extensionType = (*(BufPtr) << 8) + *(BufPtr + 1);
      BufPtr += 2;
      UINT16 extensionDataLength = (*(BufPtr) << 8) + *(BufPtr + 1);
      BufPtr += 2;
      if (extensionType == 0) // server name list, in practice there's only ever one name in it (see RFC 6066)
      {
        UINT16 serverNameListLength = (*(BufPtr) << 8) + *(BufPtr + 1);
        BufPtr += 2;
        DebugMsg(_T("Server name list extension, length %d"), serverNameListLength);
        const byte * serverNameListEnd = BufPtr + serverNameListLength;
        while (BufPtr < serverNameListEnd)
        {
          UINT8 serverNameType = *(BufPtr++);
          UINT16 serverNameLength = (*(BufPtr) << 8) + *(BufPtr + 1);
          BufPtr += 2;
          if(serverNameType == 0)
          {
            DebugMsg(_T("   Requested name \"%*s\""),serverNameLength,BufPtr);
          }
          else
          {
            DebugMsg(_T("   Server name Type %d, length %d, data \"%*s\""),serverNameType,serverNameLength,serverNameLength,BufPtr);
          }
          BufPtr += serverNameLength;
        }
      }
      else
      {
        DebugMsg(_T("Extension Type %d, length %d"), extensionType, extensionDataLength);
        BufPtr += extensionDataLength;
      }
    }
    if(BufPtr == BufEnd)
    {
      DebugMsg(_T("Extensions exactly filled the header, as expected"));
    }
    else
    {
      LogError(_T("** Error ** Extensions did not fill the header"));
    }
  }
  PrintHexDump(MaxBufBytes, OriginalBufPtr);
  return;
}

// Is this packet a complete client initialize packet
bool SSLTracer::IsClientInitialize()
{
   return decoded;
}

// Get SNI provided hostname
CString SSLTracer::GetSNIHostname()
{
   const byte * BufPtr = DataPtr;
   if (decoded)
   {
      // This is a client hello message (handshake type 1)
      BufPtr += 2; // Skip ClientVersion
      BufPtr += 32; // Skip Random
      UINT8 sessionidLength = *BufPtr;
      BufPtr += 1 + sessionidLength; // Skip SessionID
      UINT16 cipherSuitesLength = (*(BufPtr) << 8) + *(BufPtr + 1);
      BufPtr += 2 + cipherSuitesLength; // Skip CipherSuites
      UINT8 compressionMethodsLength = *BufPtr;
      BufPtr += 1 + compressionMethodsLength; // Skip Compression methods
      bool extensionsPresent = BufPtr < BufEnd;
      UINT16 extensionsLength = (*(BufPtr) << 8) + *(BufPtr + 1);
      BufPtr += 2;
      while (BufPtr < BufEnd)
      {
         UINT16 extensionType = (*(BufPtr) << 8) + *(BufPtr + 1);
         BufPtr += 2;
         UINT16 extensionDataLength = (*(BufPtr) << 8) + *(BufPtr + 1);
         BufPtr += 2;
         if (extensionType == 0) // server name list, in practice there's only ever one name in it (see RFC 6066)
         {
            UINT16 serverNameListLength = (*(BufPtr) << 8) + *(BufPtr + 1);
            BufPtr += 2;
            const byte * serverNameListEnd = BufPtr + serverNameListLength;
            while (BufPtr < serverNameListEnd)
            {
               UINT8 serverNameType = *(BufPtr++);
               UINT16 serverNameLength = (*(BufPtr) << 8) + *(BufPtr + 1);
               BufPtr += 2;
               if(serverNameType == 0)
               {
                 return CString((TCHAR*)BufPtr,serverNameLength);
               }
               BufPtr += serverNameLength;
            }
         }
         else
         {
            BufPtr += extensionDataLength;
         }
      }
   }
   return CString();
}
