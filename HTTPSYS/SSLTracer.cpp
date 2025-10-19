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

// General purpose helper class for SSL, decodes buffers for diagnostics, handles SNI

SSLTracer::SSLTracer(const byte* BufPtr,const int BufBytes)
          :m_contentType(0),
           m_major(0),
           m_minor(0),
           m_length(0),
           m_handshakeType(0),
           m_handshakeLength(0),
           m_originalBufPtr(BufPtr),
           m_dataPtr(BufPtr),
           m_maxBufBytes(BufBytes)
{
  m_decoded = (BufPtr != nullptr) && CanDecode();
}

SSLTracer::~SSLTracer() = default;

// Decode a buffer
bool SSLTracer::CanDecode()
{
  if(m_maxBufBytes < 5)
  {
    return false;
  }
  else
  {
    m_contentType = *(m_dataPtr++);
    m_major       = *(m_dataPtr++);
    m_minor       = *(m_dataPtr++);
    m_length      = (*(m_dataPtr) << 8) + *(m_dataPtr + 1);
    m_dataPtr    += 2;

    if(m_length + 5 > m_maxBufBytes)
    {
      return false;
    }
    // This is a version we recognize
    if(m_contentType != 22)
    {
      return false;
    }
    // This is a handshake message (content type 22)
    m_handshakeType   = *(m_dataPtr++);
    m_handshakeLength = (*m_dataPtr << 16) + (*(m_dataPtr + 1) << 8) + *(m_dataPtr + 2);
    m_dataPtr += 3;
    if(m_handshakeType != 1)
    {
      return false;
    }
    m_bufEnd = m_originalBufPtr + 5 + 4 + m_handshakeLength;
    return true;
  }
}

// Trace handshake buffer
void SSLTracer::TraceHandshake()
{
  if(m_maxBufBytes < 5)
  {
    LogError(_T("Buffer space too small"));
  }
  else
  {
    const byte * BufPtr = m_dataPtr;
    if(m_length + 5 == m_maxBufBytes)
    {
      DebugMsg(_T("Exactly one buffer is present"));
    }
    else if(m_length + 5 <= m_maxBufBytes)
    {
      DebugMsg(_T("Whole buffer is present"));
    }
    else
    {
      DebugMsg(_T("Only part of the buffer is present"));
    }
    if (m_major == 3)
    {
           if (m_minor == 0)  DebugMsg(_T("SSL version 3.0"));
      else if (m_minor == 1)  DebugMsg(_T("TLS version 1.0"));
      else if (m_minor == 2)  DebugMsg(_T("TLS version 1.1"));
      else if (m_minor == 3)  DebugMsg(_T("TLS version 1.2"));
      else                    DebugMsg(_T("TLS version after 1.2"));
    }
    else
    {
        DebugMsg(_T("Content Type = %d, Major.Minor Version = %d.%d, length %d (0x%04X)"), m_contentType, m_major, m_minor, m_length, m_length);
        DebugMsg(_T("This version is not recognized so no more information is available"));
        PrintHexDump(m_maxBufBytes, m_originalBufPtr);
        return;
    }
    // This is a version we recognize
    if (m_contentType != 22)
    {
        DebugMsg(_T("This content type (%d) is not recognized"), m_contentType);
        PrintHexDump(m_maxBufBytes, m_originalBufPtr);
        return;
    }
    // This is a handshake message (content type 22)
    if (m_handshakeType != 1)
    {
        DebugMsg(_T("This handshake type (%d) is not recognized"), m_handshakeType);
        PrintHexDump(m_maxBufBytes, m_originalBufPtr);
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
    bool extensionsPresent = BufPtr < m_bufEnd;
    UINT16 extensionsLength = (*(BufPtr) << 8) + *(BufPtr + 1);
    BufPtr += 2;
    if(extensionsLength == m_bufEnd - BufPtr)
    {
      DebugMsg(_T("There are %d bytes of extension data"),extensionsLength);
    }
    while (BufPtr < m_bufEnd)
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
    if(BufPtr == m_bufEnd)
    {
      DebugMsg(_T("Extensions exactly filled the header, as expected"));
    }
    else
    {
      LogError(_T("** Error ** Extensions did not fill the header"));
    }
  }
  PrintHexDump(m_maxBufBytes, m_originalBufPtr);
  return;
}

// Is this packet a complete client initialize packet
bool SSLTracer::IsClientInitialize()
{
   return m_decoded;
}

// Get SNI provided hostname
XString SSLTracer::GetSNIHostname()
{
   const byte * BufPtr = m_dataPtr;
   if (m_decoded)
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
      bool extensionsPresent = BufPtr < m_bufEnd;
      UINT16 extensionsLength = (*(BufPtr) << 8) + *(BufPtr + 1);
      BufPtr += 2;
      while (BufPtr < m_bufEnd)
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
                 XString answer;
                 answer.append((size_t)serverNameLength,*(TCHAR*)BufPtr);
                 return answer;
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
   return _T("");
}
