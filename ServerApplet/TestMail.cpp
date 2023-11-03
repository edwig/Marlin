/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestMail.cpp
//
// Marlin Component: Internet server/client
// 
// Copyright (c) 2014-2022 ir. W.E. Huisman
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
#include "stdafx.h"
#include "TestMail.h"
#include "ServerApplet.h"
#include <WinFile.h>
#include <version.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

TestMail::TestMail(XString p_afzender,XString p_ontvanger,XString p_server)
         :m_sender(p_afzender)
         ,m_receiver(p_ontvanger)
         ,m_server(p_server)
{
}

bool
TestMail::Send()
{
  int cc = 0;
  WinFile file;
  file.CreateTempFileName(_T("testmail"),_T("txt"));
  if(file.Open(winfile_write,FAttributes::attrib_none,Encoding::UTF8))
  {
    file.Format(_T("HOST:%s\n"),m_server);
    file.Format(_T("FROM:%s\n"),m_sender);
    file.Format(_T("TO:%s\n"),  m_receiver);
    file.Write(_T("SUBJECT:TEST e-mail from the ServerApplet of the test program\n"));
    file.Write(_T("DIALOOG:nee\n"));
    file.Write(_T("WIJZIGONDERWERP:nee\n"));
    file.Write(_T("WIJZIGBERICHT:nee\n"));
    file.Write(_T("VOORTGANG:ja\n"));
    file.Write(_T("VERWIJDEREN:ja\n"));
    file.Write(_T("<BODY>\n"));
    file.Write(_T("Dear system maintainer,\n"));
    file.Write(_T("\n"));
    file.Write(_T("This is a test e-mail to see if you have configured everything well.\n"));
    file.Write(_T("If this e-mail reaches you, it is a proof that the Marlin modules can send crash rapports to you.\n"));
    file.Write(_T("\n"));
    file.Write(_T("With kindly regards,\n"));
    file.Write(_T("The Marlin team.\n"));

    // Close the parameter file
    file.Close();

    // Send via POSTMAIL
    XString program(_T("PostMail5.exe"));
    XString melding(_T("Cannot find the e-mail program 'PostMail5' in the executable directory"));
    cc = theApp.StartProgram(program,file.GetFilename(),true,melding);
  }
  return (cc == 0);
}

//////////////////////////////////////////////////////////////////////////
//
// Service functions
//
//////////////////////////////////////////////////////////////////////////

static XString
ErrorString(int p_error)
{
  TCHAR buffer[1024];
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                p_error,
                MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), // Default language
                (LPTSTR)buffer,
                1024,
                NULL);
  return XString(buffer);
}
