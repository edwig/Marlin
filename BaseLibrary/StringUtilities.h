/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: StringUtilities.h
//
// BaseLibrary: Indispensable general objects and functions
// 
// Copyright (c) 2014-2025 ir. W.E. Huisman
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
#include <vector>

// Get a number (int,double) as a string
XString AsString(int p_number,int p_radix = 10);
XString AsString(double p_number);

// Convert a string to int,double,bcd
int     AsInteger(XString p_string);
double  AsDouble(XString p_string);
bcd     AsBcd(XString p_string);

int     SplitString(XString p_string,std::vector<XString>& p_vector,TCHAR p_separator = _T(','),bool p_trim = false);
void    NormalizeLineEndings(XString& p_string);

// Find the position of the matching bracket
// starting at the bracket in the parameters bracketPos
//
int     FindMatchingBracket(const CString& p_string,int p_bracketPos);

// Split arguments with p_splitter not within brackets
// p_pos must be 0 initially
bool    SplitArgument(int& p_pos,const CString& p_data,TCHAR p_splitter,CString& p_argument);

// Unicode aware Clipboard handling
XString GetStringFromClipboard(HWND p_wnd = NULL);
bool    PutStringToClipboard(XString p_string,HWND p_wnd = NULL,bool p_append = false);

// Count the number of instances of a character in a string
int     CountOfChars(XString p_string,TCHAR p_char);
