#include "stdafx.h"
#include "MarlinServer.h"
#include <assert.h>

// The one-and-only Marlin server
MarlinServer* s_theServer = nullptr;

MarlinServer::MarlinServer()
{
  assert(s_theServer == nullptr);
  s_theServer = this;
}

MarlinServer::~MarlinServer()
{
  assert(s_theServer == this);
  s_theServer = nullptr;
}

// Should not come to here!
// You should derive a class from "MarlinServer" and do your thing there!
//
bool MarlinServer::Startup()
{
  assert(s_theServer != nullptr);
  return false;
}

void MarlinServer::ShutDown()
{
}