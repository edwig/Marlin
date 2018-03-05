/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Version.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
#pragma once

// Version number components
#define MARLIN_PRODUCT_NAME     "MarlinServer"   // Our name
#define MARLIN_VERSION_NUMBER   "4.7.3"          // The real version
#define MARLIN_VERSION_BUILD    ""               // Can carry strings like 'Alpha', 'Beta', 'RC'
#define MARLIN_VERSION_DATE     "05-03-2018"     // Last production date

// This is our version string "MarlinServer 4.7.3"
#define MARLIN_SERVER_VERSION MARLIN_PRODUCT_NAME " " MARLIN_VERSION_NUMBER MARLIN_VERSION_BUILD
