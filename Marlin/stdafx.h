// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#pragma once

#include "targetver.h"
#include "framework.h"

// Do not autolink with Marlin from within the library itself
#define MARLIN_NOAUTOLINK

// Use the base-library for the general components and string class
#include <BaseLibrary.h>
