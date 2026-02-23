#include "pch.h"
#include "KernelObjects.h"
#include <StdException.h>

XSyncObject::XSyncObject(LPCTSTR pstrName)
{
  UNREFERENCED_PARAMETER(pstrName);   // unused in release builds

  m_hObject = NULL;
#ifdef _DEBUG
  m_strName = pstrName;
#endif
}

XSyncObject::~XSyncObject()
{
  if(m_hObject != NULL)
  {
    ::CloseHandle(m_hObject);
    m_hObject = NULL;
  }
}

BOOL 
XSyncObject::Lock(DWORD dwTimeout)
{
  DWORD dwRet = ::WaitForSingleObject(m_hObject,dwTimeout);
  if(dwRet == WAIT_OBJECT_0 || dwRet == WAIT_ABANDONED)
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

/////////////////////////////////////////////////////////////////////////////
// CEvent

XEvent::XEvent(BOOL bInitiallyOwn,BOOL bManualReset,LPCTSTR pstrName,
               LPSECURITY_ATTRIBUTES lpsaAttribute)
       :XSyncObject(pstrName)
{
  m_hObject = ::CreateEvent(lpsaAttribute,bManualReset,
                            bInitiallyOwn,pstrName);
  if(m_hObject == NULL)
  {
    throw StdException(_T("Cannot create an MS-Windows Event"));
  }
}

XEvent::~XEvent()
{
}

BOOL XEvent::Unlock()
{
  return TRUE;
}


//////////////////////////////////////////////////////////////////////////
// Critical Section

HRESULT XCriticalSection::Init()
{
  if(!InitializeCriticalSectionAndSpinCount(&m_sect,0))
  {
    return HRESULT_FROM_WIN32(GetLastError());
  }
  return S_OK;
}

XCriticalSection::XCriticalSection()
                 :XSyncObject(NULL)
{
  HRESULT hr = Init();

  if(FAILED(hr))
  {
    throw StdException(hr,_T("Init XCriticalSection"));
  }
}

XCriticalSection::operator CRITICAL_SECTION* ()
{
  return (CRITICAL_SECTION*)&m_sect;
}

XCriticalSection::~XCriticalSection()
{
  ::DeleteCriticalSection(&m_sect);
}

BOOL 
XCriticalSection::Lock()
{
  ::EnterCriticalSection(&m_sect);
  return TRUE;
}

BOOL 
XCriticalSection::Lock(DWORD dwTimeout)
{
  assert(dwTimeout == INFINITE);
  (void)dwTimeout;
  return Lock();
}

BOOL
XCriticalSection::Unlock()
{
  ::LeaveCriticalSection(&m_sect);
  return TRUE;
}
