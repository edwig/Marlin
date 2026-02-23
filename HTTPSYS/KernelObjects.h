#pragma once
#include <assert.h>

//////////////////////////////////////////////////////////////////////////
//

class XSyncObject
{
  // Constructor
public:
  explicit XSyncObject(LPCTSTR pstrName);
  virtual ~XSyncObject();

  // Attributes
  operator HANDLE() const;
  HANDLE  m_hObject;

  // Operations
  virtual BOOL Lock(DWORD dwTimeout = INFINITE);
  virtual BOOL Unlock() = 0;
  virtual BOOL Unlock(LONG /* lCount */,LPLONG /* lpPrevCount=NULL */)
  {
    return TRUE;
  }

  // Implementation
public:
#ifdef _DEBUG
  CString m_strName;
#endif
  friend class CSingleLock;
  friend class CMultiLock;
};

inline 
XSyncObject::operator HANDLE() const
{
  return m_hObject;
}



//////////////////////////////////////////////////////////////////////////
// Event

class XEvent: public XSyncObject
{
  // Constructor
public:
  explicit XEvent(BOOL bInitiallyOwn = FALSE,BOOL bManualReset = FALSE,
                  LPCTSTR lpszNAme = NULL,LPSECURITY_ATTRIBUTES lpsaAttribute = NULL);
  virtual ~XEvent();

  // Operations
  BOOL SetEvent();
  BOOL PulseEvent();
  BOOL ResetEvent();
  BOOL Unlock();

  // Implementation
public:

private:
  using XSyncObject::Unlock;
};

inline BOOL XEvent::SetEvent()
{
  assert(m_hObject != NULL);

  return ::SetEvent(m_hObject);
}

inline BOOL XEvent::PulseEvent()
{
  assert(m_hObject != NULL);

  return ::PulseEvent(m_hObject);
}

inline BOOL XEvent::ResetEvent()
{
  assert(m_hObject != NULL);

  return ::ResetEvent(m_hObject);
}

//////////////////////////////////////////////////////////////////////////
// Critical section

class XCriticalSection : public XSyncObject
{
private:
  using XSyncObject::Unlock;

  // Constructor
public:
  XCriticalSection();
  virtual ~XCriticalSection();

  // Attributes
  operator CRITICAL_SECTION* ();
  CRITICAL_SECTION m_sect;

  // Operations
  BOOL Unlock();
  BOOL Lock();
  BOOL Lock(DWORD dwTimeout);

private:
  HRESULT Init();
};

