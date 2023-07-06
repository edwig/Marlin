/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestUnicode.h
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
#include "stdafx.h"
#include "TestClient.h"
#include "ConvertWideString.h"
#include "CrackURL.h"
#include <atlconv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef unsigned char uchar;

void PrintBytes(void* p_buffer,int p_length)
{
  xprintf("TESTING BUFFER:\n");
  xprintf("---------------\n");

  for(int ind = 0; ind < p_length; ++ind)
  {
    unsigned ch = ((uchar*)p_buffer)[ind];
    xprintf("  byte: %02X %c\n",ch,ch);
  }
  xprintf("\n");
}

int TestUnicode(void)
{
//const char *text = "Sôn bôn de magnà el véder, el me fa minga mal.";
  uchar* buffer = NULL;
  const char* text = "áàäâéèëêíìïîóòöôúùüûýÿçñõã";
  int   length = 0;
  int   errors = 1;
  bool  doBom  = true;
  bool  foundBOM = false;

  PrintBytes((void*)text,(int)strlen(text));

  // Try to convert to Unicode UTF-16
  if(TryCreateWideString(text,"",doBom,&buffer,length))
  {
    PrintBytes(buffer,length);

    XString result;
    if(TryConvertWideString(buffer,length,"",result,foundBOM))
    {
      PrintBytes((void*)result.GetString(),result.GetLength());
    }
    // Do not forget to free the buffer
    delete [] buffer;

    if(result.Compare(text) == 0)
    {
      --errors;
    }
  }

  // SUMMARY OF THE TEST
  printf("Testcase                                         Result\n");
  printf("---------------------------------------------- - ------\n");
  printf("Testing Unicode conversions on diacritic chars : %s\n",errors ? "ERROR" : "OK");

  return errors;
}

int TestURLChars(void)
{
  // Number of tests
  int errors = 2; 

  XString urlString("http://" MARLIN_HOST "/Marlin Test all/URL?first=abc&second=áàäâéèëêíìïîóòöôúùüûýÿçñõã");
  XString mustBeUrl("http://" MARLIN_HOST "/Marlin%20Test%20all/URL?first=abc&second=%C3%A1%C3%A0%C3%A4%C3%A2%C3%A9%C3%A8%C3%AB%C3%AA%C3%AD%C3%AC%C3%AF%C3%AE%C3%B3%C3%B2%C3%B6%C3%B4%C3%BA%C3%B9%C3%BC%C3%BB%C3%BD%C3%BF%C3%A7%C3%B1%C3%B5%C3%A3");
  CrackedURL url1(urlString);
  XString encoded = url1.URL();

  // Test if success
  if(encoded == mustBeUrl)
  {
    --errors;
  }

  // Encoding of higher order UTF-8 like the euro signs
  urlString = "http://" MARLIN_HOST "/Een%20Twee%20drie?een=waarde=%3Crequest%3E%3Cpage%3Eklantenkaart%3C%2Fpage%3E%3Crequesttype%3Econtactmemoaanmaken%3C%2Frequesttype%3E%3Ccallback%3EdesktopCallback%3C%2Fcallback%3E%3Cparameters%3E%3Cparameter%3E%3Cname%3Econtactmemogegevens%3C%2Fname%3E%3Cvalue%3E%7B%22contact_id%22:151714,%22eerste_contact_id%22:151714,%22werknemer_id%22:2942,%22werknemer_naam%22:%22N.N.+Bhoera%22,%22beheerorgnaam%22:%22Edwin%22,%22richting%22:%22uit%22,%22onderwerp%22:%22H%C3%AF%C3%ABr+st%C3%A4%C3%A4n+%E2%82%AC+test%22%7D%3C%2Fvalue%3E%3C%2Fparameter%3E%3C%2Fparameters%3E%3C%2Frequest%3E";
  CrackedURL url2(urlString);
  encoded = url2.URL();

  // Test if success
  if(urlString == encoded)
  {
    --errors;
  }

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Testing URL %% encoding and Unicode chars       : %s\n",errors ? "ERROR" : "OK");

  return errors;
}

int TestCrackURL(void)
{
  int errors = 3;

  XString urlString1("https://" MARLIN_HOST "/One/Two/tim.berners-lee/room/error404");
  XString urlString2("https://" MARLIN_HOST "/One/Two/tim.berners-lee/room/error404.html");
  XString urlString3("https://" MARLIN_HOST "/One/Two/tim.berners-lee/room/error404.html?par1=cern&par2=geneve");
  CrackedURL url1(urlString1);
  CrackedURL url2(urlString2);
  CrackedURL url3(urlString3);

  if(url1.GetExtension().IsEmpty())
  {
    --errors;
  }
  if(url2.GetExtension() == "html")
  {
    --errors;
  }
  if(url3.GetExtension() == "html")
  {
    --errors;
  }
  return errors;
}