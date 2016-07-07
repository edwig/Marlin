#include "stdafx.h"
#include "ServerApp.h"

// The one and only server object
// Create one for your own derived class
// <My>ServerApp theServer;

//XTOR
ServerApp::ServerApp()
{
}

// DTOR
ServerApp::~ServerApp()
{
  ExitInstance();
}

void 
ServerApp::ConnectServerApp(IHttpServer*   p_iis
                           ,HTTPServerIIS* p_server
                           ,ThreadPool*    p_pool
                           ,LogAnalysis*   p_logfile
                           ,ErrorReport*   p_report)
{
  // Remember our registration
  m_iis        = p_iis;
  m_appServer  = p_server;
  m_appPool    = p_pool;
  m_appLogfile = p_logfile;
  m_appReport  = p_report;

  // Init the server application
  InitInstance();
}

void 
ServerApp::InitInstance()
{
  // Create and start your sites
}

void 
ServerApp::ExitInstance()
{
  // Stop your sites
}
