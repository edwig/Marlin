/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Framework.h
//
// BaseLibrary: Indispensable general objects and functions
// 
// Copyright (c) 2014-2024 ir. W.E. Huisman
// All rights reserved
//
#pragma once

/////////////////////////////////////////////////////////////////////////////////
//
// This file determines whether your project is using MFC XString for a string
// or uses the SMX_String (derived from std::string) class.
//

// Included in an Microsoft Application Framework eXtension environment (MFC)

// Un-comment when building the production MarlinModule !
// #include <tchar.h>
// For other purposes use the MFC AFX main header
#include <afx.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some XString constructors will be explicit
#include <atlstr.h>

/////////////////////////////////////////////////////////////////////////////////
//
// Comment out the three lines above to use the SMX_String class.
//
/////////////////////////////////////////////////////////////////////////////////
