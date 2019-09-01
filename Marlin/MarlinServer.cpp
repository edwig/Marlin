#include "stdafx.h"
#include "MarlinServer.h"
#include <assert.h>

static MarlinServer* s_instance = nullptr;

MarlinServer::MarlinServer()
{
  assert(s_instance == nullptr);
  s_instance = this;
}

MarlinServer::~MarlinServer()
{
  assert(s_instance == this);
  s_instance = nullptr;
}

// Should not come to here!
// You should derive a class from "MarlinServer" and do your thing there!
//
bool MarlinServer::Startup()
{
  return false;
}

void MarlinServer::ShutDown()
{
}