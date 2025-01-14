/////////////////////////////////////////////////////////////////////////////
// 
// SocketStream
//
//
#include "stdafx.h"
#include "SocketStream.h"

// PlainSocket or SecureSocket is running in SSL/TLS mode or not

bool
SocketStream::InSecureMode()
{
  return m_secureMode;
};

// Plain/Secure sockets can be kept alive longer for callbacks from the threadpool
// With the drop of the last reference, the object WILL destroy itself

void
SocketStream::AddReference()
{
  InterlockedIncrement(&m_references);
}

void
SocketStream::DropReference()
{
  if(InterlockedDecrement(&m_references) <= 0)
  {
    delete this;
  }
}
