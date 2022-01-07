/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ErrorReportPostMail.cpp
//
// WinHTTP Component: Internet server/client
// 
// Copyright (c) 2015 ir. W.E. Huisman
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
#include "StdAfx.h"
#include "ErrorReportPostMail.h"
#include "ProcInfo.h"
#include "StackTrace.h"
#include "WebConfig.h"
#include "EnsureFile.h"
#include "Versie.h"
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Functions from ErrorReport.cpp
extern CString FileTimeToString(FILETIME const& ft,bool p_relative = false);
extern CString ErrorReportWriteToFile(const CString& p_filename
                                     ,const CString& p_report
                                     ,const CString& p_webroot
                                     ,const CString& p_url);

// Standard crash report as a email from the webroot URL directory
//
void
ErrorReportPostMail::DoReport(const CString&    p_subject
                             ,const StackTrace& p_trace
                             ,const CString&    p_webroot
                             ,const CString&    p_url) const
{
  extern const char* PRODUCT_ADMIN_EMAIL;

  // Getting proces information
  std::auto_ptr<ProcInfo> procInfo(new ProcInfo);
  FILETIME creationTime, exitTime, kernelTime, userTime;
  GetProcessTimes(GetCurrentProcess(),
                  &creationTime,
                  &exitTime,
                  &kernelTime,
                  &userTime);

  // Getting the error address
  const CString &errorAddress    = p_trace.FirstAsString();
  const CString &xmlErrorAddress = p_trace.FirstAsXMLString();
  CString receiver(m_receiver);
  if(receiver.IsEmpty() && PRODUCT_ADMIN_EMAIL)
  {
    receiver = CString(PRODUCT_ADMIN_EMAIL);
  }

  CString version;
  version.Format("%s.%s",SHM_VERSION,VERSION_BUILD);

  // Format the message
  CString message = "HOST:" + m_mailServer + "\n"
                    "FROM:" + procInfo->m_username + " op " + procInfo->m_computer + " <" + m_sender + ">\n"
                    "TO:"   + receiver + "\n"
                    "ONDERWERP: " PRODUCT_NAME ": version: " + version + ": " + p_subject + " in " + errorAddress + "\n"
                    "VERWIJDEREN:nee\n"
                    "<BODY>\n"
                  + p_subject + " in " + errorAddress + "\n\n"
                    PRODUCT_NAME "\n"
                    "-------------\n"
                    "Version:       " + SHM_VERSION    + "\n"
                    "Buildnumber:   " + VERSION_BUILD  + "\n"
                    "Builddate:     " + VERSION_DATUM  + "\n\n"
                  + p_trace.AsString() + "\n";

  // Format the attachment
  CString attach =  "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\n"
                    "<foutinformatie xmlns=\"http://xml.centric.nl/2009/Pronto/FoutInformatie\">\n"
                    "<fout>\n"
                    "\t<naam>"
                  + p_subject
                  + "</naam>\n"
                  + "\t<adres>" + xmlErrorAddress + "</adres>\n"
                  + "</fout>\n"
                    "<APPLICATION>\n"
                    "\t<versie>"
                  + version
                  + "</versie>\n"
                    "\t<build>"
                  + VERSION_BUILD
                  + "</build>\n"
                    "\t<builddatum>"
                  + VERSION_DATUM
                  + "</builddatum>\n"
                    "</APPLICATION>\n"
                  + p_trace.AsXMLString();

  /// Info about the proces
  message += "Proces\n"
             "------\n";
  message += "Commandoregel:     " + CString(GetCommandLine())          + "\n";
  message += "Starttijd:         " + FileTimeToString(creationTime)     + "\n";
  message += "Lokale tijd:       " + procInfo->m_datetime               + "\n";
  message += "Processorgebruik:  " + FileTimeToString(userTime, true)   + " (user), "
                                   + FileTimeToString(kernelTime, true) + " (kernel)\n\n";

  attach += "<proces>\n";
  attach += "\t<commandoregel>" + CString(GetCommandLine())           + "</commandoregel>\n";
  attach += "\t<starttijd>"     + FileTimeToString(creationTime)      + "</starttijd>\n";
  attach += "\t<tijd>"          + procInfo->m_datetime                + "</tijd>\n";
  attach += "\t<usertijd>"      + FileTimeToString(userTime, true)    + "</usertijd>\n";
  attach += "\t<kerneltijd>"    + FileTimeToString(kernelTime, true)  + "</kerneltijd>\n";
  attach += "</proces>\n";

  // Info about the system
  message += "Systeem\n"
             "-------\n";
  message += "Computernaam:      " + procInfo->m_computer       + "\n";
  message += "Huidige gebruiker: " + procInfo->m_username       + "\n";
  message += "Windows versie:    " + procInfo->m_platform       + "\n";
  message += "Windows pad:       " + procInfo->m_windows_path   + "\n";
  message += "Huidig pad:        " + procInfo->m_current_path   + "\n";
  message += "Zoekpad:           " + procInfo->m_pathspec       + "\n";
  message += "Numeriek formaat:  " + procInfo->m_system_number  + " (systeem), " + procInfo->m_user_number + " (gebruiker)\n";
  message += "Datumformaat:      " + procInfo->m_system_date    + " (systeem), " + procInfo->m_user_date   + " (gebruiker)\n";
  message += "Tijdformaat:       " + procInfo->m_system_time    + " (systeem), " + procInfo->m_user_time   + " (gebruiker)\n\n";

  attach += "<systeem>\n";
  attach += "\t<computernaam>"  + procInfo->m_computer      + "</computernaam>\n";
  attach += "\t<gebruiker>"     + procInfo->m_username      + "</gebruiker>\n";
  attach += "\t<windowsversie>" + procInfo->m_platform      + "</windowsversie>\n";
  attach += "\t<windowspad>"    + procInfo->m_windows_path  + "</windowspad>\n";
  attach += "\t<huidigpad>"     + procInfo->m_current_path  + "</huidigpad>\n";
  attach += "\t<zoekpad>"       + procInfo->m_pathspec      + "</zoekpad>\n";

  attach += "\t<numeriekformaat><systeem>"
           + procInfo->m_system_number
           + "</systeem><gebruiker>"
           + procInfo->m_user_number
           + "</gebruiker></numeriekformaat>\n";
  attach += "\t<datumformaat><systeem>"
           + procInfo->m_system_date
           + "</systeem><gebruiker>"
           + procInfo->m_user_date
           + "</gebruiker></datumformaat>\n";
  attach += "\t<tijdformaat><systeem>"
           + procInfo->m_system_time
           + "</systeem><gebruiker>"
           + procInfo->m_user_time
           + "</gebruiker></tijdformaat>\n";
  attach += "</systeem>\n";

  // Info about loaded modules
  message += "Modules\n"
             "-------";
  attach += "<modules>\n";

  for(auto module : procInfo->m_modules)
  {
    // Info about this module
    message += "\n" + module->m_full_path + "\n"
               "\tAdres:          " + module->m_load_address      + "\n"
               "\tOriginele naam: " + module->m_original_filename + "\n"
               "\tVersie:         " + module->m_fileversion       + "\n"
               "\tBeschrijving:   " + module->m_file_description  + "\n"
               "\tProductnaam:    " + module->m_product_name      + "\n"
               "\tProductversie:  " + module->m_product_version   + "\n";

    attach +=  "<module pad=\""    + module->m_full_path          + "\">\n"
               "\t<adres>"         + module->m_load_address       + "</adres>\n"
               "\t<originelenaam>" + module->m_original_filename  + "</originelenaam>\n"
               "\t<versie>"        + module->m_fileversion        + "</versie>\n"
               "\t<beschrijving>"  + module->m_file_description   + "</beschrijving>\n"
               "\t<productnaam>"   + module->m_product_name       + "</productnaam>\n"
               "\t<productversie>" + module->m_product_version    + "</productversie>\n"
               "</module>\n";
  }
  attach += "</modules>\n"
            "</foutinformatie>\n";

  // Writing the attachment
  // We use a fixed filename in the hope that crashes will not occur often
  CString attachmentFile = p_webroot + "/" + p_url;
  attachmentFile.Replace("/","\\");
  attachmentFile.Replace("\\\\","\\");

  EnsureFile ensure(attachmentFile);
  ensure.CheckCreateDirectory();

  attachmentFile += "HTTPServer_dump.xml";

  attachmentFile = ErrorReportWriteToFile(attachmentFile,attach,p_webroot,p_url);
  if(attachmentFile.IsEmpty())
  {
    return;
  }

  // Ask to send the message
  CString melding = p_subject + " in " + errorAddress;
  CString afzender(m_receiver);
  bool    zend(m_sendReport);
  if(ContinueSending(melding,message,afzender,zend) == false)
  {
    return;
  }


  CString extra("FOUTEN:nee\n"
                "DIALOOG:nee\n"
                "VOORTGANG:nee\n");

  // Adding the attachment to the message
  message = "ATTACH:" + attachmentFile + "\n" + extra + message;

  // Writing the message
  CString messageFile = ErrorReportWriteToFile("",message,p_webroot,p_url);
  if(messageFile.IsEmpty())
  {
    return;
  }

  // Init infobuffers for CreateProcess
  STARTUPINFO         startupInfo;
  PROCESS_INFORMATION processInfo;

  ZeroMemory(&processInfo, sizeof(processInfo));
  ZeroMemory(&startupInfo, sizeof(startupInfo));
  startupInfo.cb = sizeof(startupInfo);
  startupInfo.dwFlags = STARTF_USESHOWWINDOW;
  startupInfo.wShowWindow = (WORD) SW_SHOW;

  // Sending the message
  CString program = WebConfig::GetExePath() + "PostMail5.exe";
  char cmdLine[1024];
  sprintf_s(cmdLine,1024,"\"%s\" /S \"%s\"",(LPCTSTR) program,(LPCTSTR) messageFile);

  BOOL ok = CreateProcess(program,
                          cmdLine,
                          NULL,
                          NULL,
                          FALSE,
                          NORMAL_PRIORITY_CLASS,
                          NULL,
                          NULL,
                          &startupInfo,
                          &processInfo);

  // Sluit de proces info handles
  if(ok)
  {
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
  }
}

// Doet hier niets. Is bedoeld voor afgeleide klassen
bool 
ErrorReportPostMail::ContinueSending(CString& p_subject
                                    ,CString& p_melding
                                    ,CString& p_receiver
                                    ,bool&    p_cansend) const
{
  return true;
}