/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestMail.cpp
//
// Marlin Component: Internet server/client
// 
// Copyright (c) 2014-2021 ir. W.E. Huisman
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
#include <version.h>

static FILE* CreateTempFile(CString& p_filename,char* p_mode);

TestMail::TestMail(CString p_afzender,CString p_ontvanger,CString p_server)
         :m_sender(p_afzender)
         ,m_receiver(p_ontvanger)
         ,m_server(p_server)
{
}

bool
TestMail::Send()
{
  int cc = 0;
  CString filename("testmail");
  FILE* file = CreateTempFile(filename,(char*)"w");
  if(file)
  {
    fprintf(file,"HOST:%s\n",m_server);
    fprintf(file,"FROM:%s\n",m_sender);
    fprintf(file,"TO:%s\n",  m_receiver);
    fprintf(file,"SUBJECT:TEST e-mail from the ServerApplet of the test program\n");
    fprintf(file,"DIALOOG:nee\n");
    fprintf(file,"WIJZIGONDERWERP:nee\n");
    fprintf(file,"WIJZIGBERICHT:nee\n");
    fprintf(file,"VOORTGANG:ja\n");
    fprintf(file,"VERWIJDEREN:ja\n");
    fprintf(file,"<BODY>\n");
    fprintf(file,"Dear system maintainer,\n");
    fprintf(file,"\n");
    fprintf(file,"This is a test e-mail to see if you have configured everything well.\n");
    fprintf(file,"If this e-mail reaches you, it is a proof that the Marlin modules can send crash rapports to you.\n");
    fprintf(file,"\n");
    fprintf(file,"With kindly regards,\n");
    fprintf(file,"The Marlin team.\n");

    // Close the parameter file
    fclose(file);

    // Send via POSTMAIL
    CString program("PostMail5.exe");
    CString melding("Cannot find the e-mail program 'PostMail5' in the executable directory");
    cc = theApp.StartProgram(program,filename,true,melding);
  }
  return (cc == 0);
}

//////////////////////////////////////////////////////////////////////////
//
// Service functions
//
//////////////////////////////////////////////////////////////////////////

static CString
ErrorString(int p_error)
{
  char buffer[1024];
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                p_error,
                MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), // Default language
                (LPTSTR)buffer,
                1024,
                NULL);
  return CString(buffer);
}

// Create a temporary file from a template, returning:
// 1) A file pointer to an open file, or NULL, and an error message
// 2) The name of the temporary file
static FILE* 
CreateTempFile(CString& p_filename,char* p_mode)
{
  // Set up a file
  char tempname[MAX_PATH + 1];
  size_t size = 0;
  getenv_s(&size,tempname,MAX_PATH,"TMP");
  char* file = _tempnam(tempname,p_filename);

  FILE* pointer = NULL;
  int error = fopen_s(&pointer,file,p_mode);
  if(pointer)
  {
    p_filename = file;
  }
  else
  {
    p_filename.Format("ERROR [%d] %s",error,ErrorString(error));
  }
  // Free temp space
  free(file);
  // Result of the fopen_s call
  return pointer;
}
