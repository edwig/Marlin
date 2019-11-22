#pragma once

// Load product and application constants
void LoadConstants();

class MarlinServer
{
public:
  MarlinServer();
 ~MarlinServer();

  virtual bool Startup();
  virtual void ShutDown();

private:
};

// The one and only server instance!
// Can be called from within ServerMain for standalone and NT-Service 
// Can be called from within ServerApp  for IIS factored applications
extern MarlinServer* s_theServer;

