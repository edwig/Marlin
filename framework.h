/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Framework.h
//
// BaseLibrary: Indispensable general objects and functions
// 
// Copyright (c) 2014-2022 ir. W.E. Huisman
// All rights reserved
//
#pragma once

/////////////////////////////////////////////////////////////////////////////////
//
// This file determines wheter your project is using MFC XString for a string
// or uses the SMX_String (derived from std::string) class.
//

// Included in an Microsoft Application Framework eXtension environment (MFC)
#include <tchar.h>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some XString constructors will be explicit
#include <afx.h>

/////////////////////////////////////////////////////////////////////////////////
//
// Comment out the three lines above to use the SMX_String class.
//
/////////////////////////////////////////////////////////////////////////////////
