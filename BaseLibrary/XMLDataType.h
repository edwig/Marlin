/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLDataType.h
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
#pragma once

// XML DATA TYPES

// The XML Datatype holds datatypes AND wsdl options
enum class XmlDataType
{
  XDT_Unknown             = 0x00000000
 ,XDT_String              = 0x00000001
 ,XDT_StringCDATA         = 0x00002001
 ,XDT_Integer             = 0x00000002
 ,XDT_Boolean             = 0x00000003
 ,XDT_Double              = 0x00000004
 ,XDT_Base64Binary        = 0x00000005
 ,XDT_DateTime            = 0x00000006
  // New in version 4
 ,XDT_AnyURI              = 0x00000007
 ,XDT_Date                = 0x00000008
 ,XDT_DateTimeStamp       = 0x00000009
 ,XDT_Decimal             = 0x0000000A
 ,XDT_Long                = 0x0000000B
 ,XDT_Int                 = 0x0000000C
 ,XDT_Short               = 0x0000000D
 ,XDT_Byte                = 0x0000000E
 ,XDT_NonNegativeInteger  = 0x0000000F
 ,XDT_PositiveInteger     = 0x00000010
 ,XDT_UnsignedLong        = 0x00000011
 ,XDT_UnsignedInt         = 0x00000012
 ,XDT_UnsignedShort       = 0x00000013
 ,XDT_UnsignedByte        = 0x00000014
 ,XDT_NonPositiveInteger  = 0x00000015
 ,XDT_NegativeInteger     = 0x00000016
 ,XDT_Duration            = 0x00000017
 ,XDT_DayTimeDuration     = 0x00000018
 ,XDT_YearMonthDuration   = 0x00000019
 ,XDT_Float               = 0x0000001A
 ,XDT_GregDay             = 0x0000001B
 ,XDT_GregMonth           = 0x0000001C
 ,XDT_GregMonthDay        = 0x0000001D
 ,XDT_GregYear            = 0x0000001E
 ,XDT_GregYearMonth       = 0x0000001F
 ,XDT_HexBinary           = 0x00000020
 ,XDT_NOTATION            = 0x00000021
 ,XDT_QName               = 0x00000022
 ,XDT_NormalizedString    = 0x00000023
 ,XDT_Token               = 0x00000024
 ,XDT_Language            = 0x00000025
 ,XDT_Name                = 0x00000026
 ,XDT_NCName              = 0x00000027
 ,XDT_ENTITY              = 0x00000028
 ,XDT_ID                  = 0x00000029
 ,XDT_IDREF               = 0x0000002A
 ,XDT_NMTOKEN             = 0x0000002B
 ,XDT_Time                = 0x0000002C
 ,XDT_ENTITIES            = 0x0000002D
 ,XDT_IDREFS              = 0x0000002E
 ,XDT_NMTOKENS            = 0x0000002F
  // End - new in version 4 and 5
 ,XDT_Type                = 0x00001000 // Show type in XML always
 ,XDT_CDATA               = 0x00002000
 ,XDT_Array               = 0x00004000
 ,XDT_Complex             = 0x00008000
  // WSDL Options (USE ONLY ONE (1) !!)
 ,WSDL_Mandatory          = 0x00010000
 ,WSDL_Optional           = 0x00020000
 ,WSDL_ZeroOne            = 0x00040000
 ,WSDL_OnceOnly           = 0x00080000
 ,WSDL_ZeroMany           = 0x00100000
 ,WSDL_OneMany            = 0x00200000
 ,WSDL_Choice             = 0x00400000 // Choice
 ,WSDL_Sequence           = 0x00800000 // Exact sequence
};

#define XDT_Mask            0x0000ffff
#define XDT_MaskTypes       0x00000fff

#define WSDL_Mask           0x00ff0000
#define WSDL_MaskOrder      0x00C00000
#define WSDL_MaskField      0x003F0000

// Combining to XmlDataTypes

#define XDT_Combine(a,b)   (XmlDataType)((static_cast<int>(a)) | (static_cast<int>(b)))

// Conversion between XML datatype and string names
XString     XmlDataTypeToString(XmlDataType p_type);
XmlDataType StringToXmlDataType(const XString& p_name);

// Conversion between XML datatype and ODBC types
int         XmlDataTypeToODBC(XmlDataType p_type);
XmlDataType ODBCToXmlDataType(int p_type);

