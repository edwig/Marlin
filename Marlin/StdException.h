//////////////////////////////////////////////////////////////////////////
//
// StdException. Catches Safe Exceptions and normal exceptions alike
//
// Code based on original idea of "Martin Ziacek" on www.codeproject.com
//
//////////////////////////////////////////////////////////////////////////

#ifndef __SEEXCEPTION_H__
#define __SEEXCEPTION_H__

#include <eh.h>

class StdException : public CException
{
	DECLARE_DYNAMIC(StdException)
public:
  // Application type constructors
  StdException(int p_errorCode);
  StdException(const char* p_fault);
  StdException(int p_errorCode,const char* p_fault);
  // Construct from a SafeExceptionHandler (SEH)
	StdException(UINT p_safe,_EXCEPTION_POINTERS* p_exceptionPointers);
	StdException(StdException& p_other);

  void                 Delete();
  UINT                 GetSafeExceptionCode();
	_EXCEPTION_POINTERS* GetExceptionPointers();
	PVOID                GetExceptionAddress();
  int                  GetApplicationCode();
  CString              GetApplicationFault();
  CString              GetErrorMessage();
  BOOL                 GetErrorMessage(LPTSTR p_error,UINT p_maxSize,PUINT p_helpContext = NULL);

private:
  UINT                 m_safeExceptionCode { 0 };
	_EXCEPTION_POINTERS* m_exceptionPointers { nullptr };
  UINT                 m_applicationCode   { 0 };
  CString              m_fault;
};

// Easily get an string from a CException
CString MessageFromException(CException* p_exception);

typedef void(*SeTranslatorFunc)(UINT, _EXCEPTION_POINTERS*);

void SeTranslator(UINT p_safe,_EXCEPTION_POINTERS* p_exceptionPointers);

#endif //__SEEXCEPTION_H__