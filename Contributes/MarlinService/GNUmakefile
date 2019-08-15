# 
# MIT License
# 
# Copyright (c) 2019 Yuguo Zhang 
# 
#
# GNUmakefile for MarlinWebServer
#
# to build this project,
# you need GNU make and 
# visual studio 2019 command line build enviroment.
#
DEBUG ?= 1
WIN64 ?= 1

# Microsoft tools
RD = rd /s /q
RM = del
CC = cl.exe
LD = link.exe
RC = rc.exe
MT = mt.exe

# where is Marlin's source root 
MARLIN_DIR = ../..
SOURCE_DIR = src

#
OUTDIR = bin
PROJECT_NAME = MarlinWebServer


vpath %.cpp $(SOURCE_DIR)
vpath %.cpp $(MARLIN_DIR)/Marlin
vpath %.cpp $(MARLIN_DIR)/Marlin/ZLib
vpath %.c $(MARLIN_DIR)/Marlin/ZLib

CXXBASEFLAGS = /nologo /std:c++17 -D_WINDOWS -D_WIN32 -DWIN32 -DMARLIN_USE_ATL_ONLY /EHa /Zc:wchar_t -I $(MARLIN_DIR)/ExtraParts

WINLIBS = httpapi.lib winhttp.lib crypt32.lib Rpcrt4.lib Secur32.lib ws2_32.lib

MARLIN_LIB = $(OUTDIR)\Marlin.lib
COMPONENT  = $(OUTDIR)\$(PROJECT_NAME).exe

INTDIR        = objs_server
MARLIN_INTDIR = objs_marlin

ifdef DEBUG
CXXFLAGS        = -D_DEBUG /MTd /W4 /Zi /Fd$(INTDIR)/ $(CXXBASEFLAGS) -I $(MARLIN_DIR)/Marlin -DMARLIN_NOAUTOLINK
MARLIN_CXX_FLAGS= -D_DEBUG /MTd /W4 /Zi /Fd$(MARLIN_INTDIR)/ $(CXXBASEFLAGS) -I $(MARLIN_DIR)/Marlin/ZLib
LDFLAGS = /nologo /DEBUG /SUBSYSTEM:WINDOWS
else
CXXFLAGS        = -DNDEBUG /MT /WX /O2 /Ob1 /GF /Gy $(CXXBASEFLAGS) -I $(MARLIN_DIR)/Marlin -DMARLIN_NOAUTOLINK
MARLIN_CXX_FLAGS= -DNDEBUG /MT /WX /O2 /Ob1 /GF /Gy $(CXXBASEFLAGS) -I $(MARLIN_DIR)/Marlin/ZLib
LDFLAGS = /nologo /SUBSYSTEM:WINDOWS
endif

ALL:	$(OUTDIR) $(MARLIN_LIB) $(COMPONENT)

$(INTDIR):
	@if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

$(MARLIN_INTDIR):
	@if not exist "$(MARLIN_INTDIR)/$(NULL)" mkdir "$(MARLIN_INTDIR)"

$(OUTDIR):
	@if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

clean:
	@if exist "$(INTDIR)/$(NULL)" $(RD) $(INTDIR)
	@if exist "$(COMPONENT)" $(RM) $(basename $(COMPONENT)).*

cleanall:clean
	@if exist "$(MARLIN_INTDIR)/$(NULL)" $(RD) $(MARLIN_INTDIR)
	@if exist "$(MARLIN_LIB)" $(RM) $(basename $(MARLIN_LIB)).*

MARLIN_SERVICE_SOURCES = $(SOURCE_DIR)/utils.cpp \
                         $(SOURCE_DIR)/MarlinService.cpp \
                         $(SOURCE_DIR)/WinMain.cpp

#not used Marlin sources
#CommandBus.cpp ThreadPoolED.cpp
MARLIN_SOURCES = $(MARLIN_DIR)/Marlin/Analysis.cpp \
  $(MARLIN_DIR)/Marlin/Base64.cpp \
  $(MARLIN_DIR)/Marlin/CommandBus.cpp \
  $(MARLIN_DIR)/Marlin/ConvertWideString.cpp \
  $(MARLIN_DIR)/Marlin/Cookie.cpp \
  $(MARLIN_DIR)/Marlin/CrackURL.cpp \
  $(MARLIN_DIR)/Marlin/CreateURLPrefix.cpp \
  $(MARLIN_DIR)/Marlin/Crypto.cpp \
  $(MARLIN_DIR)/Marlin/DefuseBOM.cpp \
  $(MARLIN_DIR)/Marlin/EnsureFile.cpp \
  $(MARLIN_DIR)/Marlin/ErrorReport.cpp \
  $(MARLIN_DIR)/Marlin/EventSource.cpp \
  $(MARLIN_DIR)/Marlin/FileBuffer.cpp \
  $(MARLIN_DIR)/Marlin/FindProxy.cpp \
  $(MARLIN_DIR)/Marlin/GenerateGUID.cpp \
  $(MARLIN_DIR)/Marlin/GetLastErrorAsString.cpp \
  $(MARLIN_DIR)/Marlin/GetUserAccount.cpp \
  $(MARLIN_DIR)/Marlin/HostedWebCore.cpp \
  $(MARLIN_DIR)/Marlin/HPFCounter.cpp \
  $(MARLIN_DIR)/Marlin/HTTPCertificate.cpp \
  $(MARLIN_DIR)/Marlin/HTTPClient.cpp \
  $(MARLIN_DIR)/Marlin/HTTPClientTracing.cpp \
  $(MARLIN_DIR)/Marlin/HTTPError.cpp \
  $(MARLIN_DIR)/Marlin/HTTPMessage.cpp \
  $(MARLIN_DIR)/Marlin/HTTPRequest.cpp \
  $(MARLIN_DIR)/Marlin/HTTPServer.cpp \
  $(MARLIN_DIR)/Marlin/HTTPServerIIS.cpp \
  $(MARLIN_DIR)/Marlin/HTTPServerMarlin.cpp \
  $(MARLIN_DIR)/Marlin/HTTPServerSync.cpp \
  $(MARLIN_DIR)/Marlin/HTTPSite.cpp \
  $(MARLIN_DIR)/Marlin/HTTPSiteIIS.cpp \
  $(MARLIN_DIR)/Marlin/HTTPSiteMarlin.cpp \
  $(MARLIN_DIR)/Marlin/HTTPTime.cpp \
  $(MARLIN_DIR)/Marlin/HTTPURLGroup.cpp \
  $(MARLIN_DIR)/Marlin/HTTPWebSocket.cpp \
  $(MARLIN_DIR)/Marlin/IISDebug.cpp \
  $(MARLIN_DIR)/Marlin/JSONMessage.cpp \
  $(MARLIN_DIR)/Marlin/JSONParser.cpp \
  $(MARLIN_DIR)/Marlin/MediaType.cpp \
  $(MARLIN_DIR)/Marlin/MultiPartBuffer.cpp \
  $(MARLIN_DIR)/Marlin/Namespace.cpp \
  $(MARLIN_DIR)/Marlin/PrintToken.cpp \
  $(MARLIN_DIR)/Marlin/ProcInfo.cpp \
  $(MARLIN_DIR)/Marlin/Redirect.cpp \
  $(MARLIN_DIR)/Marlin/RunRedirect.cpp \
  $(MARLIN_DIR)/Marlin/ServerApp.cpp \
  $(MARLIN_DIR)/Marlin/SiteFilter.cpp \
  $(MARLIN_DIR)/Marlin/SiteFilterClientCertificate.cpp \
  $(MARLIN_DIR)/Marlin/SiteFilterXSS.cpp \
  $(MARLIN_DIR)/Marlin/SiteHandler.cpp \
  $(MARLIN_DIR)/Marlin/SiteHandlerConnect.cpp \
  $(MARLIN_DIR)/Marlin/SiteHandlerDelete.cpp \
  $(MARLIN_DIR)/Marlin/SiteHandlerFormData.cpp \
  $(MARLIN_DIR)/Marlin/SiteHandlerGet.cpp \
  $(MARLIN_DIR)/Marlin/SiteHandlerHead.cpp \
  $(MARLIN_DIR)/Marlin/SiteHandlerJson.cpp \
  $(MARLIN_DIR)/Marlin/SiteHandlerJson2Soap.cpp \
  $(MARLIN_DIR)/Marlin/SiteHandlerMerge.cpp \
  $(MARLIN_DIR)/Marlin/SiteHandlerOptions.cpp \
  $(MARLIN_DIR)/Marlin/SiteHandlerPatch.cpp \
  $(MARLIN_DIR)/Marlin/SiteHandlerPost.cpp \
  $(MARLIN_DIR)/Marlin/SiteHandlerPut.cpp \
  $(MARLIN_DIR)/Marlin/SiteHandlerSoap.cpp \
  $(MARLIN_DIR)/Marlin/SiteHandlerTrace.cpp \
  $(MARLIN_DIR)/Marlin/SiteHandlerWebDAV.cpp \
  $(MARLIN_DIR)/Marlin/SiteHandlerWebSocket.cpp \
  $(MARLIN_DIR)/Marlin/SOAPMessage.cpp \
  $(MARLIN_DIR)/Marlin/SOAPSecurity.cpp \
  $(MARLIN_DIR)/Marlin/StackTrace.cpp \
  $(MARLIN_DIR)/Marlin/StdException.cpp \
  $(MARLIN_DIR)/Marlin/ThreadPool.cpp \
  $(MARLIN_DIR)/Marlin/WebConfig.cpp \
  $(MARLIN_DIR)/Marlin/WebConfigIIS.cpp \
  $(MARLIN_DIR)/Marlin/WebServiceClient.cpp \
  $(MARLIN_DIR)/Marlin/WebServiceServer.cpp \
  $(MARLIN_DIR)/Marlin/WebSocket.cpp \
  $(MARLIN_DIR)/Marlin/WebSocketClient.cpp \
  $(MARLIN_DIR)/Marlin/WebSocketServer.cpp \
  $(MARLIN_DIR)/Marlin/WebSocketServerIIS.cpp \
  $(MARLIN_DIR)/Marlin/WebSocketServerSync.cpp \
  $(MARLIN_DIR)/Marlin/WinSocket.cpp \
  $(MARLIN_DIR)/Marlin/WSDLCache.cpp \
  $(MARLIN_DIR)/Marlin/XMLDataType.cpp \
  $(MARLIN_DIR)/Marlin/XMLMessage.cpp \
  $(MARLIN_DIR)/Marlin/XMLParser.cpp \
  $(MARLIN_DIR)/Marlin/XMLParserImport.cpp \
  $(MARLIN_DIR)/Marlin/XMLRestriction.cpp \
  $(MARLIN_DIR)/Marlin/XMLTemporal.cpp

GZIP_SOURCES = $(MARLIN_DIR)/Marlin/Zlib/gzip.cpp

# obj files
SERVICE_OBJS = $(addprefix $(INTDIR)/, $(addsuffix .obj,$(basename $(notdir $(MARLIN_SERVICE_SOURCES)))))

MARLIN_OBJS = $(addprefix $(MARLIN_INTDIR)/, $(addsuffix .obj,$(basename $(notdir $(MARLIN_SOURCES)))))
GZIPOBJS = $(addprefix $(MARLIN_INTDIR)/, $(addsuffix .obj,$(basename $(notdir $(GZIP_SOURCES)))))
ZLIBOBJS = $(addprefix $(MARLIN_INTDIR)/, $(addsuffix .obj,$(basename $(notdir $(wildcard $(MARLIN_DIR)/Marlin/ZLib/*.c)))))

# artifacts
$(MARLIN_LIB): $(MARLIN_INTDIR) $(MARLIN_INTDIR)/stdafx.obj $(MARLIN_OBJS) $(ZLIBOBJS) $(GZIPOBJS)
	@echo creating Marlin static library
	@LIB /nologo /out:$@ $(MARLIN_INTDIR)/stdafx.obj $(MARLIN_OBJS) $(ZLIBOBJS) $(GZIPOBJS)

$(COMPONENT): $(INTDIR) $(INTDIR)/pch.obj $(SERVICE_OBJS) $(TEST_OBJS)
	@echo linking $@ ...
	@$(LD) $(LDFLAGS) /out:$@ $(INTDIR)/pch.obj $(SERVICE_OBJS) $(TEST_OBJS) $(WINLIBS) $(MARLIN_LIB)
	@if exist $(COMPONENT).manifest $(MT) -nologo -manifest $(COMPONENT).manifest -outputresource:$(COMPONENT);2
	@if exist $(COMPONENT).manifest $(RM) $(COMPONENT).manifest

# rules and actions
$(INTDIR)/%.obj:%.cpp
	@$(CC) $(CXXFLAGS) -c -Fo$@ /Yu"pch.h" /Fp$(INTDIR)/$(PROJECT_NAME).pch $<
$(INTDIR)/%.res:%.rc
	@echo compiling resources
	@$(RC) /nologo /l 0x804 /fo"$@" $<
$(INTDIR)/pch.obj:pch.cpp
	@$(CC) $(CXXFLAGS) -c /Yc"pch.h" /Fp$(INTDIR)/$(PROJECT_NAME).pch /Fo$@ $<
$(INTDIR)/$(PROJECT_NAME).res:$(PROJECT_NAME).rc
$(MARLIN_INTDIR)/stdafx.obj:stdafx.cpp
	@$(CC) $(MARLIN_CXX_FLAGS) -c /Yc"stdafx.h" /Fp$(MARLIN_INTDIR)/Marlin.pch /Fo$@ $<
$(MARLIN_INTDIR)/gzip.obj:gzip.cpp
	@$(CC) $(MARLIN_CXX_FLAGS) -c /wd5033 /wd4457 -Fo$@ $<
$(MARLIN_INTDIR)/%.obj:%.cpp
	@$(CC) $(MARLIN_CXX_FLAGS) -c -Fo$@ /Yu"stdafx.h" /Fp$(MARLIN_INTDIR)/Marlin.pch $<
$(MARLIN_INTDIR)/%.obj:%.c
	@$(CC) $(MARLIN_CXX_FLAGS) -c -Fo$@ $<
