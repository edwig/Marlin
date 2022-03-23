//////////////////////////////////////////////////////////////////////////
//
// Getting the framework settings for this library/program
// Uses the $(SolutionDir)Framework.h file for configuration of 
// the MFC XString or the std::string SMX_String
//
#pragma once

#define WIN32_LEAN_AND_MEAN                     // Exclude rarely-used stuff from Windows headers

// remove support for MFC controls in dialogs
#define _AFX_NO_MFC_CONTROLS_IN_DIALOGS         
// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include "..\framework.h"


#include <stdio.h>

#ifndef _ATL
//
#define TRACE  ATLTRACE
#define ASSERT ATLASSERT
#define VERIFY ATLVERIFY
#ifdef _DEBUG
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
//use XString in ATL
#include <atlstr.h>
#endif // _ATL

//////////////////////////////////////////////////////////////////////////
//
// Can be extended beyond this point with extra MFC requirements
//
//////////////////////////////////////////////////////////////////////////

// Extras needed for MFC in this program
#include <afxext.h>         // MFC extensions
