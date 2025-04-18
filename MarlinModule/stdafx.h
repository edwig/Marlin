// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.
#pragma once

// add headers that you want to pre-compile here
#include "targetver.h"
#include "framework.h"

#include <stdio.h>
#include <tchar.h>

#ifndef _ATL
//
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some XString constructors will be explicit
//
#define TRACE  ATLTRACE
#define ASSERT ATLASSERT
#define VERIFY ATLVERIFY
#ifdef _DEBUG
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif

// Auto link with BaseLibrary
#include <BaseLibrary.h>

