#pragma once

// Define in the derived class from "MarlinServer"
extern const char* APPLICATION_NAME;       // Name of the application EXE file!!
extern const char* PRODUCT_NAME;           // Short name of the product (one word only)
extern const char* PRODUCT_DISPLAY_NAME;   // "Service for PRODUCT_NAME: <description of the service>"
extern const char* PRODUCT_COPYRIGHT;      // Copyright line of the product (c) <year> etc.
extern const char* PRODUCT_VERSION;        // Short version string (e.g.: "3.2.0") Release.major.minor ONLY!
extern const char* PRODUCT_MESSAGES_DLL;   // Filename of the WMI Messages dll.
extern const char* PRODUCT_SITE;           // Standard base URL absolute path e.g. "/MarlinServer/"

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

