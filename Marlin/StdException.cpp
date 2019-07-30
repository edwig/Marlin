//////////////////////////////////////////////////////////////////////////
//
// StdException. Catches Safe Exceptions and normal exceptions alike
//
// Code based on original idea of "Martin Ziacek" on www.codeproject.com
// Exception class is now **NOT** based on any base class
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "StdException.h"

#ifndef __STDEXCEPTION_IMPL__
#define __STDEXCEPTION_IMPL__

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Macro to help with the display switch in GetErrorMessage()
#define CASE(seCode,errorstring) case EXCEPTION_##seCode: \
										errorstring.Format(_T("Exception %s (0x%.8X) at address 0x%.8X."),_T(#seCode),EXCEPTION_##seCode,m_exceptionPointers->ExceptionRecord->ExceptionAddress); \
										break;

// Setting our Safe-Exception-Translator function
// So that the C++ runtime can call and create our StdException
void 
SeTranslator(UINT p_safeExceptionCode,_EXCEPTION_POINTERS* pExcPointers)
{
	throw StdException(p_safeExceptionCode,pExcPointers);
}

// CTOR: Create exception from Safe-Exception
StdException::StdException(UINT p_safeExceptionCode,_EXCEPTION_POINTERS* p_exceptionPointers)
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

// CTOR: Create exception from static text or CString
StdException::StdException(const char* p_fault)
             :m_applicationFault(p_fault)
{
}

StdException::StdException(const CString& p_fault)
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
UINT 
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
CString
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
PVOID 
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
CString
StdException::GetErrorMessage()
{
  CString errorstring;

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
                errorstring.Format("Error: %d ",m_applicationCode);
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
BOOL
StdException::GetErrorMessage(LPTSTR p_error,UINT p_maxSize,PUINT p_helpContext /*= NULL*/)
{
  // Reset help context
  if(p_helpContext)
  {
    *p_helpContext = 0;
  }
  // Get compound error message
  CString error = GetErrorMessage();
  // Copy it out
  strncpy_s(p_error,p_maxSize,error.GetString(),error.GetLength() + 1);
  return TRUE;
}

#ifndef MARLIN_USE_ATL_ONLY
CString
MessageFromException(CException& p_exception)
{
  char buffer[4 * _MAX_PATH + 1];
  p_exception.GetErrorMessage(buffer,4 * _MAX_PATH);
  return CString(buffer);
}
#endif//MARLIN_USE_ATL_ONLY

#endif // __STDEXCEPTION_IMPL__
