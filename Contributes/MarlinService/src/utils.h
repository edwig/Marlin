#ifndef MARLINWEBSERVER_UTILS_H
#define MARLINWEBSERVER_UTILS_H
/*
MIT License

Copyright (c) 2019 Yuguo Zhang 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once

class utils
{
public:
  static BOOL AmIWellKnown(WELL_KNOWN_SID_TYPE sidType);
  static DWORD GetCurrentSessionId();

	static BOOL InstallService(PWSTR pszServiceName, 
											PWSTR pszDisplayName,
											PWSTR pszDescription,
											DWORD dwStartType,
											PWSTR pszDependencies, 
											PWSTR pszAccount, 
											PWSTR pszPassword);


	static BOOL UninstallService(PWSTR pszServiceName);

  static INT MsgBoxFmt(UINT uiStyle, LPCWSTR pwszTitle, WCHAR* pwszFmt, ...);
  static INT MsgBoxFmt(UINT uiStyle, const char *pszTitle, char *pszFmt, ...);

  static void LogFmt(WCHAR* pwszFmt, ...);
  static void LogFmt(char *pszFmt, ...);
private:
};

#endif//MARLINWEBSERVER_UTILS_H
