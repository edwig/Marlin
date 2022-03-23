//////////////////////////////////////////////////////////////////////////
//
// StdException. Catches Safe Exceptions and normal exceptions alike
//
// Code based on original idea of "Martin Ziacek" on www.codeproject.com
// Exception class is now **NOT** based on any base class
//
//////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "StdException.h"

// Macro to help with the display switch in GetErrorMessage()
#define CASE(seCode,errorstring) case EXCEPTION_##seCode: \
										errorstring.Format(_T("Exception %s (0x%.8X) at address 0x%.8X."),_T(#seCode),EXCEPTION_##seCode,m_exceptionPointers->ExceptionRecord->ExceptionAddress); \
										break;

// Setting our Safe-Exception-Translator function
// So that the C++ runtime can call and create our StdException
void 
SeTranslator(unsigned p_safeExceptionCode,_EXCEPTION_POINTERS* pExcPointers)
{
	throw StdException(p_safeExceptionCode,pExcPointers);
}

// CTOR: Create exception from Safe-Exception
StdException::StdException(unsigned p_safeExceptionCode,_EXCEPTION_POINTERS* p_exceptionPointers)
{ 
  m_safeExceptionCode = p_safeExceptionCode;
	m_exceptionPointers = p_exceptionPointers;
}

// CTOR: Create exception from another one
StdException::StdException(const StdException& p_other)
{
  m_safeExceptionCode = p_other.m_safeExceptionCode;
	m_exceptionPointers = p_other.m_exceptionPointers;
}

// CTOR: Create exception from static text or XString
StdException::StdException(const char* p_fault)
             :m_applicationFault(p_fault)
{
}

StdException::StdException(const XString& p_fault)
             :m_applicationFault(p_fault)
{
}

// CTOR: Simply create from an error code
StdException::StdException(int p_errorCode)
             :m_applicationCode(p_errorCode)
{
}

StdException::StdException(int p_errorCode, const char* p_fault)
             :m_applicationCode(p_errorCode)
             ,m_applicationFault(p_fault)
{
}

// Return the Safe-Exception code
unsigned
StdException::GetSafeExceptionCode()
{
	return m_safeExceptionCode;
}

// Return the application error code
int
StdException::GetApplicationCode()
{
  return m_applicationCode;
}

// Return the application error text
XString
StdException::GetApplicationFault()
{
  return m_applicationFault;
}

// Return the exception pointers (build stack traces etcetera)
_EXCEPTION_POINTERS* 
StdException::GetExceptionPointers()
{
	return m_exceptionPointers;
}

// Address where the exception did occur
void* 
StdException::GetExceptionAddress()
{
  if(m_exceptionPointers)
  {
    if(m_exceptionPointers->ExceptionRecord)
    {
      return m_exceptionPointers->ExceptionRecord->ExceptionAddress;
    }
  }
  return nullptr;
}

// Getting the resulting error as a string
XString
StdException::GetErrorMessage()
{
  XString errorstring;

	switch(m_safeExceptionCode)
  {   
		CASE(ACCESS_VIOLATION,        errorstring);
		CASE(DATATYPE_MISALIGNMENT,   errorstring);
		CASE(BREAKPOINT,              errorstring);
		CASE(SINGLE_STEP,             errorstring);
		CASE(ARRAY_BOUNDS_EXCEEDED,   errorstring);
		CASE(FLT_DENORMAL_OPERAND,    errorstring);
		CASE(FLT_DIVIDE_BY_ZERO,      errorstring);
		CASE(FLT_INEXACT_RESULT,      errorstring);
		CASE(FLT_INVALID_OPERATION,   errorstring);
		CASE(FLT_OVERFLOW,            errorstring);
		CASE(FLT_STACK_CHECK,         errorstring);
		CASE(FLT_UNDERFLOW,           errorstring);
		CASE(INT_DIVIDE_BY_ZERO,      errorstring);
		CASE(INT_OVERFLOW,            errorstring);
		CASE(PRIV_INSTRUCTION,        errorstring);
		CASE(IN_PAGE_ERROR,           errorstring);
		CASE(ILLEGAL_INSTRUCTION,     errorstring);
		CASE(NONCONTINUABLE_EXCEPTION,errorstring);
		CASE(STACK_OVERFLOW,          errorstring);
		CASE(INVALID_DISPOSITION,     errorstring);
		CASE(GUARD_PAGE,              errorstring);
		CASE(INVALID_HANDLE,          errorstring);
    case 0:   if(m_applicationCode)
              {
                errorstring.Format("Error: %d", m_applicationCode);
              }
              errorstring += m_applicationFault;
              break;
	  default:  errorstring = "Unknown exception.";
		          break;
	}
	return errorstring;
}

// The CException way of getting an error message
// By copying it out of the object through a string pointer
bool
StdException::GetErrorMessage(char* p_error, unsigned p_maxSize, unsigned* p_helpContext /*= NULL*/)
{
  // Reset help context
  if (p_helpContext)
  {
    *p_helpContext = 0;
  }
  // Get compound error message
  XString error = GetErrorMessage();
  // Copy it out
  strncpy_s(p_error, p_maxSize, error.GetString(), error.GetLength() + 1);
  return true;
}

#ifdef _ATL
XString
MessageFromException(CException& p_exception)
{
  char buffer[4 * _MAX_PATH + 1];
  p_exception.GetErrorMessage(buffer,4 * _MAX_PATH);
  return XString(buffer);
}
#endif

