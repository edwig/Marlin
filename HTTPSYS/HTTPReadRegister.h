#pragma once

#define BUFF_LEN 1024

// Read one value from the registry
// Special function to read HTTP parameters
bool
HTTPReadRegister(CString  p_sectie,CString p_key,DWORD p_type
                ,CString& p_value1
                ,PDWORD   p_value2
                ,PTCHAR   p_value3,PDWORD p_size3);
