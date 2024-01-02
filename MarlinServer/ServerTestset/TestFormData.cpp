/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestFormData.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2024 ir. W.E. Huisman
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
#include "TestMarlinServer.h"
#include "SiteHandlerFormData.h"
#include "MultiPartBuffer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int totalChecks = 5;

//////////////////////////////////////////////////////////////////////////
//
// Form Data POST handler
//
//////////////////////////////////////////////////////////////////////////

class FormDataHandler : public SiteHandlerFormData
{
protected:
  int PreHandleBuffer (HTTPMessage* p_message,MultiPartBuffer* p_buffer);
  int HandleData      (HTTPMessage* p_message,MultiPart*       p_part);
  int HandleFile      (HTTPMessage* p_message,MultiPart*       p_part);
  int PostHandleBuffer(HTTPMessage* p_message,MultiPartBuffer* p_buffer);
private:
  int m_parts { 0 };
};

int 
FormDataHandler::PreHandleBuffer(HTTPMessage* /*p_message*/,MultiPartBuffer* p_buffer)
{
  // Resetting the m_parts counter to the number of parts received
  m_parts = (int) p_buffer->GetParts();
  --totalChecks;
  return 0;
}

int 
FormDataHandler::HandleData(HTTPMessage* /*p_message*/,MultiPart* p_part)
{
  SITE_DETAILLOGS(_T("Handling form-data data-part: "),p_part->GetName());

  XString data = p_part->GetData();
  XString name = p_part->GetName();
  xprintf(_T("MULTI-PART DATA = Name : %s\n"),p_part->GetName().GetString());
  xprintf(_T("MULTI-PART Content-type: %s\n"),p_part->GetContentType().GetString());
  xprintf(_T("MULTI-PART\n%s\n"),             p_part->GetData().GetString());

  // Remember the fact that we where called
  bool result = !data.IsEmpty() || !name.IsEmpty();

  // Total result
  if (result)
  {
    --m_parts;
    --totalChecks;
  }

  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("Multi-part form-data - data part               : %s\n"),result ? _T("OK") : _T("ERROR"));
  return result ? 0 : 1;
}

int 
FormDataHandler::HandleFile(HTTPMessage* /*p_message*/,MultiPart* p_part)
{
  SITE_DETAILLOGV(_T("Handling form-data file-part: [%s] %s"),p_part->GetName().GetString(),p_part->GetShortFileName().GetString());

  xprintf(_T("MULTI-PART FILE = Name : %s\n"),p_part->GetName().GetString());
  xprintf(_T("MULTI-PART Content-type: %s\n"),p_part->GetContentType().GetString());
  xprintf(_T("MULTI-PART Filename    : %s\n"),p_part->GetShortFileName().GetString());
  xprintf(_T("File date creation     : %s\n"),p_part->GetDateCreation().GetString());
  xprintf(_T("File date modification : %s\n"),p_part->GetDateModification().GetString());
  xprintf(_T("File date last-read    : %s\n"),p_part->GetDateRead().GetString());
  xprintf(_T("File indicated size    : %d\n"),static_cast<int>(p_part->GetSize()));

  // Keep debugging things together, by resetting the filename
  XString filename = MarlinConfig::GetExePath() + p_part->GetShortFileName();
  p_part->SetFileName(filename);
  // Re-write the file part buffer + optional file times.
  bool result = p_part->WriteFile();

  // Remember the fact that we where called
  m_parts -= result ? 1 : 0;

  // Check done
  if(result)
  {
    --totalChecks;
  }
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("Multi-part form-data - file part               : %s\n"),result ? _T("OK") : _T("ERROR"));
  return result ? 0 : 1;
}

int 
FormDataHandler::PostHandleBuffer(HTTPMessage* p_message,MultiPartBuffer* p_buffer)
{
  // Essentially, test if all parts where received!
  // By checking the m_parts counter in the class
  bool result = m_parts == 0 && p_buffer->GetParts() == 3;

  // Check done
  if(result)
  {
    --totalChecks;
  }

  XString resultString;
  resultString.Format(_T("RESULT=%s\n"),result ? _T("OK") : _T("ERROR"));
  p_message->SetContentType(_T("text/plain; charset=utf-8"));
  p_message->AddBody(resultString);

  // SUMMARY OF THE TEST
  //  --- "---------------------------------------------- - ------
  qprintf(_T("Multi-part form-data - total test              : %s\n"),result ? _T("OK") : _T("ERROR"));
  return 0;  
}

//////////////////////////////////////////////////////////////////////////
//
// TESTING
//
//////////////////////////////////////////////////////////////////////////

int 
TestMarlinServer::TestFormData()
{
  int error = 0;

  // If errors, change detail level
  m_doDetails = false;

  XString url(_T("/MarlinTest/FormData/"));

  xprintf(_T("TESTING FORM-DATA MULTI-PART-BUFFER FUNCTION OF THE HTTP SERVER\n"));
  xprintf(_T("===============================================================\n"));

  // Create HTTP site to listen to "http://+:port/MarlinTest/FormData/"
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,url);
  if(site)
  {
    // SUMMARY OF THE TEST
    //  --- "--------------------------- - ------\n"
    qprintf(_T("HTTPSite Form-data          : OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot make a HTTP site for: %s\n"),url.GetString());
    return error;
  }

  // Setting the POST handler for this site
  site->SetHandler(HTTPCommand::http_post,new FormDataHandler());

  // Modify the standard settings for this site
  site->AddContentType(_T(""),_T("multipart/form-data;"));

  // Start the site explicitly
  if(site->StartSite())
  {
    xprintf(_T("Site started correctly: %s\n"),url.GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR STARTING SITE: %s\n"),url.GetString());
  }
  return error;
}

int
TestMarlinServer::AfterTestFormData()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("Form-data multi-buffer test complete           : %s\n"),totalChecks > 0 ? _T("ERROR") : _T("OK"));
  return totalChecks > 0;
}
