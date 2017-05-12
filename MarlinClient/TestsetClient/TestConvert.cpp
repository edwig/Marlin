/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestConvert.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2017 ir. W.E. Huisman
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
#include "ConvertWideString.h"
#include "SOAPMessage.h"
#include "TestClient.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern void WaitForKey();

int 
TestConvertFile(CString p_fileName,CString p_extraFile)
{
  bool result = false;
  xprintf("\nTESTING\n=======\n");

  FILE* file = nullptr;
  fopen_s(&file,p_fileName,"r");
  if(file)
  {
    CString inhoud;
    char buffer[1001];
    while(fgets(buffer,1000,file))
    {
      inhoud += CString(buffer);
    }
    fclose(file);

    xprintf("Message:\n%s\n",inhoud.GetString());

    SOAPMessage msg;
    msg.SetSoapVersion(SoapVersion::SOAP_10);
    msg.ParseMessage(inhoud);

    if(msg.GetInternalError() == XmlError::XE_NoError)
    {
      CString string1,string2;

      XMLElement* vhe1 = msg.FindElement("Eenheid");
      if(vhe1)
      {
        XMLElement* adres1 = msg.FindElement(vhe1,"Adres");
        if(adres1)
        {
          string1 = adres1->GetValue();
        }

        XMLElement* vhe2 = msg.GetElementSibling(vhe1);
        if(vhe2)
        {
          XMLElement* adres2 = msg.FindElement(vhe2,"Adres");
          if(adres2)
          {
            string2 = adres2->GetValue();
          }
        }
        xprintf("First  address: %s\n", string1.GetString());
        xprintf("Second address: %s\n", string2.GetString());

        if(doDetails)
        {
          MessageBox(NULL,string1,"First",MB_OK);
          MessageBox(NULL,string2,"Second",MB_OK);
        }
        else
        {
          if(string1 == "¡¿ƒ¬…»À ÕÃœŒ”“÷‘⁄Ÿ‹€›_«—’√" &&
             string2 == "·‡‰‚ÈËÎÍÌÏÔÓÛÚˆÙ˙˘¸˚˝ˇÁÒı„")
          {
            result = true;
          }
          // SUMMARY OF THE TEST
          // --- "---------------------------------------------- - ------
          printf("Tesing UTF-8 encoding on XML files             : %s\n",result ? "OK" : "ERROR");
        }
      }

      CString resultaat = msg.Print();
      xprintf("OK\nMessage is now:\n%s\n",resultaat.GetString());

      if(doDetails)
      {
        FILE* out = nullptr;
        fopen_s(&out,p_extraFile,"w");
        if(out)
        {
          fprintf(out,resultaat);
          fclose(out);
        }
      }
    }
    else
    {
      printf("Message not read in\n");
    }
  }
  else
  {
    printf("File not opened: %s\n",p_fileName.GetString());
  }
  return result ? 0 : 1;
}

#include <windows.h>

// 65001 is utf-8.
wchar_t *CodePageToUnicode(int codePage, const char *src)
{
  if (!src) return 0;
  int srcLen = (int) strlen(src);
  if (!srcLen)
  {
    wchar_t *w = new wchar_t[1];
    w[0] = 0;
    return w;
  }

  int requiredSize = MultiByteToWideChar(codePage,0,src, srcLen,0,0);

  if (!requiredSize)
  {
    return 0;
  }

  wchar_t *w = new wchar_t[requiredSize + 1];
  w[requiredSize] = 0;

  int retval = MultiByteToWideChar(codePage,0,src,srcLen,w,requiredSize);
  if (!retval)
  {
    delete[] w;
    return 0;
  }

  return w;
}

char *UnicodeToCodePage(int codePage, const wchar_t *src)
{
  if (!src) return 0;
  int srcLen = (int)wcslen(src);
  if (!srcLen)
  {
    char *x = new char[1];
    x[0] = '\0';
    return x;
  }

  int requiredSize = WideCharToMultiByte(codePage,0,src,srcLen,0,0,0,0);

  if (!requiredSize)
  {
    return 0;
  }

  char *x = new char[requiredSize + 1];
  x[requiredSize] = 0;

  int retval = WideCharToMultiByte(codePage,0,src,srcLen,x,requiredSize,0,0);
  if (!retval)
  {
    delete[] x;
    return 0;
  }
  return x;
}

int 
TestConvert()
{
//   const char *text = "SÙn bÙn de magn‡ el vÈder, el me fa minga mal.";
// 
//   // Convert ANSI (Windows-1252, i.e. CP1252) to utf-8:
//   wchar_t *wText = CodePageToUnicode(1252, text);
// 
//   char *utf8Text = UnicodeToCodePage(65001, wText);
// 
//   FILE *fp = fopen("utf8File.txt", "w");
//   fprintf(fp, "%s\n", utf8Text);
//   fclose(fp);
// 
//   // Now convert utf-8 back to ANSI:
//   wchar_t *wText2 = CodePageToUnicode(65001, utf8Text);
// 
//   char *ansiText = UnicodeToCodePage(1252, wText2);
// 
//   fp = fopen("ansiFile.txt", "w");
//   fprintf(fp, "%s\n", ansiText);
//   fclose(fp);
// 
//   delete[] ansiText;
//   delete[] wText2;
//   delete[] wText;
//   delete[] utf8Text;

  xprintf("TESTING ENCODING READING/WRITING FOR UTF-8 MESSAGES IN XML\n");
  xprintf("==========================================================\n");

  return TestConvertFile("utf8File.txt","utf8FileConverted.txt");
}