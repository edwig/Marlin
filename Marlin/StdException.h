//////////////////////////////////////////////////////////////////////////
//
// StdException. Catches Safe Exceptions and normal exceptions alike
//
// Code based on original idea of "Martin Ziacek" on www.codeproject.com
// Exception class is now **NOT** based on any base class
//
//////////////////////////////////////////////////////////////////////////

#ifndef __STDEXCEPTION_H__
#define __STDEXCEPTION_H__

#include <eh.h>

// Macro to re-throw a safe exception
#define ReThrowSafeException(ex) if(ex.GetSafeExceptionCode()) throw ex;

class StdException 
{
public:
  // Application type constructors
  StdException(int p_errorCode);
  StdException(const char* p_fault);
  StdException(const CString& p_fault);
  StdException(int p_errorCode,const char* p_fault);
  // Construct from a SafeExceptionHandler (SEH)
	StdException(UINT p_safe,_EXCEPTION_POINTERS* p_exceptionPointers);
	StdException(const StdException& p_other);

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
  CString              m_applicationFault;
};

void SeTranslator(UINT p_safe,_EXCEPTION_POINTERS* p_exceptionPointers);

// Translate CException to CString
CString MessageFromException(CException& p_exception);

// Auto class to store the SE translator function
// Of an original source system, while setting our own
class AutoSeTranslator
{
public:
  AutoSeTranslator(_se_translator_function p_func)
  {
    m_original = _set_se_translator(p_func);
  }
  ~AutoSeTranslator()
  {
    _set_se_translator(m_original);
  }

private:
  _se_translator_function m_original;
};

#endif //__STDEXCEPTION_H__