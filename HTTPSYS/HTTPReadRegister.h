#pragma once

#define BUFF_LEN 1024

// Read one value from the registry
// Special function to read HTTP parameters
bool
HTTPReadRegister(const XString& p_sectie
                ,const XString& p_key
                ,DWORD    p_type
                ,XString& p_value1
                ,PDWORD   p_value2
                ,PTCHAR   p_value3,PDWORD p_size3);
