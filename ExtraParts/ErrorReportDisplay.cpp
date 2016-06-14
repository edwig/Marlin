/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ErrorReportDisplay.cpp
//
#include "StdAfx.h"
#include "ErrorReportDisplay.h"
#include "ErnstigDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool 
ErrorReportDisplay::ContinueSending(CString& p_subject
                                   ,CString& p_melding
                                   ,CString& p_receiver
                                   ,bool&    p_cansend) const
{
  // Ask to send the message
  ErnstigDlg dlg(NULL,p_subject,p_melding,p_receiver,p_cansend);
  return (dlg.DoModal() == IDOK);
}