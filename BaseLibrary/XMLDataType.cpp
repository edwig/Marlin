/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLDataType.cpp
//
// BaseLibrary: Indispensable general objects and functions
// 
// Copyright (c) 2014-2022 ir. W.E. Huisman
// All rights reserved
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include "pch.h"
#include "XMLDataType.h"
#include <sql.h>
#include <sqlext.h>

const char* xml_datatypes[] =
{
  ""                          // 0
 ,"string"                    // 1  XDT_String            SQL_C_CHAR
 ,"integer"                   // 2  XDT_Integer           SQL_C_LONG / SQL_C_SLONG
 ,"boolean"                   // 3  XDT_Boolean           SQL_C_BIT
 ,"double"                    // 4  XDT_Double            SQL_C_DOUBLE
 ,"base64Binary"              // 5  XDT_Base64Binary
 ,"dateTime"                  // 6  XDT_DateTime          SQL_C_TIMESTAMP
 ,"anyURI"                    // 7  XDT_AnyURI
 ,"date"                      // 8  XDT_Date              SQL_C_DATE
 ,"dateTimeStamp"             // 9  XDT_DateTimeStamp     SQL_C_TIMESTAMP + timezone
 ,"decimal"                   // 10 XDT_Decimal           SQL_C_NUMERIC
 ,"long"                      // 11 XDT_Long              SQL_C_LONG  / SQL_C_SLONG
 ,"int"                       // 12 XDT_Int               SQL_C_SLONG
 ,"short"                     // 13 XDT_Short             SQL_C_SHORT / SQL_C_SSHORT
 ,"byte"                      // 14 XDT_Byte              SQL_C_TINYINT / SQL_C_STINYINT
 ,"nonNegativeInteger"        // 15 XDT_NonNegativeInteger
 ,"positiveInteger"           // 16 XDT_PositiveInteger
 ,"unsignedLong"              // 17 XDT_UnsignedLong      SQL_C_ULONG
 ,"unsignedInt"               // 18 XDT_UnsignedInt       SQL_C_ULONG
 ,"unsignedShort"             // 19 XDT_UnsignedShort     SQL_C_USHORT
 ,"unsignedByte"              // 20 XDT_UnsignedByte      SQL_C_UTINYINT
 ,"nonPositiveInteger"        // 21 XDT_NonPositiveInteger
 ,"negativeInteger"           // 22 XDT_NegativeInteger
 ,"duration"                  // 23 XDT_Duration          SQL_C_INTERVAL_*
 ,"dayTimeDuration"           // 24 XDT_DayTimeDuration   SQL_C_INTERVAL_DAY_SECOND
 ,"yearMonthDuration"         // 25 XDT_YearMonthDuration SQL_C_INTERVAL_YEAR_MONTH
 ,"float"                     // 26 XDT_Float             SQL_C_FLOAT / SQL_C_REAL
 ,"gDay"                      // 27 XDT_GregDay
 ,"gMonth"                    // 28 XDT_GregMonth
 ,"gMonthDay"                 // 29 XDT_GregMonthDay
 ,"gYear"                     // 30 XDT_GregYear
 ,"gYearMonth"                // 31 XDT_GregYearMonth
 ,"hexBinary"                 // 32 XDT_HeXBinary
 ,"NOTATION"                  // 33 XDT_NOTATION
 ,"QName"                     // 34 XDT_QName
 ,"normalizedString"          // 35 XDT_NormalizedString
 ,"token"                     // 36 XDT_Token
 ,"language"                  // 37 XDT_Language
 ,"Name"                      // 38 XDT_Name
 ,"NCName"                    // 39 XDT_NCName
 ,"ENTITY"                    // 40 XDT_ENTITY
 ,"ID"                        // 41 XDT_ID
 ,"IDREF"                     // 42 XDT_IDREF
 ,"NMTOKEN"                   // 43 XDT_NMTOKEN
 ,"time"                      // 44 XDT_Time
 ,"ENTITIES"                  // 45 XDT_ENTITIES
 ,"IDREFS"                    // 46 XDT_IDREFS
 ,"NMTOKENS"                  // 47 XDT_NMTOKENS
};

XString
XmlDataTypeToString(XmlDataType p_type)
{
  // See if XmlDataType is withing valid range
  if(p_type < XDT_String || p_type > XDT_NMTOKENS)
  {
    return "";
  }
  return XString(xml_datatypes[p_type]);
}

XmlDataType
StringToXmlDataType(XString p_name)
{
  const char** datatypes = xml_datatypes;

  for(int ind = 1;ind < sizeof(xml_datatypes) / sizeof(char*); ++ind)
  {
    if(p_name.Compare(datatypes[ind]) == 0)
    {
      return ind;
    }
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// CONVERSION BETWEEN XML datatypes AND ODBC datatypes
//
//////////////////////////////////////////////////////////////////////////

struct _xmlodbc
{
  XmlDataType m_xmlType;
  int         m_odbcType;
} 
xmlodbc_types [] =
{
  { XDT_String,           SQL_C_CHAR      }
 ,{ XDT_Integer,          SQL_C_LONG      }
 ,{ XDT_Integer,          SQL_C_SLONG     }
 ,{ XDT_Boolean,          SQL_C_BIT       }
 ,{ XDT_Double,           SQL_C_DOUBLE    }
 ,{ XDT_DateTime,         SQL_C_TIMESTAMP }
 ,{ XDT_Date,             SQL_C_DATE      }
 ,{ XDT_Decimal,          SQL_C_NUMERIC   }
 ,{ XDT_Long,             SQL_C_LONG      }
 ,{ XDT_Long,             SQL_C_SLONG     }
 ,{ XDT_Int,              SQL_C_SLONG     }
 ,{ XDT_Short,            SQL_C_SHORT     }
 ,{ XDT_Short,            SQL_C_SSHORT    }
 ,{ XDT_Byte,             SQL_C_TINYINT   }
 ,{ XDT_Byte,             SQL_C_STINYINT  }
 ,{ XDT_UnsignedLong,     SQL_C_ULONG     }
 ,{ XDT_UnsignedInt,      SQL_C_ULONG     }
 ,{ XDT_UnsignedShort,    SQL_C_USHORT    }
 ,{ XDT_UnsignedByte,     SQL_C_UTINYINT  }
 ,{ XDT_DayTimeDuration,  SQL_C_INTERVAL_DAY_TO_SECOND }
 ,{ XDT_YearMonthDuration,SQL_C_INTERVAL_YEAR_TO_MONTH }
 ,{ XDT_Float,            SQL_C_FLOAT     }
};

int
XmlDataTypeToODBC(XmlDataType p_type)
{
  // See if XmlDataType is withing valid range
  if(p_type < XDT_String || p_type > XDT_NMTOKENS)
  {
    return 0;
  }
  for(int ind = 0;ind < sizeof(xmlodbc_types)/sizeof(struct _xmlodbc);++ind)
  {
    if(xmlodbc_types[ind].m_xmlType == p_type)
    {
      return xmlodbc_types[ind].m_odbcType;
    }
  }
  return 0;
}

XmlDataType 
ODBCToXmlDataType(int p_type)
{
  for(int ind = 0;ind < sizeof(xmlodbc_types)/sizeof(struct _xmlodbc);++ind)
  {
    if(xmlodbc_types[ind].m_odbcType == p_type)
    {
      return xmlodbc_types[ind].m_xmlType;
    }
  }
  return 0;
}
