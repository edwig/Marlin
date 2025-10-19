//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 - 2024 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "http_private.h"
#include "HTTPReadRegister.h"

// Read one value from the registry
// Special function to read HTTP parameters
bool
HTTPReadRegister(const XString& p_sectie
                ,const XString& p_key
                ,DWORD    p_type
                ,XString& p_value1
                ,PDWORD   p_value2
                ,PTCHAR   p_value3,PDWORD p_size3)
{
  HKEY    hkUserURL;
  bool    result = false;
  XString key = _T("SYSTEM\\ControlSet001\\Services\\HTTP\\Parameters\\") + p_sectie;

  DWORD dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE
                            ,(LPCTSTR) key
                            ,0
                            ,KEY_QUERY_VALUE
                            ,&hkUserURL);
  if(dwErr == ERROR_SUCCESS)
  {
    DWORD dwIndex = 0;
    DWORD dwType  = 0;
    TCHAR buffName[BUFF_LEN];
    BYTE  buffData[BUFF_LEN];

    //enumerate this key's values
    while(ERROR_SUCCESS == dwErr)
    {
      DWORD dwNameSize = BUFF_LEN;
      DWORD dwDataSize = BUFF_LEN;
      dwErr = RegEnumValue(hkUserURL
                          ,dwIndex++
                          ,buffName
                          ,&dwNameSize
                          ,NULL
                          ,&dwType
                          ,buffData
                          ,&dwDataSize);
      if(dwErr == ERROR_SUCCESS && dwType == REG_SZ && p_type == dwType)
      {
        if(p_key.CompareNoCase(buffName) == 0)
        {
          buffData[dwDataSize] = 0;
          p_value1 = (LPCTSTR)buffData;
          result   = true;
          break;
        }
      }
      if(dwErr == ERROR_SUCCESS && dwType == REG_DWORD && p_type == dwType)
      {
        if(p_key.CompareNoCase(buffName) == 0)
        {
          *p_value2 = *((PDWORD)buffData);
          result    = true;
          break;
        }
      }
      if(dwErr == ERROR_SUCCESS && dwType == REG_BINARY && p_type == dwType)
      {
        if(p_key.CompareNoCase(buffName) == 0)
        {
          if(*p_size3 >= dwDataSize)
          {
            memcpy_s(p_value3,*p_size3,buffData,dwDataSize);
            *p_size3 = dwDataSize;
            result   = true;
            break;
          }
        }
      }
    }
    RegCloseKey(hkUserURL);
  }
  return result;
}
