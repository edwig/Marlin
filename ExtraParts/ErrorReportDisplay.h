/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ErrorReportDisplay.h
//

#pragma once
#include "ErrorReportPostMail.h"

//////////////////////////////////////////////////////////////////////////
//
// Sending our error report by PostMail
//
//////////////////////////////////////////////////////////////////////////

class ErrorReportDisplay : public ErrorReportPostMail
{
protected:
  virtual bool ContinueSending(CString& p_subject
                              ,CString& p_melding
                              ,CString& p_receiver
                              ,bool&    p_cansend) const;
};
