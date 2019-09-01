#pragma once

#define APPLICATIE_NAAM "DocumentServer.exe"

// Prototypes of static server functions

void WINAPI SvcMain( DWORD, LPTSTR * ); 
void WINAPI SvcCtrlHandler( DWORD ); 

void        ReadConfig();
int         SvcInstall(char* username,char* password);
int         SvcDelete();
void        SvcInit( DWORD, LPTSTR * ); 
void        SvcReportSuccessEvent(LPCTSTR p_message);
void        SvcReportErrorEvent(bool p_doFormat,LPCTSTR p_function,LPCTSTR p_message,...);
void        SvcReportInfoEvent (bool p_doFormat,LPCTSTR p_message,...);
void        ReportSvcStatus( DWORD, DWORD, DWORD );
int         InstallMessageDLL();
int         SvcStart();
int         SvcStop();
void        DeleteEventLogRegistration();
BOOL        StopDependentServices();
void        PrintCopyright();
void        PrintHelp();
void        CheckPlatform();
int         QueryService();

// Alles nog een keer, maar dan voor stand-alone

int         StandAloneStart();
int         StandAloneStop();
BOOL        SvcInitStandAlone();
int         GetBriefServiceStatusStandAlone(int& p_status);
void        ReportSvcStatusStandAlone(int p_status);
int         QueryServiceStandAlone();

// En nog een keer, maar nu voor IIS

int         StartIISApp();
int         StopIISApp();
int         QueryIISApp();
bool        FindApplicationCommand();
