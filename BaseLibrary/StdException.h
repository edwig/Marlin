//////////////////////////////////////////////////////////////////////////
//
// StdException. Catches Safe Exceptions and normal exceptions alike
//
// Code based on original idea of "Martin Ziacek" on www.codeproject.com
// Exception class is now **NOT** based on any base class
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "XString.h"
#include <eh.h>

// Macro to re-throw a safe exception
#define ReThrowSafeException(ex) if(ex.GetSafeExceptionCode()) throw ex;

class StdException 
{
public:
  // Application type constructors
  StdException(int p_errorCode);
  StdException(const char* p_fault);
  StdException(const XString& p_fault);
  StdException(int p_errorCode,const char* p_fault);
  // Construct from a SafeExceptionHandler (SEH)
	StdException(unsigned p_safe,_EXCEPTION_POINTERS* p_exceptionPointers);
	StdException(const StdException& p_other);

  unsigned             GetSafeExceptionCode();
	_EXCEPTION_POINTERS* GetExceptionPointers();
	void*                GetExceptionAddress();
  int                  GetApplicationCode();
  XString              GetApplicationFault();
  XString              GetErrorMessage();
  bool                 GetErrorMessage(char* p_error,unsigned p_maxSize,unsigned* p_helpContext = NULL);

private:
  unsigned             m_safeExceptionCode { 0 };
	_EXCEPTION_POINTERS* m_exceptionPointers { nullptr };
  unsigned             m_applicationCode   { 0 };
  XString              m_applicationFault;
};

void SeTranslator(unsigned p_safe,_EXCEPTION_POINTERS* p_exceptionPointers);
#ifdef _ATL
// Translate CException to XString
XString MessageFromException(CException& p_exception);
#endif

// Auto class to store the SE translator function
// Of an original source system, while setting our own
class AutoSeTranslator
{
public:
  explicit AutoSeTranslator(_se_translator_function p_func): m_original(_set_se_translator(p_func))
  {
  }
  ~AutoSeTranslator()
  {
    _set_se_translator(m_original);
  }

private:
  _se_translator_function m_original;
};
