/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: StdException.h
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

#pragma once
#include "XString.h"
#include <eh.h>

// Macro to re-throw a safe exception
#define ReThrowSafeException(ex) if(ex.GetSafeExceptionCode()) throw ex;

class StdException 
{
public:
  // Application type constructors
  explicit StdException(int p_errorCode);
  explicit StdException(const PTCHAR p_fault);
  explicit StdException(const XString& p_fault);
  explicit StdException(int p_errorCode,LPCTSTR p_fault);
  // Construct from a SafeExceptionHandler (SEH)
	StdException(unsigned p_safe,_EXCEPTION_POINTERS* p_exceptionPointers);
	StdException(const StdException& p_other);

  unsigned             GetSafeExceptionCode() const;
  _EXCEPTION_POINTERS* GetExceptionPointers() const;
  void*                GetExceptionAddress() const;
  int                  GetApplicationCode() const;
  XString              GetApplicationFault() const;
  XString              GetErrorMessage() const;
  bool                 GetErrorMessage(PTCHAR p_error,unsigned p_maxSize,unsigned* p_helpContext = NULL) const;

protected:
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
