/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ConvertWideString.cpp
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
#include "ConvertWideString.h"
#include <map>
#include <xstring>

typedef struct _cpNames
{
  int     m_codepage_ID;    // Active codepage ID
  XString m_codepage_Name;  // Codepage name as in HTTP protocol "charset"
  XString m_information;    // MSDN Documentary description
}
CodePageName;

static CodePageName cpNames[] =
{
  { 000,    "ACP",                "Default ANSI code page"                                                                  }
 ,{ 001,    "OEMCP",              "Default OEM code page"                                                                   }
 ,{ 002,    "MACCP",              "Default MAC code page"                                                                   }
 ,{ 003,    "THREAD",             "Current thread's ANSI code page"                                                         }
 ,{ 037,    "IBM037",             "IBM EBCDIC US-Canada"                                                                    }
 ,{ 042,    "SYMBOL",             "Symbol translations"                                                                     }
 ,{ 437,    "IBM437",             "OEM United States"                                                                       }
 ,{ 500,    "IBM500",             "IBM EBCDIC International"                                                                }
 ,{ 708,    "ASMO-708",           "Arabic (ASMO 708)"                                                                       }
 ,{ 709,    "",                   "Arabic (ASMO-449+, BCON V4)"                                                             }
 ,{ 710,    "",                   "Arabic - Transparent-Arabic"                                                             }
 ,{ 720,    "DOS-720",            "Arabic(Transparent ASMO); Arabic (DOS)"                                                  }
 ,{ 737,    "ibm737",             "OEM Greek (Formerly 437G); Greek (DOS)"                                                  }
 ,{ 775,    "ibm775",             "OEM Baltic; Baltic (DOS)"                                                                }
 ,{ 850,    "ibm850",             "OEM Multilingual Latin 1; Western European (DOS)"                                        }
 ,{ 852,    "imb852",             "OEM Latin 2; Central European (DOS)"                                                     }
 ,{ 855,    "ibm855",             "OEM Cyrillic (primarily Russian)"                                                        }
 ,{ 857,    "ibm857",             "OEM Turkish; Turkish (DOS)"                                                              }
 ,{ 858,    "IBM00858",           "OEM Multilingual Latin 1 + Euro symbol"                                                  }
 ,{ 860,    "IBM860",             "OEM Portuguese; Portuguese (DOS)"                                                        }
 ,{ 861,    "ibm861",             "OEM Icelandic; Icelandic (DOS)"                                                          }
 ,{ 862,    "DOS-862",            "OEM Hebrew; Hebrew (DOS)"                                                                }
 ,{ 863,    "IBM863",             "OEM French Canadian; French Canadian (DOS)"                                              }
 ,{ 864,    "IBM864",             "OEM Arabic; Arabic (864)"                                                                }
 ,{ 865,    "IBM865",             "OEM Nordic; Nordic (DOS)"                                                                }
 ,{ 866,    "cp866",              "OEM Russian; Cyrillic (DOS)"                                                             }
 ,{ 869,    "ibm869",             "OEM Modern Greek; Greek, Modern (DOS)"                                                   }
 ,{ 870,    "IBM870",             "IBM EBCDIC Multilingual/ROECE (Latin 2); IBM EBCDIC Multilingual Latin 2"                }
 ,{ 874,    "windows-874",        "ANSI/OEM Thai (same as 28605, ISO 8859-15); Thai (Windows)"                              }
 ,{ 875,    "cp875",              "IBM EBCDIC Greek Modern"                                                                 }
 ,{ 932,    "shift_jis",          "ANSI/OEM Japanese; Japanese (Shift-JIS)"                                                 }
 ,{ 936,    "gb2312",             "ANSI/OEM Simplified Chinese (PRC, Singapore); Chinese Simplified (GB2312)"               }
 ,{ 949,    "ks_c_5601-1987",     "ANSI/OEM Korean (Unified Hangul Code)"                                                   }
 ,{ 950,    "big5",               "ANSI/OEM Traditional Chinese (Taiwan; Hong Kong SAR, PRC); Chinese Traditional (Big5)"   }
 ,{ 1026,   "IBM1026",            "IBM EBCDIC Turkish (Latin 5)"                                                            }
 ,{ 1047,   "IBM01047",           "IBM EBCDIC Latin 1/Open System"                                                          }
 ,{ 1140,   "IBM01140",           "IBM EBCDIC US-Canada (037 + Euro symbol); IBM EBCDIC (US-Canada-Euro)"                   }
 ,{ 1141,   "IBM01141",           "IBM EBCDIC Germany (20273 + Euro symbol); IBM EBCDIC (Germany-Euro)"                     }
 ,{ 1142,   "IBM01142",           "IBM EBCDIC Denmark-Norway (20277 + Euro symbol); IBM EBCDIC (Denmark-Norway-Euro)"       }
 ,{ 1143,   "IBM01143",           "IBM EBCDIC Finland-Sweden (20278 + Euro symbol); IBM EBCDIC (Finland-Sweden-Euro)"       }
 ,{ 1144,   "IBM01144",           "IBM EBCDIC Italy (20280 + Euro symbol); IBM EBCDIC (Italy-Euro)"                         }
 ,{ 1145,   "IBM01145",           "IBM EBCDIC Latin America-Spain (20284 + Euro symbol); IBM EBCDIC (Spain-Euro)"           }
 ,{ 1146,   "IBM01146",           "IBM EBCDIC United Kingdom (20285 + Euro symbol); IBM EBCDIC (UK-Euro)"                   }
 ,{ 1147,   "IBM01147",           "IBM EBCDIC France (20297 + Euro symbol); IBM EBCDIC (France-Euro)"                       }
 ,{ 1148,   "IBM01148",           "IBM EBCDIC International (500 + Euro symbol); IBM EBCDIC (International-Euro)"           }
 ,{ 1149,   "IBM01149",           "IBM EBCDIC Icelandic (20871 + Euro symbol); IBM EBCDIC (Icelandic-Euro)"                 }
 ,{ 1200,   "utf-16",             "Unicode UTF-16, little endian byte order (BMP of ISO 10646);"                            }
 ,{ 1201,   "unicodeFFFE",        "Unicode UTF-16, big endian byte order"                                                   }
 ,{ 1250,   "windows-1250",       "ANSI Central European; Central European (Windows) "                                      }
 ,{ 1251,   "windows-1251",       "ANSI Cyrillic; Cyrillic (Windows)"                                                       }
 ,{ 1252,   "windows-1252",       "ANSI Latin 1; Western European (Windows)"                                                }
 ,{ 1253,   "windows-1253",       "ANSI Greek; Greek (Windows)"                                                             }
 ,{ 1254,   "windows-1254",       "ANSI Turkish; Turkish (Windows)"                                                         }
 ,{ 1255,   "windows-1255",       "ANSI Hebrew; Hebrew (Windows)"                                                           }
 ,{ 1256,   "windows-1256",       "ANSI Arabic; Arabic (Windows)"                                                           }
 ,{ 1257,   "windows-1257",       "ANSI Baltic; Baltic (Windows)"                                                           }
 ,{ 1258,   "windows-1258",       "ANSI/OEM Vietnamese; Vietnamese (Windows)"                                               }
 ,{ 1361,   "Johab",              "Korean (Johab)"                                                                          }
 ,{ 10000,  "macintosh",          "MAC Roman; Western European (Mac)"                                                       }
 ,{ 10001,  "x-mac-japanese",     "Japanese (Mac)"                                                                          }
 ,{ 10002,  "x-mac-chinesetrad",  "MAC Traditional Chinese (Big5); Chinese Traditional (Mac)"                               }
 ,{ 10003,  "x-mac-korean",       "Korean (Mac)"                                                                            }
 ,{ 10004,  "x-mac-arabic",       "Arabic (Mac)"                                                                            }
 ,{ 10005,  "x-mac-hebrew",       "Hebrew (Mac)"                                                                            }
 ,{ 10006,  "x-mac-greek",        "Greek (Mac)"                                                                             }
 ,{ 10007,  "x-mac-cyrillic",     "Cyrillic (Mac)"                                                                          }
 ,{ 10008,  "x-mac-chinesesimp",  "MAC Simplified Chinese (GB 2312); Chinese Simplified (Mac)"                              }
 ,{ 10010,  "x-mac-romanian",     "Romanian (Mac)"                                                                          }
 ,{ 10017,  "x-mac-ukranian",     "Ukranian (Mac)"                                                                          }
 ,{ 10021,  "x-mac-thai",         "Thai (Mac)"                                                                              }
 ,{ 10029,  "x-mac-ce",           "MAC Latin 2; Central European (Mac)"                                                     }
 ,{ 10079,  "x-mac-icelandic",    "Icelandic (Mac)"                                                                         }
 ,{ 10081,  "x-mac-turkish",      "Turkish (Mac)"                                                                           }
 ,{ 10082,  "x-mac-croatian",     "Croatian (Mac)"                                                                          }
 ,{ 12000,  "utf-32",             "Unicode UTF-32, little endian byte order"                                                }
 ,{ 12001,  "utf-32BE",           "Unicode UTF-32, big endian byte order"                                                   }
 ,{ 20000,  "x-Chinese_CNS",      "CNS Taiwan; Chinese Traditional (CNS) "                                                  }
 ,{ 20001,  "x-cp20001",          "TCA Taiwan"                                                                              }
 ,{ 20002,  "x_Chinese-Eten",     "Eten Taiwan; Chinese Traditional (Eten)"                                                 }
 ,{ 20003,  "x-cp20003",          "IBM5550 Taiwan"                                                                          }
 ,{ 20004,  "x-cp20004",          "TeleText Taiwan"                                                                         }
 ,{ 20005,  "x-cp20005",          "Want Taiwan"                                                                             }
 ,{ 20105,  "x-IA5",              "IA5 (IRV International Alphabet No. 5, 7-bit); Western European (IA5)"                   }
 ,{ 20106,  "x-IA5-German",       "IA5 German (7-bit)"                                                                      }
 ,{ 20107,  "x-IA5-Swedish",      "IA5 Swedish (7-bit)"                                                                     }
 ,{ 20108,  "x-IA5-Norwegian",    "IA5 Norwegian (7-bit)"                                                                   }
 ,{ 20127,  "us-ascii",           "US-ASCII (7-bit)"                                                                        }
 ,{ 20261,  "x-cp20261",          "T.61"                                                                                    }
 ,{ 20269,  "x-cp20269",          "ISO 6937 Non-Spacing Accent"                                                             }
 ,{ 20273,  "IBM273",             "IBM EBCDIC Germany"                                                                      }
 ,{ 20277,  "IBM277",             "IBM EBCDIC Denmark-Norway"                                                               }
 ,{ 20278,  "IBM278",             "IBM EBCDIC Finland-Sweden"                                                               }
 ,{ 20280,  "IBM280",             "IBM EBCDIC Italy"                                                                        }
 ,{ 20284,  "IBM284",             "IBM EBCDIC Latin America-Spain"                                                          }
 ,{ 20285,  "IBM285",             "IBM EBCDIC United Kingdom"                                                               }
 ,{ 20290,  "IBM290",             "IBM EBCDIC Japanese Katakana Extended"                                                   }
 ,{ 20297,  "IBM297",             "IBM EBCDIC France"                                                                       }
 ,{ 20420,  "IBM420",             "IBM EBCDIC Arabic"                                                                       }
 ,{ 20423,  "IBM423",             "IBM EBCDIC Greek"                                                                        }
 ,{ 20424,  "IBM424",             "IBM EBCDIC Hebrew"                                                                       }
 ,{ 20833,  "x-EBCDIC-KoreanExtended", "IBM EBCDIC Korean Extended"                                                         }
 ,{ 20838,  "IBM-Thai",           "IBM EBCDIC Thai"                                                                         }
 ,{ 20866,  "koi8-r",             "Russian (KOI8-R); Cyrillic (KOI8-R)"                                                     }
 ,{ 20871,  "IBM871",             "IBM EBCDIC Icelandic"                                                                    }
 ,{ 20880,  "IBM880",             "IBM EBCDIC Cyrillic Russian"                                                             }
 ,{ 20905,  "IMB905",             "IBM EBCDIC Turkish"                                                                      }
 ,{ 20924,  "IBM00924",           "IBM EBCDIC Latin 1/Open System (1047 + Euro symbol)"                                     }
 ,{ 20932,  "EUC-JP",             "Japanese (JIS 0208-1990 and 0121-1990)"                                                  }
 ,{ 20936,  "x-cp20936",          "Simplified Chinese (GB2312); Chinese Simplified (GB2312-80)"                             }
 ,{ 20949,  "x-cp20949",          "Korean Wansung"                                                                          }
 ,{ 21025,  "cp1025",             "IBM EBCDIC Cyrillic Serbian-Bulgarian"                                                   }
 ,{ 21866,  "koi8-u",             "Ukrainian (KOI8-U); Cyrillic (KOI8-U)"                                                   }
 ,{ 28591,  "iso-8859-1",         "ISO 8859-1 Latin 1; Western European (ISO)"                                              }
 ,{ 28592,  "iso-8859-2",         "ISO 8859-2 Central European; Central European (ISO)"                                     }
 ,{ 28593,  "iso-8859-3",         "ISO 8859-3 Latin 3 "                                                                     }
 ,{ 28594,  "iso-8859-4",         "ISO 8859-4 Baltic"                                                                       }
 ,{ 28595,  "iso-8859-5",         "ISO 8859-5 Cyrillic"                                                                     }
 ,{ 28596,  "iso-8859-6",         "ISO 8859-6 Arabic"                                                                       }
 ,{ 28597,  "iso-8859-7",         "ISO 8859-7 Greek"                                                                        }
 ,{ 28598,  "iso-8859-8",         "ISO 8859-8 Hebrew; Hebrew (ISO-Visual)"                                                  }
 ,{ 28599,  "iso-8859-9",         "ISO 8859-9 Turkish"                                                                      }
 ,{ 28603,  "iso-8859-13",        "ISO 8859-13 Estonian"                                                                    }
 ,{ 28605,  "iso-8859-15",        "ISO 8859-15 Latin 9"                                                                     }
 ,{ 29001,  "x-Europa",           "Europa 3"                                                                                }
 ,{ 38598,  "iso-8859-8-i",       "ISO 8859-8 Hebrew; Hebrew (ISO-Logical)"                                                 }
 ,{ 50220,  "iso-2022",           "ISO 2022 Japanese with no halfwidth Katakana; Japanese (JIS)"                            }
 ,{ 50221,  "csISO20220JP",       "ISO 2022 Japanese with halfwidth Katakana; Japanese (JIS-Allow 1 byte Kana)"             }
 ,{ 50222,  "iso-2022-jp",        "ISO 2022 Japanese JIS X 0201-1989; Japanese (JIS-Allow 1 byte Kana - SO/SI)"             }
 ,{ 50225,  "iso-2022-kr",        "ISO 2022 Korean"                                                                         }
 ,{ 50227,  "x-cp50227",          "ISO 2022 Simplified Chinese; Chinese Simplified (ISO 2022)"                              }
 ,{ 50229,  "",                   "ISO 2022 Traditional Chinese"                                                            }
 ,{ 50930,  "",                   "EBCDIC Japanese (Katakana) Extended"                                                     }
 ,{ 50931,  "",                   "EBCDIC US-Canada and Japanese"                                                           }
 ,{ 50933,  "",                   "EBCDIC Korean Extended and Korean"                                                       }
 ,{ 50935,  "",                   "EBCDIC Simplified Chinese Extended and Simplified Chinese"                               }
 ,{ 50936,  "",                   "EBCDIC Simplified Chinese"                                                               }
 ,{ 50937,  "",                   "EBCDIC US-Canada and Traditional Chinese"                                                }
 ,{ 50939,  "",                   "EBCDIC Japanese (Latin) Extended and Japanese"                                           }
 ,{ 51932,  "euc-jp",             "EUC Japanese"                                                                            }
 ,{ 51936,  "EUC-CN",             "EUC Simplified Chinese; Chinese Simplified (EUC)"                                        }
 ,{ 51949,  "euc-kr",             "EUC Korean"                                                                              }
 ,{ 51950,  "",                   "EUC Traditional Chinese"                                                                 }
 ,{ 52936,  "hz-gb-2312",         "HZ-GB2312 Simplified Chinese; Chinese Simplified (HZ)"                                   }
 ,{ 54936,  "GB18030",            "GB18030 Simplified Chinese (4 byte); Chinese Simplified (GB18030)"                       }
 ,{ 57002,  "x-iscii-de",         "ISCII Devanagari"                                                                        }
 ,{ 57003,  "x-iscii-be",         "ISCII Bengali"                                                                           }
 ,{ 57004,  "x-iscii-ta",         "ISCII Tamil"                                                                             }
 ,{ 57005,  "x-iscii-te",         "ISCII Telugu"                                                                            }
 ,{ 57006,  "x-iscii-as",         "ISCII Assamese"                                                                          }
 ,{ 57007,  "x-iscii-or",         "ISCII Oriya"                                                                             }
 ,{ 57008,  "x-iscii-ka",         "ISCII Kannada"                                                                           }
 ,{ 57009,  "x-iscii-ma",         "ISCII Malayalam"                                                                         }
 ,{ 57010,  "x-iscii-gu",         "ISCII Gujarati"                                                                          }
 ,{ 57011,  "x-iscii-pa",         "ISCII Punjabi"                                                                           }
 ,{ 65000,  "utf-7",              "Unicode (UTF-7)"                                                                         }
 ,{ 65001,  "utf-8",              "Unicode (UTF-8)"                                                                         }
 ,{    -1,  "",                   ""                                                                                        }
};

using CPIDNameMap = std::map<int,XString>;
using NameCPIDMap = std::map<XString,int>;
// 
static CPIDNameMap cp_cpid_map;
static NameCPIDMap cp_name_map;
static CPIDNameMap cp_info_map;
// 
void 
InitCodePageNames()
{
  // See if we are already initialized
  if(cp_cpid_map.empty())
  {
    CodePageName* pointer = cpNames;
    while(pointer->m_codepage_ID >= 0)
    {
      // Fill in both mappings
      if(strlen(pointer->m_codepage_Name))
      {
        XString lowerName(pointer->m_codepage_Name);
        lowerName.MakeLower();

        cp_cpid_map.insert(std::make_pair(pointer->m_codepage_ID,pointer->m_codepage_Name));
        cp_name_map.insert(std::make_pair(lowerName,pointer->m_codepage_ID));
      }
      // always fill in code to info 
      cp_info_map.insert(std::make_pair(pointer->m_codepage_ID,pointer->m_information));
      // Next record
      ++pointer;
    }
  }
}

XString
CharsetToCodePageInfo(XString p_charset)
{
  // Fill the code page names the first time
  InitCodePageNames();

  NameCPIDMap::iterator it = cp_name_map.find(p_charset);
  if(it != cp_name_map.end())
  {
    // Find the code page
    int codePage = it->second;
    CPIDNameMap::iterator in = cp_info_map.find(codePage);
    if(in != cp_info_map.end())
    {
      // Find the info record
      return in->second;
    }
  }
  return "";
}

// Getting the codepage number from the charset
// ""     -> Get the current codepage
// "name" -> Get legal number if "name" exists
//        -> Otherwise return -1 as an error
int
CharsetToCodepage(XString p_charset)
{
  int result = -1;

  // Get current codepage
  if(p_charset.IsEmpty())
  {
    return GetACP();
  }

  InitCodePageNames();
  p_charset.MakeLower();
  NameCPIDMap::iterator it = cp_name_map.find(p_charset);
  if(it != cp_name_map.end())
  {
    result = it->second;
  }
  return result;
}

// Getting the name of the codepage
// -1   -> Get the name of the current codepage
// >0   -> Get name of legal codepage if codepage is legel
//      -> Otherwise returns an empty string
XString
CodepageToCharset(int p_codepage)
{
  XString result;
  UINT cp = p_codepage < 0 ? GetACP() : p_codepage;

  InitCodePageNames();
  CPIDNameMap::iterator it = cp_cpid_map.find(cp);
  if(it != cp_cpid_map.end())
  {
    result = it->second;
  }
  return result;
}

// Find the value of a specific field within a HTTP header
// Header: firstvalue; field=value2
XString
FindFieldInHTTPHeader(XString p_headervalue,XString p_field)
{
  XString value;
  XString head(p_headervalue);
  int length = p_headervalue.GetLength();
  head.MakeLower();
  p_field.MakeLower();
  head.Replace(" =","=");
  p_field += "=";

  int pos = head.Find(p_field);
  if(pos > 0)
  {
    // Skip past the fieldname
    pos += p_field.GetLength();
    // Skip white space
    while(pos < length && isspace(head.GetAt(pos))) ++pos;
    if(head.GetAt(pos) == '\"')
    {
      // field="value2 value3"
      ++pos;
      int end = head.Find('\"',pos+1);
      if(end < 0) end = length+1;
      value = p_headervalue.Mid(pos,end-pos);
    }
    else
    {
      // field=value3; fld=value4, part=somewhat
      int end1 = head.Find(';',pos+1);
      int end2 = head.Find(',',pos+1);
      int end = end1 > 0 ? end1 : end2;
      if(end < 0) end = length+1;
      value = p_headervalue.Mid(pos,end-pos);
    }
  }
  return value;
}

// Set (modify) the field value within a HTTP header
// Header: theValue
// Header: firstvalue; field=value2
// Header: firstvalue; secondvalue="value2 with spaces", fld="value3"
XString
SetFieldInHTTPHeader(XString p_headervalue,XString p_field,XString p_value)
{
  // No value yet, or the main first value. Simply return the new value
  if(p_headervalue.IsEmpty() || p_field.IsEmpty())
  {
    return p_value;
  }
  // If doesn't exist yet: append to the header string
  XString existing = FindFieldInHTTPHeader(p_headervalue,p_field);
  if(existing.IsEmpty())
  {
    bool spaces = p_value.Find(' ') >= 0;
    p_headervalue += "; ";
    p_headervalue += p_field;
    p_headervalue += "=";
    if(spaces) p_headervalue += "\"";
    p_headervalue += p_value;
    if(spaces) p_headervalue += "\"";
    return p_headervalue;
  }

  // The hard part: replace the header value
  XString value;
  XString head(p_headervalue);
  int length = p_headervalue.GetLength();
  head.MakeLower();
  p_field.MakeLower();
  head.Replace(" =", "=");
  p_field += "=";

  int pos = head.Find(p_field);
  if(pos > 0)
  {
    // Skip past the field name
    pos += p_field.GetLength();
    // Skip white space
    while (pos < length && isspace(head.GetAt(pos))) ++pos;
    if (head.GetAt(pos) == '\"')
    {
      // field="value2 value3"
      ++pos;
      int end = head.Find('\"', pos + 1);
      if (end < 0) end = length + 1;
      value = p_headervalue.Left(pos) + p_value + p_headervalue.Mid(end);
    }
    else
    {
      // field=value3; fld=value4, part=somewhat
      int end1 = head.Find(';', pos + 1);
      int end2 = head.Find(',', pos + 1);
      int end = end1 > 0 ? end1 : end2;
      if (end < 0) end = length + 1;
      value = p_headervalue.Left(pos) + p_value + p_headervalue.Mid(end);
    }
  }
  return value;
}

// Find the charset in the content-type header
// content-type: application/json; charset=utf-16
XString
FindCharsetInContentType(XString p_contentType)
{
  return FindFieldInHTTPHeader(p_contentType,"charset");
}

// Find the mime-type in the content-type header
// content-type: application/json; charset=utf-16
XString
FindMimeTypeInContentType(XString p_contentType)
{
  XString mime(p_contentType);
  unsigned length = p_contentType.GetLength();
  unsigned pos = 0;
  mime.MakeLower();

  while(pos < length)
  {
    unsigned ch = mime.GetAt(pos);
    if((ch == ';' || ch == ',') || isspace(ch) || ch == 0)
    {
      break;
    }
    ++pos;
  }
  // Return mime only
  return p_contentType.Left(pos);
}

// Construct an UTF-8 Byte-Order-Mark
XString ConstructBOM()
{
  XString bom;

  // 3 BYTE UTF-8 Byte-order-Mark "\0xEF\0xBB\0xBF"
  // Strange construction, but the only way to do it
  // because a XString is a signed char field
  bom.GetBufferSetLength(4);
  bom.SetAt(0,(unsigned char)0xEF);
  bom.SetAt(1,(unsigned char)0xBB);
  bom.SetAt(2,(unsigned char)0xBF);
  bom.SetAt(3,0);
  bom.ReleaseBuffer(3);

  return bom;
}

// Construct a UTF-16 Byte-Order-Mark
std::wstring ConstructBOMUTF16()
{
  std::wstring bom;

  // 2 BYTE UTF-16 Byte-order-Mark "\0xFE\0xFF"
  bom += (unsigned char)0xFE;
  bom += (unsigned char)0xFF;

  return bom;
}

// Construct a BOM
XString ConstructBOM(StringEncoding p_encoding)
{
  XString bom;

  switch (p_encoding)
  {
    case StringEncoding::ENC_UTF8:    bom = ConstructBOM();      break;
    case StringEncoding::ENC_ISO88591:// Fall through
    case StringEncoding::ENC_Plain:   // Fall through
    default:                          break;
  }
  return bom;
}

bool
TryConvertWideString(const uchar* p_buffer
                    ,int          p_length
                    ,XString      p_charset
                    ,XString&     p_string
                    ,bool&        p_foundBOM)
{
  UINT codePage = GetACP(); // Default is to use the current codepage
  int   iLength = -1;       // I think it will be null terminated
  bool  result  = false;
  BYTE* extraBuffer = nullptr;

  // Early dropout
  if(p_buffer == nullptr || p_length == 0)
  {
    return true;
  }

  // Reset result
  p_string.Empty();
  p_foundBOM = false;

  // Fill the code page names the first time
  InitCodePageNames();

  if(((BYTE*)p_buffer)[p_length    ] != 0 &&
     ((BYTE*)p_buffer)[p_length + 1] != 0)
  {
    // Unicode buffer not null terminated !!! 
    // Completely legal to get from the HTTP service.
    // So make it so, and expand with two (!!) extra null chars.
    extraBuffer = new BYTE[p_length + 4];
    memcpy_s(extraBuffer,p_length + 2,p_buffer,p_length + 2);
    // Needs two (!) extra nulls in the buffer for conversion
    extraBuffer[++p_length] = 0;
    extraBuffer[++p_length] = 0;
    p_buffer = extraBuffer;
  }

  // Check if we know the codepage from the charset
  if(!p_charset.IsEmpty())
  {
    NameCPIDMap::iterator it = cp_name_map.find(p_charset);
    if(it != cp_name_map.end())
    {
      codePage = it->second;
    }
  }

  // Scanning for a BOM UTF-16 in Little-endian mode for Intel processors
  DWORD_PTR extra = 0;
  if(((BYTE*)p_buffer)[0] == 0xFF && ((BYTE*)p_buffer)[1] == 0xFE)
  {
    extra      = 2;     // Offset in buffer for conversion
    p_length  -= 2;     // Buffer is effectively 2 shorter
    p_foundBOM = true;  // Remember we found a BOM
  }

  // Getting the length of the translation buffer first
  iLength = ::WideCharToMultiByte(codePage,
                                  0, 
                                  (LPCWSTR)((DWORD_PTR)p_buffer + extra), 
                                  -1, // p_length, 
                                  NULL, 
                                  0,
                                  NULL,
                                  NULL);
  // Convert the string if we find something
  if(iLength > 0)
  {
    iLength *= 2; // Funny but needed in most cases with 3-byte composite chars
    char* buffer = new char[iLength];
    if(buffer != NULL)
    {
      DWORD dwFlag = 0; // WC_COMPOSITECHECK | WC_DISCARDNS;
      memset(buffer, 0, iLength * sizeof(char));
      iLength = ::WideCharToMultiByte(codePage,
                                      dwFlag, 
                                      (LPCWSTR)((DWORD_PTR)p_buffer + extra),
                                      -1, // p_length, 
                                      (LPSTR)buffer, 
                                      iLength,
                                      NULL,
                                      NULL);
      if(iLength > 0)
      {
        p_string = buffer;
        result   = true;
      }
      delete[] buffer;
    }
  }
  // Freeing the extra buffer
  if(extraBuffer)
  {
    delete[] extraBuffer;
  }
  return result;
}

// Try to convert a MBCS string to UTF-16 buffer
bool    
TryCreateWideString(const XString& p_string
                   ,const XString  p_charset
                   ,const bool     p_doBom
                   ,      uchar**  p_buffer
                   ,      int&     p_length)
{
  bool   result = false;    // No yet
  UINT codePage = GetACP(); // Default the current codepage
  int   iLength = -1;       // I think it will be null terminated
  DWORD dwFlags = MB_ERR_INVALID_CHARS;

  // Reset result
  *p_buffer = NULL;
  p_length = 0;

  // Check if we know the codepage from the character set
  if(!p_charset.IsEmpty())
  {
    // Fill the code page names the first time
    InitCodePageNames();

    NameCPIDMap::iterator it = cp_name_map.find(p_charset);
    if(it != cp_name_map.end())
    {
      codePage = it->second;
    }
  }

  // Getting the length of the buffer, by specifying no output
  iLength = MultiByteToWideChar(codePage
                               ,dwFlags
                               ,p_string.GetString()
                               ,-1 // p_string.GetLength()
                               ,NULL
                               ,0);
  if(iLength)
  {
    // Getting a UTF-16 wide-character buffer
    // +2 chars for a BOM, +2 chars for the closing zeros
    *p_buffer = new uchar[2*iLength + 4];
    unsigned char* buffer = (unsigned char*) *p_buffer;

    // Construct an UTF-16 Byte-Order-Mark
    // In little endian order. OS/X calls this UTF-16LE
    if(p_doBom)
    {
      *buffer++ = 0xFF;
      *buffer++ = 0xFE;
    }

    // Doing the 'real' conversion
    iLength = MultiByteToWideChar(codePage
                                  ,dwFlags
                                  ,p_string.GetString()
                                  ,-1 // p_string.GetLength()
                                  ,(LPWSTR)buffer
                                  ,iLength);
    if(iLength > 0)
    {
      // Buffer length is twice the number of logical characters
      iLength *= 2;
      // UTF-16 has two closing zeros
      buffer[iLength    ] = 0;
      buffer[iLength + 1] = 0;
      // Result is OK, but 2 longer if we constructed a BOM
      p_length = iLength + (p_doBom ? 2 : 0);
      result   = true;
    }
    else
    {
      // No glory
      delete [] (*p_buffer);
      *p_buffer = 0;
    }
  }
  return result;
}


// Decoding incoming strings from the internet
// Defaults to UTF-8 encoding
XString
DecodeStringFromTheWire(XString p_string,XString p_charset /*="utf-8"*/)
{
  // Check for empty charset
  if(p_charset.IsEmpty())
  {
    p_charset = "utf-8";
  }

  // Now decode the UTF-8 in the encoded string, to decoded MBCS
  uchar* buffer = nullptr;
  int    length = 0;
  if (TryCreateWideString(p_string,p_charset,false,&buffer,length))
  {
    XString decoded;
    bool foundBom = false;

    if (TryConvertWideString(buffer,length,"",decoded,foundBom))
    {
      p_string = decoded;
    }
  }
  delete[] buffer;
  return p_string;
}

// Encode to string for internet. Defaults to UTF-8 encoding
XString
EncodeStringForTheWire(XString p_string,XString p_charset /*="utf-8"*/)
{
  // Check for empty charset
  if(p_charset.IsEmpty())
  {
    p_charset = "utf-8";
  }

  // Now encode MBCS to UTF-8 without a BOM
  uchar*  buffer = nullptr;
  int     length = 0;
  if(TryCreateWideString(p_string,"",false,&buffer,length))
  {
    XString encoded;
    bool foundBom = false;

    if(TryConvertWideString(buffer,length,p_charset,encoded,foundBom))
    {
      p_string = encoded;
    }
  }
  delete[] buffer;
  return p_string;
}

// Scan for UTF-8 chars in a string
bool
DetectUTF8(XString& p_string)
{
  const unsigned char* bytes = (const unsigned char*)p_string.GetString();
  bool detectedUTF8 = false;
  unsigned int cp = 0;
  int num = 0;

  while(*bytes != 0x00)
  {
    if((*bytes & 0x80) == 0x00)
    {
      // U+0000 to U+007F 
      cp = (*bytes & 0x7F);
      num = 1;
    }
    else if((*bytes & 0xE0) == 0xC0)
    {
      // U+0080 to U+07FF 
      cp = (*bytes & 0x1F);
      detectedUTF8 = true;
      num = 2;
    }
    else if((*bytes & 0xF0) == 0xE0)
    {
      // U+0800 to U+FFFF 
      cp = (*bytes & 0x0F);
      detectedUTF8 = true;
      num = 3;
    }
    else if((*bytes & 0xF8) == 0xF0)
    {
      // U+10000 to U+10FFFF 
      cp = (*bytes & 0x07);
      detectedUTF8 = true;
      num = 4;
    }
    else
    {
      return false;
    }
    bytes += 1;
    for(int i = 1; i < num; ++i)
    {
      if((*bytes & 0xC0) != 0x80)
      {
        // Not a UTF-8 6 bit follow up char
        return false;
      }
      cp = (cp << 6) | (*bytes & 0x3F);
      bytes += 1;
    }

    // Check for overlong encodings or UTF-16 surrogates
    if((cp >  0x10FFFF) ||
      ((cp >= 0xD800)  && (cp  <= 0xDFFF)) ||
      ((cp <= 0x007F)  && (num != 1)) ||
      ((cp >= 0x0080)  && (cp  <= 0x07FF)   && (num != 2)) ||
      ((cp >= 0x0800)  && (cp  <= 0xFFFF)   && (num != 3)) ||
      ((cp >= 0x10000) && (cp  <= 0x1FFFFF) && (num != 4)))
    {
      return false;
    }
  }
  return detectedUTF8;
}
