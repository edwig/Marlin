/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: StdException.cpp
//
// BaseLibrary: Indispensable general objects and functions
// 
// Copyright (c) 2014-2024 ir. W.E. Huisman
// All rights reserved
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

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

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

// Macro to help with the display switch in GetErrorMessage()
#define CASE(seCode,errorstring) case EXCEPTION_##seCode: \
                    errorstring.Format(_T("Exception %s (0x%.8X) at address 0x%.8I64X."),_T(#seCode),(unsigned int)EXCEPTION_##seCode,(INT64)m_exceptionPointers->ExceptionRecord->ExceptionAddress); \
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
  m_applicationCode   = p_other.m_applicationCode;
  m_applicationFault  = p_other.m_applicationFault;
}

// CTOR: Create exception from static text or XString
StdException::StdException(const PTCHAR p_fault)
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

StdException::StdException(int p_errorCode,LPCTSTR p_fault)
             :m_applicationCode(p_errorCode)
             ,m_applicationFault(p_fault)
{
}

// Return the Safe-Exception code
unsigned
StdException::GetSafeExceptionCode() const
{
	return m_safeExceptionCode;
}

// Return the application error code
int
StdException::GetApplicationCode() const
{
  return m_applicationCode;
}

// Return the application error text
XString
StdException::GetApplicationFault() const
{
  return m_applicationFault;
}

// Return the exception pointers (build stack traces etcetera)
_EXCEPTION_POINTERS* 
StdException::GetExceptionPointers() const
{
	return m_exceptionPointers;
}

// Address where the exception did occur
void* 
StdException::GetExceptionAddress() const
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
StdException::GetErrorMessage() const
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
                errorstring.Format(_T("Error: %u"), m_applicationCode);
              }
              errorstring += m_applicationFault;
              break;
	  default:  errorstring = _T("Unknown exception.");
		          break;
	}
	return errorstring;
}

// The CException way of getting an error message
// By copying it out of the object through a string pointer
bool
StdException::GetErrorMessage(PTCHAR p_error, unsigned p_maxSize, unsigned* p_helpContext /*= NULL*/) const
{
  // Reset help context
  if (p_helpContext)
  {
    *p_helpContext = 0;
  }
  // Get compound error message
  XString error = GetErrorMessage();
  // Copy it out
  _tcsncpy_s(p_error, p_maxSize, error.GetString(),(size_t) error.GetLength() + 1);
  return true;
}

#ifdef _AFX
XString
MessageFromException(CException& p_exception)
{
  TCHAR buffer[4 * _MAX_PATH + 1];
  p_exception.GetErrorMessage(buffer,4 * _MAX_PATH);
  return XString(buffer);
}
#endif
