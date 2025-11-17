/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLDataType.cpp
//
// BaseLibrary: Indispensable general objects and functions
// 
// Created: 2014-2025 ir. W.E. Huisman
// MIT License
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

LPCTSTR xml_datatypes[] =
{
  _T("")                          // 0
 ,_T("string")                    // 1  XDT_String            SQL_C_TCHAR
 ,_T("integer")                   // 2  XDT_Integer           SQL_C_LONG / SQL_C_SLONG
 ,_T("boolean")                   // 3  XDT_Boolean           SQL_C_BIT
 ,_T("double")                    // 4  XDT_Double            SQL_C_DOUBLE
 ,_T("base64Binary")              // 5  XDT_Base64Binary
 ,_T("dateTime")                  // 6  XDT_DateTime          SQL_C_TIMESTAMP
 ,_T("anyURI")                    // 7  XDT_AnyURI
 ,_T("date")                      // 8  XDT_Date              SQL_C_DATE
 ,_T("dateTimeStamp")             // 9  XDT_DateTimeStamp     SQL_C_TIMESTAMP + timezone
 ,_T("decimal")                   // 10 XDT_Decimal           SQL_C_NUMERIC
 ,_T("long")                      // 11 XDT_Long              SQL_C_LONG  / SQL_C_SLONG
 ,_T("int")                       // 12 XDT_Int               SQL_C_SLONG
 ,_T("short")                     // 13 XDT_Short             SQL_C_SHORT / SQL_C_SSHORT
 ,_T("byte")                      // 14 XDT_Byte              SQL_C_TINYINT / SQL_C_STINYINT
 ,_T("nonNegativeInteger")        // 15 XDT_NonNegativeInteger
 ,_T("positiveInteger")           // 16 XDT_PositiveInteger
 ,_T("unsignedLong")              // 17 XDT_UnsignedLong      SQL_C_ULONG
 ,_T("unsignedInt")               // 18 XDT_UnsignedInt       SQL_C_ULONG
 ,_T("unsignedShort")             // 19 XDT_UnsignedShort     SQL_C_USHORT
 ,_T("unsignedByte")              // 20 XDT_UnsignedByte      SQL_C_UTINYINT
 ,_T("nonPositiveInteger")        // 21 XDT_NonPositiveInteger
 ,_T("negativeInteger")           // 22 XDT_NegativeInteger
 ,_T("duration")                  // 23 XDT_Duration          SQL_C_INTERVAL_*
 ,_T("dayTimeDuration")           // 24 XDT_DayTimeDuration   SQL_C_INTERVAL_DAY_SECOND
 ,_T("yearMonthDuration")         // 25 XDT_YearMonthDuration SQL_C_INTERVAL_YEAR_MONTH
 ,_T("float")                     // 26 XDT_Float             SQL_C_FLOAT / SQL_C_REAL
 ,_T("gDay")                      // 27 XDT_GregDay
 ,_T("gMonth")                    // 28 XDT_GregMonth
 ,_T("gMonthDay")                 // 29 XDT_GregMonthDay
 ,_T("gYear")                     // 30 XDT_GregYear
 ,_T("gYearMonth")                // 31 XDT_GregYearMonth
 ,_T("hexBinary")                 // 32 XDT_HeXBinary
 ,_T("NOTATION")                  // 33 XDT_NOTATION
 ,_T("QName")                     // 34 XDT_QName
 ,_T("normalizedString")          // 35 XDT_NormalizedString
 ,_T("token")                     // 36 XDT_Token
 ,_T("language")                  // 37 XDT_Language
 ,_T("Name")                      // 38 XDT_Name
 ,_T("NCName")                    // 39 XDT_NCName
 ,_T("ENTITY")                    // 40 XDT_ENTITY
 ,_T("ID")                        // 41 XDT_ID
 ,_T("IDREF")                     // 42 XDT_IDREF
 ,_T("NMTOKEN")                   // 43 XDT_NMTOKEN
 ,_T("time")                      // 44 XDT_Time
 ,_T("ENTITIES")                  // 45 XDT_ENTITIES
 ,_T("IDREFS")                    // 46 XDT_IDREFS
 ,_T("NMTOKENS")                  // 47 XDT_NMTOKENS
};

XString
XmlDataTypeToString(XmlDataType p_type)
{
  // See if XmlDataType is withing valid range
  if((p_type == XmlDataType::XDT_Unknown) || ((int)p_type > (int)XmlDataType::XDT_NMTOKENS))
  {
    return _T("");
  }
  return XString(xml_datatypes[(int)p_type]);
}

XmlDataType
StringToXmlDataType(const XString& p_name)
{
  const TCHAR** datatypes = xml_datatypes;

  for(int ind = 1;ind < sizeof(xml_datatypes) / sizeof(TCHAR*); ++ind)
  {
    if(p_name.Compare(datatypes[ind]) == 0)
    {
      return (XmlDataType)ind;
    }
  }
  return XmlDataType::XDT_Unknown;
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
  { XmlDataType::XDT_String,           SQL_C_TCHAR     }
 ,{ XmlDataType::XDT_Integer,          SQL_C_LONG      }
 ,{ XmlDataType::XDT_Integer,          SQL_C_SLONG     }
 ,{ XmlDataType::XDT_Boolean,          SQL_C_BIT       }
 ,{ XmlDataType::XDT_Double,           SQL_C_DOUBLE    }
 ,{ XmlDataType::XDT_DateTime,         SQL_C_TIMESTAMP }
 ,{ XmlDataType::XDT_Date,             SQL_C_DATE      }
 ,{ XmlDataType::XDT_Decimal,          SQL_C_NUMERIC   }
 ,{ XmlDataType::XDT_Long,             SQL_C_LONG      }
 ,{ XmlDataType::XDT_Long,             SQL_C_SLONG     }
 ,{ XmlDataType::XDT_Int,              SQL_C_SLONG     }
 ,{ XmlDataType::XDT_Short,            SQL_C_SHORT     }
 ,{ XmlDataType::XDT_Short,            SQL_C_SSHORT    }
 ,{ XmlDataType::XDT_Byte,             SQL_C_TINYINT   }
 ,{ XmlDataType::XDT_Byte,             SQL_C_STINYINT  }
 ,{ XmlDataType::XDT_UnsignedLong,     SQL_C_ULONG     }
 ,{ XmlDataType::XDT_UnsignedInt,      SQL_C_ULONG     }
 ,{ XmlDataType::XDT_UnsignedShort,    SQL_C_USHORT    }
 ,{ XmlDataType::XDT_UnsignedByte,     SQL_C_UTINYINT  }
 ,{ XmlDataType::XDT_DayTimeDuration,  SQL_C_INTERVAL_DAY_TO_SECOND }
 ,{ XmlDataType::XDT_YearMonthDuration,SQL_C_INTERVAL_YEAR_TO_MONTH }
 ,{ XmlDataType::XDT_Float,            SQL_C_FLOAT     }
};

int
XmlDataTypeToODBC(XmlDataType p_type)
{
  // See if XmlDataType is withing valid range
  if((p_type == XmlDataType::XDT_Unknown) || ((int)p_type > (int)XmlDataType::XDT_NMTOKENS))
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
  return XmlDataType::XDT_Unknown;
}
