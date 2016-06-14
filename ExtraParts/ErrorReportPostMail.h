/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ErrorReportPostMail.h
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
#pragma once
#include "ErrorReport.h"

//////////////////////////////////////////////////////////////////////////
//
// Sending our error report by PostMail
//
//////////////////////////////////////////////////////////////////////////

class ErrorReportPostMail : public ErrorReport
{
public:
  void SetSending   (bool  p_send);
  void SetMailServer(const CString& p_mailServer);
  void SetSender    (const CString& p_sender);
  void SetReceiver  (const CString& p_receiver);

protected:
  virtual void DoReport(const CString&     p_subject
                       ,const StackTrace&  p_trace
                       ,const CString&     p_webroot
                       ,const CString&     p_url) const;

  virtual bool ContinueSending(CString& p_subject
                              ,CString& p_melding
                              ,CString& p_receiver
                              ,bool&    p_cansend) const;

  // Data
  bool    m_sendReport { false }; // Should we send the error report?
  CString m_mailServer;           // SMTP email server
  CString m_sender;               // Sender of the error report
  CString m_receiver;             // Receiver of the error report
};

inline void
ErrorReportPostMail::SetMailServer(const CString& p_mailServer)
{
  m_mailServer = p_mailServer;
}

inline void
ErrorReportPostMail::SetSender(const CString& p_sender)
{
  m_sender = p_sender;
}

inline void
ErrorReportPostMail::SetReceiver(const CString& p_receiver)
{
  m_receiver = p_receiver;
}

inline void
ErrorReportPostMail::SetSending(bool p_send)
{
  m_sendReport = p_send;
}

