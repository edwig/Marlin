#pragma once
#include "ServerApp.h"

class MarlinServerApp : public ServerApp
{
public:
  MarlinServerApp();
 ~MarlinServerApp();

  virtual void InitInstance();
  virtual void ExitInstance();

  void IncrementError() { ++m_errors; };

private:
  bool CorrectStarted();
  void ReportAfterTesting();

  int  m_errors    { 0     };
  bool m_doDetails { false };
  bool m_running   { false };
};

