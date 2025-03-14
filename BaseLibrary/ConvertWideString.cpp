/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ConvertWideString.cpp
//
// Copyright (c) 2014-2025 ir. W.E. Huisman
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
#include "WinFile.h"
#include <map>
#include <xstring>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

typedef struct _cpNames
{
  int     m_codepage_ID;    // Active codepage ID
  XString m_codepage_Name;  // Codepage name as in HTTP protocol "charset"
  XString m_information;    // MSDN Documentary description
}
CodePageName;

static CodePageName cpNames[] =
{
  { 000,     _T("ACP"),                 _T("Default ANSI code page")                                                                  }
 ,{ 001,     _T("OEMCP"),               _T("Default OEM code page")                                                                   }
 ,{ 002,     _T("MACCP"),               _T("Default MAC code page")                                                                   }
 ,{ 003,     _T("THREAD"),              _T("Current thread's ANSI code page")                                                         }
 ,{ 037,     _T("IBM037"),              _T("IBM EBCDIC US-Canada")                                                                    }
 ,{ 042,     _T("SYMBOL"),              _T("Symbol translations")                                                                     }
 ,{ 437,     _T("IBM437"),              _T("OEM United States")                                                                       }
 ,{ 500,     _T("IBM500"),              _T("IBM EBCDIC International")                                                                }
 ,{ 708,     _T("ASMO-708"),            _T("Arabic (ASMO 708)")                                                                       }
 ,{ 709,     _T(""),                    _T("Arabic (ASMO-449+, BCON V4)")                                                             }
 ,{ 710,     _T(""),                    _T("Arabic - Transparent-Arabic")                                                             }
 ,{ 720,     _T("DOS-720"),             _T("Arabic(Transparent ASMO); Arabic (DOS)")                                                  }
 ,{ 737,     _T("ibm737"),              _T("OEM Greek (Formerly 437G); Greek (DOS)")                                                  }
 ,{ 775,     _T("ibm775"),              _T("OEM Baltic; Baltic (DOS)")                                                                }
 ,{ 850,     _T("ibm850"),              _T("OEM Multilingual Latin 1; Western European (DOS)")                                        }
 ,{ 852,     _T("imb852"),              _T("OEM Latin 2; Central European (DOS)")                                                     }
 ,{ 855,     _T("ibm855"),              _T("OEM Cyrillic (primarily Russian)")                                                        }
 ,{ 857,     _T("ibm857"),              _T("OEM Turkish; Turkish (DOS)")                                                              }
 ,{ 858,     _T("IBM00858"),            _T("OEM Multilingual Latin 1 + Euro symbol")                                                  }
 ,{ 860,     _T("IBM860"),              _T("OEM Portuguese; Portuguese (DOS)")                                                        }
 ,{ 861,     _T("ibm861"),              _T("OEM Icelandic; Icelandic (DOS)")                                                          }
 ,{ 862,     _T("DOS-862"),             _T("OEM Hebrew; Hebrew (DOS)")                                                                }
 ,{ 863,     _T("IBM863"),              _T("OEM French Canadian; French Canadian (DOS)")                                              }
 ,{ 864,     _T("IBM864"),              _T("OEM Arabic; Arabic (864)")                                                                }
 ,{ 865,     _T("IBM865"),              _T("OEM Nordic; Nordic (DOS)")                                                                }
 ,{ 866,     _T("cp866"),               _T("OEM Russian; Cyrillic (DOS)")                                                             }
 ,{ 869,     _T("ibm869"),              _T("OEM Modern Greek; Greek, Modern (DOS)")                                                   }
 ,{ 870,     _T("IBM870"),              _T("IBM EBCDIC Multilingual/ROECE (Latin 2); IBM EBCDIC Multilingual Latin 2")                }
 ,{ 874,     _T("windows-874"),         _T("ANSI/OEM Thai (same as 28605, ISO 8859-15); Thai (Windows)")                              }
 ,{ 875,     _T("cp875"),               _T("IBM EBCDIC Greek Modern")                                                                 }
 ,{ 932,     _T("shift_jis"),           _T("ANSI/OEM Japanese; Japanese (Shift-JIS)")                                                 }
 ,{ 936,     _T("gb2312"),              _T("ANSI/OEM Simplified Chinese (PRC, Singapore); Chinese Simplified (GB2312)")               }
 ,{ 949,     _T("ks_c_5601-1987"),      _T("ANSI/OEM Korean (Unified Hangul Code)")                                                   }
 ,{ 950,     _T("big5"),                _T("ANSI/OEM Traditional Chinese (Taiwan; Hong Kong SAR, PRC); Chinese Traditional (Big5)")   }
 ,{ 1026,    _T("IBM1026"),             _T("IBM EBCDIC Turkish (Latin 5)")                                                            }
 ,{ 1047,    _T("IBM01047"),            _T("IBM EBCDIC Latin 1/Open System")                                                          }
 ,{ 1140,    _T("IBM01140"),            _T("IBM EBCDIC US-Canada (037 + Euro symbol); IBM EBCDIC (US-Canada-Euro)")                   }
 ,{ 1141,    _T("IBM01141"),            _T("IBM EBCDIC Germany (20273 + Euro symbol); IBM EBCDIC (Germany-Euro)")                     }
 ,{ 1142,    _T("IBM01142"),            _T("IBM EBCDIC Denmark-Norway (20277 + Euro symbol); IBM EBCDIC (Denmark-Norway-Euro)")       }
 ,{ 1143,    _T("IBM01143"),            _T("IBM EBCDIC Finland-Sweden (20278 + Euro symbol); IBM EBCDIC (Finland-Sweden-Euro)")       }
 ,{ 1144,    _T("IBM01144"),            _T("IBM EBCDIC Italy (20280 + Euro symbol); IBM EBCDIC (Italy-Euro)")                         }
 ,{ 1145,    _T("IBM01145"),            _T("IBM EBCDIC Latin America-Spain (20284 + Euro symbol); IBM EBCDIC (Spain-Euro)")           }
 ,{ 1146,    _T("IBM01146"),            _T("IBM EBCDIC United Kingdom (20285 + Euro symbol); IBM EBCDIC (UK-Euro)")                   }
 ,{ 1147,    _T("IBM01147"),            _T("IBM EBCDIC France (20297 + Euro symbol); IBM EBCDIC (France-Euro)")                       }
 ,{ 1148,    _T("IBM01148"),            _T("IBM EBCDIC International (500 + Euro symbol); IBM EBCDIC (International-Euro)")           }
 ,{ 1149,    _T("IBM01149"),            _T("IBM EBCDIC Icelandic (20871 + Euro symbol); IBM EBCDIC (Icelandic-Euro)")                 }
 ,{ 1200,    _T("utf-16"),              _T("Unicode UTF-16, little endian byte order (BMP of ISO 10646);")                            }
 ,{ 1201,    _T("unicodeFFFE"),         _T("Unicode UTF-16, big endian byte order")                                                   }
 ,{ 1250,    _T("windows-1250"),        _T("ANSI Central European; Central European (Windows)")                                       }
 ,{ 1251,    _T("windows-1251"),        _T("ANSI Cyrillic; Cyrillic (Windows)")                                                       }
 ,{ 1252,    _T("windows-1252"),        _T("ANSI Latin 1; Western European (Windows)")                                                }
 ,{ 1253,    _T("windows-1253"),        _T("ANSI Greek; Greek (Windows)")                                                             }
 ,{ 1254,    _T("windows-1254"),        _T("ANSI Turkish; Turkish (Windows)")                                                         }
 ,{ 1255,    _T("windows-1255"),        _T("ANSI Hebrew; Hebrew (Windows)")                                                           }
 ,{ 1256,    _T("windows-1256"),        _T("ANSI Arabic; Arabic (Windows)")                                                           }
 ,{ 1257,    _T("windows-1257"),        _T("ANSI Baltic; Baltic (Windows)")                                                           }
 ,{ 1258,    _T("windows-1258"),        _T("ANSI/OEM Vietnamese; Vietnamese (Windows)")                                               }
 ,{ 1361,    _T("Johab"),               _T("Korean (Johab)")                                                                          }
 ,{ 10000,   _T("macintosh"),           _T("MAC Roman; Western European (Mac)")                                                       }
 ,{ 10001,   _T("x-mac-japanese"),      _T("Japanese (Mac)")                                                                          }
 ,{ 10002,   _T("x-mac-chinesetrad"),   _T("MAC Traditional Chinese (Big5); Chinese Traditional (Mac)")                               }
 ,{ 10003,   _T("x-mac-korean"),        _T("Korean (Mac)")                                                                            }
 ,{ 10004,   _T("x-mac-arabic"),        _T("Arabic (Mac)")                                                                            }
 ,{ 10005,   _T("x-mac-hebrew"),        _T("Hebrew (Mac)")                                                                            }
 ,{ 10006,   _T("x-mac-greek"),         _T("Greek (Mac)")                                                                             }
 ,{ 10007,   _T("x-mac-cyrillic"),      _T("Cyrillic (Mac)")                                                                          }
 ,{ 10008,   _T("x-mac-chinesesimp"),   _T("MAC Simplified Chinese (GB 2312); Chinese Simplified (Mac)")                              }
 ,{ 10010,   _T("x-mac-romanian"),      _T("Romanian (Mac)")                                                                          }
 ,{ 10017,   _T("x-mac-ukranian"),      _T("Ukranian (Mac)")                                                                          }
 ,{ 10021,   _T("x-mac-thai"),          _T("Thai (Mac)")                                                                              }
 ,{ 10029,   _T("x-mac-ce"),            _T("MAC Latin 2; Central European (Mac)")                                                     }
 ,{ 10079,   _T("x-mac-icelandic"),     _T("Icelandic (Mac)")                                                                         }
 ,{ 10081,   _T("x-mac-turkish"),       _T("Turkish (Mac)")                                                                           }
 ,{ 10082,   _T("x-mac-croatian"),      _T("Croatian (Mac)")                                                                          }
 ,{ 12000,   _T("utf-32"),              _T("Unicode UTF-32, little endian byte order")                                                }
 ,{ 12001,   _T("utf-32BE"),            _T("Unicode UTF-32, big endian byte order")                                                   }
 ,{ 20000,   _T("x-Chinese_CNS"),       _T("CNS Taiwan; Chinese Traditional (CNS)")                                                   }
 ,{ 20001,   _T("x-cp20001"),           _T("TCA Taiwan")                                                                              }
 ,{ 20002,   _T("x_Chinese-Eten"),      _T("Eten Taiwan; Chinese Traditional (Eten)")                                                 }
 ,{ 20003,   _T("x-cp20003"),           _T("IBM5550 Taiwan")                                                                          }
 ,{ 20004,   _T("x-cp20004"),           _T("TeleText Taiwan")                                                                         }
 ,{ 20005,   _T("x-cp20005"),           _T("Want Taiwan")                                                                             }
 ,{ 20105,   _T("x-IA5"),               _T("IA5 (IRV International Alphabet No. 5, 7-bit); Western European (IA5)")                   }
 ,{ 20106,   _T("x-IA5-German"),        _T("IA5 German (7-bit)")                                                                      }
 ,{ 20107,   _T("x-IA5-Swedish"),       _T("IA5 Swedish (7-bit)")                                                                     }
 ,{ 20108,   _T("x-IA5-Norwegian"),     _T("IA5 Norwegian (7-bit)")                                                                   }
 ,{ 20127,   _T("us-ascii"),            _T("US-ASCII (7-bit)")                                                                        }
 ,{ 20261,   _T("x-cp20261"),           _T("T.61")                                                                                    }
 ,{ 20269,   _T("x-cp20269"),           _T("ISO 6937 Non-Spacing Accent")                                                             }
 ,{ 20273,   _T("IBM273"),              _T("IBM EBCDIC Germany")                                                                      }
 ,{ 20277,   _T("IBM277"),              _T("IBM EBCDIC Denmark-Norway")                                                               }
 ,{ 20278,   _T("IBM278"),              _T("IBM EBCDIC Finland-Sweden")                                                               }
 ,{ 20280,   _T("IBM280"),              _T("IBM EBCDIC Italy")                                                                        }
 ,{ 20284,   _T("IBM284"),              _T("IBM EBCDIC Latin America-Spain")                                                          }
 ,{ 20285,   _T("IBM285"),              _T("IBM EBCDIC United Kingdom")                                                               }
 ,{ 20290,   _T("IBM290"),              _T("IBM EBCDIC Japanese Katakana Extended")                                                   }
 ,{ 20297,   _T("IBM297"),              _T("IBM EBCDIC France")                                                                       }
 ,{ 20420,   _T("IBM420"),              _T("IBM EBCDIC Arabic")                                                                       }
 ,{ 20423,   _T("IBM423"),              _T("IBM EBCDIC Greek")                                                                        }
 ,{ 20424,   _T("IBM424"),              _T("IBM EBCDIC Hebrew")                                                                       }
 ,{ 20833,   _T("x-EBCDIC-KoreanExtended"),  _T("IBM EBCDIC Korean Extended")                                                         }
 ,{ 20838,   _T("IBM-Thai"),            _T("IBM EBCDIC Thai")                                                                         }
 ,{ 20866,   _T("koi8-r"),              _T("Russian (KOI8-R); Cyrillic (KOI8-R)")                                                     }
 ,{ 20871,   _T("IBM871"),              _T("IBM EBCDIC Icelandic")                                                                    }
 ,{ 20880,   _T("IBM880"),              _T("IBM EBCDIC Cyrillic Russian")                                                             }
 ,{ 20905,   _T("IMB905"),              _T("IBM EBCDIC Turkish")                                                                      }
 ,{ 20924,   _T("IBM00924"),            _T("IBM EBCDIC Latin 1/Open System (1047 + Euro symbol)")                                     }
 ,{ 20932,   _T("EUC-JP"),              _T("Japanese (JIS 0208-1990 and 0121-1990)")                                                  }
 ,{ 20936,   _T("x-cp20936"),           _T("Simplified Chinese (GB2312); Chinese Simplified (GB2312-80)")                             }
 ,{ 20949,   _T("x-cp20949"),           _T("Korean Wansung")                                                                          }
 ,{ 21025,   _T("cp1025"),              _T("IBM EBCDIC Cyrillic Serbian-Bulgarian")                                                   }
 ,{ 21866,   _T("koi8-u"),              _T("Ukrainian (KOI8-U); Cyrillic (KOI8-U)")                                                   }
 ,{ 28591,   _T("iso-8859-1"),          _T("ISO 8859-1 Latin 1; Western European (ISO)")                                              }
 ,{ 28592,   _T("iso-8859-2"),          _T("ISO 8859-2 Central European; Central European (ISO)")                                     }
 ,{ 28593,   _T("iso-8859-3"),          _T("ISO 8859-3 Latin 3")                                                                      }
 ,{ 28594,   _T("iso-8859-4"),          _T("ISO 8859-4 Baltic")                                                                       }
 ,{ 28595,   _T("iso-8859-5"),          _T("ISO 8859-5 Cyrillic")                                                                     }
 ,{ 28596,   _T("iso-8859-6"),          _T("ISO 8859-6 Arabic")                                                                       }
 ,{ 28597,   _T("iso-8859-7"),          _T("ISO 8859-7 Greek")                                                                        }
 ,{ 28598,   _T("iso-8859-8"),          _T("ISO 8859-8 Hebrew; Hebrew (ISO-Visual)")                                                  }
 ,{ 28599,   _T("iso-8859-9"),          _T("ISO 8859-9 Turkish")                                                                      }
 ,{ 28603,   _T("iso-8859-13"),         _T("ISO 8859-13 Estonian")                                                                    }
 ,{ 28605,   _T("iso-8859-15"),         _T("ISO 8859-15 Latin 9")                                                                     }
 ,{ 29001,   _T("x-Europa"),            _T("Europa 3")                                                                                }
 ,{ 38598,   _T("iso-8859-8-i"),        _T("ISO 8859-8 Hebrew; Hebrew (ISO-Logical)")                                                 }
 ,{ 50220,   _T("iso-2022"),            _T("ISO 2022 Japanese with no halfwidth Katakana; Japanese (JIS)")                            }
 ,{ 50221,   _T("csISO20220JP"),        _T("ISO 2022 Japanese with halfwidth Katakana; Japanese (JIS-Allow 1 byte Kana)")             }
 ,{ 50222,   _T("iso-2022-jp"),         _T("ISO 2022 Japanese JIS X 0201-1989; Japanese (JIS-Allow 1 byte Kana - SO/SI)")             }
 ,{ 50225,   _T("iso-2022-kr"),         _T("ISO 2022 Korean")                                                                         }
 ,{ 50227,   _T("x-cp50227"),           _T("ISO 2022 Simplified Chinese; Chinese Simplified (ISO 2022)")                              }
 ,{ 50229,   _T(""),                    _T("ISO 2022 Traditional Chinese")                                                            }
 ,{ 50930,   _T(""),                    _T("EBCDIC Japanese (Katakana) Extended")                                                     }
 ,{ 50931,   _T(""),                    _T("EBCDIC US-Canada and Japanese")                                                           }
 ,{ 50933,   _T(""),                    _T("EBCDIC Korean Extended and Korean")                                                       }
 ,{ 50935,   _T(""),                    _T("EBCDIC Simplified Chinese Extended and Simplified Chinese")                               }
 ,{ 50936,   _T(""),                    _T("EBCDIC Simplified Chinese")                                                               }
 ,{ 50937,   _T(""),                    _T("EBCDIC US-Canada and Traditional Chinese")                                                }
 ,{ 50939,   _T(""),                    _T("EBCDIC Japanese (Latin) Extended and Japanese")                                           }
 ,{ 51932,   _T("euc-jp"),              _T("EUC Japanese")                                                                            }
 ,{ 51936,   _T("EUC-CN"),              _T("EUC Simplified Chinese; Chinese Simplified (EUC)")                                        }
 ,{ 51949,   _T("euc-kr"),              _T("EUC Korean")                                                                              }
 ,{ 51950,   _T(""),                    _T("EUC Traditional Chinese")                                                                 }
 ,{ 52936,   _T("hz-gb-2312"),          _T("HZ-GB2312 Simplified Chinese; Chinese Simplified (HZ)")                                   }
 ,{ 54936,   _T("GB18030"),             _T("GB18030 Simplified Chinese (4 byte); Chinese Simplified (GB18030)")                       }
 ,{ 57002,   _T("x-iscii-de"),          _T("ISCII Devanagari")                                                                        }
 ,{ 57003,   _T("x-iscii-be"),          _T("ISCII Bengali")                                                                           }
 ,{ 57004,   _T("x-iscii-ta"),          _T("ISCII Tamil")                                                                             }
 ,{ 57005,   _T("x-iscii-te"),          _T("ISCII Telugu")                                                                            }
 ,{ 57006,   _T("x-iscii-as"),          _T("ISCII Assamese")                                                                          }
 ,{ 57007,   _T("x-iscii-or"),          _T("ISCII Oriya")                                                                             }
 ,{ 57008,   _T("x-iscii-ka"),          _T("ISCII Kannada")                                                                           }
 ,{ 57009,   _T("x-iscii-ma"),          _T("ISCII Malayalam")                                                                         }
 ,{ 57010,   _T("x-iscii-gu"),          _T("ISCII Gujarati")                                                                          }
 ,{ 57011,   _T("x-iscii-pa"),          _T("ISCII Punjabi")                                                                           }
 ,{ 65000,   _T("utf-7"),               _T("Unicode (UTF-7)")                                                                         }
 ,{ 65001,   _T("utf-8"),               _T("Unicode (UTF-8)")                                                                         }
 ,{    -1,   _T(""),                    _T("")                                                                                        }
};

// Name mapping of code-page names is case insensitive
struct CPCompare
{
  bool operator()(const XString& p_left,const XString& p_right) const
  {
    return (p_left.CompareNoCase(p_right) < 0);
  }
};

using CPIDNameMap = std::map<int,XString>;
using NameCPIDMap = std::map<XString,int,CPCompare>;
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
      if(_tcslen(pointer->m_codepage_Name))
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
  return _T("");
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
  head.MakeLower();
  p_field.MakeLower();
  head.Replace(_T(" ="),_T("="));
  p_field += _T("=");

  int pos = head.Find(p_field);
  if(pos > 0)
  {
    int length = p_headervalue.GetLength();
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
    p_headervalue += _T("; ");
    p_headervalue += p_field;
    p_headervalue += _T("=");
    if(spaces) p_headervalue += _T("\"");
    p_headervalue += p_value;
    if(spaces) p_headervalue += _T("\"");
    return p_headervalue;
  }

  // The hard part: replace the header value
  XString value;
  XString head(p_headervalue);
  head.MakeLower();
  p_field.MakeLower();
  head.Replace(_T(" ="),_T("="));
  p_field += _T("=");

  int pos = head.Find(p_field);
  if(pos > 0)
  {
    int length = p_headervalue.GetLength();

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
  return FindFieldInHTTPHeader(p_contentType,_T("charset"));
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
XString ConstructBOMUTF8()
{
  _TUCHAR bom[4];

  // 3 BYTE UTF-8 Byte-order-Mark "\0xEF\0xBB\0xBF"
  // Strange construction, but the only way to do it
  // because a XString is a signed char field
#ifdef _UNICODE
  bom[0] = 0xBBEF;
  bom[1] = 0xBF00;
  bom[2] = 0;
  bom[3] = 0;
#else
  bom[0] = (_TUCHAR) 0xEF;
  bom[1] = (_TUCHAR) 0xBB;
  bom[2] = (_TUCHAR) 0xBF;
  bom[3] = 0;
#endif

  return XString(bom);
}

#ifdef _UNICODE
// Convert a narrow string (utf-8, MBCS, win1252) to UTF-16
bool
TryConvertNarrowString(const BYTE* p_buffer
                      ,int         p_length
                      ,XString     p_charset
                      ,XString&    p_string
                      ,bool&       p_foundBOM)
{
  UINT   codePage = GetACP(); // Default is to use the current codepage
  int    iLength  = -1;       // I think it will be null terminated
  bool   result   = false;
  TCHAR* extraBuffer = nullptr;

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

  // Check if we know the codepage from the character set
  if(p_charset.IsEmpty())
  {
    p_charset = _T("utf-8");
  }

  // Check for a bom
  Encoding bom = Encoding::EN_ACP;
  unsigned skipBytes = 0;
  BOMOpenResult bomfound = WinFile::DefuseBOM(p_buffer,bom,skipBytes);
  if(bomfound == BOMOpenResult::BOM)
  {
    p_foundBOM = true;
    switch(bom)
    {
      case Encoding::UTF8:     p_charset = _T("utf-8");       break;
      case Encoding::BE_UTF16: p_charset = _T("unicodeFFFE"); break;
      case Encoding::BE_UTF32: p_charset = _T("utf-32BE");    break;
      case Encoding::LE_UTF32: p_charset = _T("utf-32");      break;
      case Encoding::LE_UTF16:
      {
        // NO Conversion needed! This is what we are!
        p_string = reinterpret_cast<LPCTSTR>(p_buffer);
        return true;
      }
    }
  }

  // Now find our codepage
  NameCPIDMap::iterator it = cp_name_map.find(p_charset);
  if(it != cp_name_map.end())
  {
    codePage = it->second;
  }

  // Getting the length of the buffer, by specifying no output
  iLength = MultiByteToWideChar(codePage
                               ,0
                               ,(LPCCH)p_buffer
                               ,-1 // p_string.GetLength()
                               ,NULL
                               ,0);
  if(iLength)
  {
    // Getting a UTF-16 wide-character buffer
    // +2 chars for a BOM, +2 chars for the closing zeros
    extraBuffer = new TCHAR[(size_t) iLength + 4];

    // Doing the 'real' conversion
    iLength = MultiByteToWideChar(codePage
                                 ,0
                                 ,(LPCCH)p_buffer
                                 ,-1 // p_string.GetLength()
                                 ,reinterpret_cast<LPWSTR>(extraBuffer)
                                 ,iLength);
    if(iLength > 0)
    {
      // UTF-16 closing zero
      extraBuffer[iLength] = 0;
      p_string = extraBuffer;
      result   = true;
    }
    delete[] extraBuffer;
  }
  return result;
}

// Convert an UTF-16 string to a narrow string (utf-8)
// Allocates a buffer, caller is responsible for the destruction!
bool
TryCreateNarrowString(const XString& p_string
                     ,const XString  p_charset
                     ,const bool     p_doBom
                     ,      BYTE**   p_buffer
                     ,      int&     p_length)
{
  UINT codePage = GetACP(); // Default is to use the current codepage
  int  iLength  = -1;       // I think it will be null terminated
  int  extra    = 0;        // Extra space for a BOM
  bool result   = false;

  // Fill the code page names the first time
  InitCodePageNames();

  // Check if we know the codepage from the charset
  XString charset = p_charset.IsEmpty() ? _T("utf-8") : p_charset;
  NameCPIDMap::iterator it = cp_name_map.find(charset);
  if(it != cp_name_map.end())
  {
    codePage = it->second;
  }

  // Getting the length of the translation buffer first
  iLength = ::WideCharToMultiByte(codePage,
                                  0,
                                  p_string.GetString(),
                                  -1, // p_length, 
                                  NULL,
                                  0,
                                  NULL,
                                  NULL);
  // Convert the string if we find something
  if(iLength > 0)
  {
    iLength *= 2; // Funny but needed in most cases with 3-byte composite chars
    *p_buffer = new BYTE[iLength];
    memset(*p_buffer,0,iLength * sizeof(char));

    // Construct an UTF-8 BOM
    if(p_doBom && codePage == 65001)
    {
      extra = 3;
      *p_buffer[0] = (BYTE) 0xEF;
      *p_buffer[1] = (BYTE) 0xBB;
      *p_buffer[2] = (BYTE) 0xBF;
    }

    DWORD dwFlag = 0; // WC_COMPOSITECHECK | WC_DISCARDNS;
    iLength = ::WideCharToMultiByte(codePage,
                                    dwFlag,
                                    p_string.GetString(),
                                    -1, // p_length, 
                                    reinterpret_cast<LPSTR>(*p_buffer + extra),
                                    iLength,
                                    NULL,
                                    NULL);
    // Result!
    p_length = iLength > 0 ? iLength - 1: 0;
    result   = true;
  }
  return result;
}

// Implode an UTF-16 string to a BYTE buffer (for UTF-8 purposes)
// BEWARE: Cannot contain Multi-Lingual-Plane characters 
void 
ImplodeString(XString p_string,BYTE* p_buffer,unsigned p_length)
{
  for(int index = 0;index < p_string.GetLength(); ++index)
  {
    p_buffer[index] = (BYTE) p_string.GetAt(index);
  }
  p_buffer[p_length] = 0;
}

XString 
ExplodeString(BYTE* p_buffer,unsigned p_length)
{
  XString string;
  PWSTR buf = string.GetBufferSetLength(p_length + 1);
  for(unsigned index = 0;index < p_length; ++index)
  {
    *buf++ = (TCHAR)*p_buffer++;
  }
  *buf = (TCHAR) 0;
  string.ReleaseBufferSetLength(p_length);

  return string;
}

// in UNICODE settings these two functions are placeholders!
std::wstring
StringToWString(XString p_string)
{
  return std::wstring(p_string.GetString());
}

XString
WStringToString(std::wstring p_string)
{
  return XString(p_string.c_str());
}

// in UNICODE settings these two functions are placeholders!
// The real conversion is done by the server on reading / writing to the internet
XString
DecodeStringFromTheWire(XString p_string,XString p_charset /*="utf-8"*/,bool* p_foundBom /*=nullptr*/)
{
  int   length = p_string.GetLength();
  BYTE* buffer = new BYTE[length + 1];
  for(int ind = 0;ind < length; ++ind)
  {
    buffer[ind] = (uchar) p_string.GetAt(ind);
  }
  buffer[length] = 0;
  bool foundBom = false;
  XString result;
  if(TryConvertNarrowString(buffer,length,p_charset,result,foundBom))
  {
    result = p_string;
  }
  if(p_foundBom && foundBom)
  {
    *p_foundBom = true;
  }
  delete[] buffer;
  return result;
}

XString
EncodeStringForTheWire(XString p_string,XString p_charset /*="utf-8"*/)
{
  BYTE* buffer = nullptr;
  int length = 0;
  if(TryCreateNarrowString(p_string,p_charset,false,&buffer,length))
  {
    XString result;
    for(int ind = 0;ind < length; ++ind)
    {
      result += (TCHAR) buffer[ind];
    }
    delete[] buffer;
    return result;
  }
  return p_string;
}

// Construct a UTF-16 Byte-Order-Mark
XString ConstructBOMUTF16()
{
  // 2 BYTE UTF-16 Byte-order-Mark "\0xFE\0xFF"
  _TUCHAR bom[4];

  bom[0] = 0xFFFE;
  bom[1] = 0;
  return XString(bom);
}

// Convert directly from LPCSTR (No 'T' !!) to XString
// UNICODE   -> char* to wchar_t*
// ANSI/MBCS -> char* to char*
// Optionally convert to UTF8
XString
LPCSTRToString(LPCSTR p_string,bool p_utf8 /*= false*/)
{
  XString result;           // Nothing yet
  int    length = -1;       // I think it will be null terminated
  UINT codePage = p_utf8 ? 65001 : GetACP();
  DWORD   flags = p_utf8 ? 0     : MB_PRECOMPOSED;

  // Getting the length of the buffer, by specifying no output
  length = MultiByteToWideChar(codePage
                              ,flags
                              ,p_string
                              ,-1        // Null-terminated-string
                              ,NULL
                              ,0);
  if(length)
  {
    // Getting a UTF-16 wide-character buffer
    // +2 chars for a BOM, +2 chars for the closing zeros
    BYTE* buffer = new BYTE[2 * (size_t) length + 4];

    // Doing the 'real' conversion
    length = MultiByteToWideChar(codePage
                                ,flags
                                ,p_string
                                ,-1        // Null-terminated-string
                                ,reinterpret_cast<LPWSTR>(buffer)
                                ,length);
    if(length > 0)
    {
      // Buffer length is twice the number of logical characters
      length *= 2;
      // UTF-16 has two closing zeros
      buffer[length] = 0;
      buffer[length + 1] = 0;

      result = reinterpret_cast<LPWSTR>(buffer);
    }
    delete[] buffer;
  }
  return result;
}

// Convert a XString (here ANSI/MBCS) to a PCSTR buffer
// Allocates a buffer. Caller is responsible for destruction!
int
StringToLPCSTR(XString p_string,LPCSTR* p_buffer,int& p_size,bool p_utf8 /*= false*/)
{
  XString charset = p_utf8 ? _T("utf-8") : _T("");
  TryCreateNarrowString(p_string,charset,false,(BYTE**)p_buffer,p_size);
  return p_size;
}

#else

// Convert incoming buffer via UTF-16 to our MBCS format
bool
TryConvertWideString(const BYTE* p_buffer
                    ,int         p_length
                    ,XString     p_charset
                    ,XString&    p_string
                    ,bool&       p_foundBOM)
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

  if(reinterpret_cast<const BYTE*>(p_buffer)[p_length    ] != 0 &&
     reinterpret_cast<const BYTE*>(p_buffer)[p_length + 1] != 0)
  {
    // Unicode buffer not null terminated !!! 
    // Completely legal to get from the HTTP service.
    // So make it so, and expand with two (!!) extra null chars.
    extraBuffer = new BYTE[(size_t)p_length + 4];
    memcpy_s(extraBuffer,(size_t)p_length + 2,p_buffer,(size_t)p_length + 2);
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
  if(reinterpret_cast<const BYTE*>(p_buffer)[0] == 0xFF && 
     reinterpret_cast<const BYTE*>(p_buffer)[1] == 0xFE)
  {
    extra      = 2;     // Offset in buffer for conversion
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
                                      reinterpret_cast<LPSTR>(buffer), 
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
                   ,      BYTE**   p_buffer
                   ,      int&     p_length)
{
  bool   result = false;    // No yet
  UINT codePage = GetACP(); // Default the current codepage
  int   iLength = -1;       // I think it will be null terminated

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
                               ,0
                               ,p_string.GetString()
                               ,-1  // Convert including closing 0
                               ,NULL
                               ,0);
  if(iLength)
  {
    // Getting a UTF-16 wide-character buffer
    // +2 chars for a BOM
    *p_buffer = new BYTE[2*(size_t)iLength + 2];
    BYTE* buffer = (BYTE*) *p_buffer;

    // Construct an UTF-16 Byte-Order-Mark
    // In little endian order. OS/X calls this UTF-16LE
    if(p_doBom)
    {
      *buffer++ = 0xFF;
      *buffer++ = 0xFE;
    }

    // Doing the 'real' conversion
    iLength = MultiByteToWideChar(codePage
                                 ,0
                                 ,p_string.GetString()
                                 ,-1 // Including closing zero
                                 ,reinterpret_cast<LPWSTR>(buffer)
                                 ,iLength);
    if(iLength > 0)
    {
      // Buffer length is twice the number of logical characters
      iLength *= 2;
      // Result is OK, but 2 longer if we constructed a BOM or 2 shorter for zero's
      p_length = iLength - (p_doBom ? 0 : 2);
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

// Convert directly between XString and std::wstring
// Thus removing the need for the "USES_CONVERSION" macros
std::wstring 
StringToWString(XString p_string)
{
  std::wstring result;      // Nothing yet
  UINT codePage = GetACP(); // Default the current codepage
  int    length = -1;       // I think it will be null terminated

  // Getting the length of the buffer, by specifying no output
  length = MultiByteToWideChar(codePage
                              ,MB_PRECOMPOSED
                              ,p_string.GetString()
                              ,-1 // p_string.GetLength()
                              ,NULL
                              ,0);
  if(length)
  {
    // Getting a UTF-16 wide-character buffer
    // +2 chars for a BOM, +2 chars for the closing zeros
    BYTE* buffer = new BYTE[2*(size_t)length + 4];

    // Doing the 'real' conversion
    length = MultiByteToWideChar(codePage
                                ,MB_PRECOMPOSED
                                ,p_string.GetString()
                                ,-1 // p_string.GetLength()
                                ,reinterpret_cast<LPWSTR>(buffer)
                                ,length);
    if(length > 0)
    {
      // Buffer length is twice the number of logical characters
      length *= 2;
      // UTF-16 has two closing zeros
//       buffer[length    ] = 0;
//       buffer[length + 1] = 0;

      result = reinterpret_cast<LPWSTR>(buffer);
    }
    delete[] buffer;
  }
  return result;
}

XString
WStringToString(std::wstring p_string)
{
  UINT codePage = GetACP(); // Default is to use the current codepage
  int    length = -1;       // I think it will be null terminated
  XString result;

  // Getting the length of the translation buffer first
  length = ::WideCharToMultiByte(codePage,
                                  0,
                                  p_string.c_str(),
                                  -1, // p_length, 
                                  NULL,
                                  0,
                                  NULL,
                                  NULL);
  // Convert the string if we find something
  if(length > 0)
  {
    length *= 2; // Funny but needed in most cases with 3-byte composite chars
    char* buffer = new char[length];
    if(buffer != NULL)
    {
      DWORD dwFlag = 0; // WC_COMPOSITECHECK | WC_DISCARDNS;
      memset(buffer,0,length * sizeof(char));
      length = ::WideCharToMultiByte(codePage,
                                      dwFlag,
                                      p_string.c_str(),
                                      -1, // p_length, 
                                      reinterpret_cast<LPSTR>(buffer),
                                      length,
                                      NULL,
                                      NULL);
      if(length > 0)
      {
        result = buffer;
      }
      delete[] buffer;
    }
  }
  return result;
}

// Decoding incoming strings from the internet
// Defaults to UTF-8 encoding
XString
DecodeStringFromTheWire(XString p_string,XString p_charset /*="utf-8"*/,bool* p_foundBom /*=nullptr*/)
{
  // Check for empty character set
  if(p_charset.IsEmpty())
  {
    p_charset = _T("utf-8");
  }

  // Now decode the UTF-8 in the encoded string, to decoded MBCS
  BYTE*  buffer = nullptr;
  int    length = 0;
  if (TryCreateWideString(p_string,p_charset,false,&buffer,length))
  {
    XString decoded;
    bool foundBom = false;

    if(TryConvertWideString(buffer,length,"",decoded,foundBom))
    {
      p_string = decoded;
      if(foundBom && p_foundBom)
      {
        *p_foundBom = true;
      }
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
    p_charset = _T("utf-8");
  }

  // Now encode MBCS to UTF-8 without a BOM
  BYTE*  buffer = nullptr;
  int    length = 0;
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

// Implode an UTF-16 string to a MBCS XString
XString ImplodeString(BYTE* p_buffer,unsigned p_length)
{
  XString result;

  wchar_t* buf = (wchar_t*) p_buffer;
  unsigned tcharlen = p_length / 2;
  for(unsigned ind = 0;ind < tcharlen; ++ind)
  {
    result += (uchar) buf[ind];
  }
  return result;
}

// Convert an UTF-16 buffer to a XString in ANSI/MBCS mode
void ExplodeString(XString p_string,BYTE* p_buffer,unsigned p_length)
{
  if((unsigned)(p_string.GetLength() * 2 + 2) <= p_length)
  {
    for(int ind = 0;ind < p_string.GetLength(); ++ind)
    {
      *p_buffer++ = (BYTE) p_string.GetAt(ind);
      *p_buffer++ = 0;
    }
    *p_buffer++ = 0;
    *p_buffer++ = 0;
  }
}

// Construct a UTF-16 Byte-Order-Mark
std::wstring ConstructBOMUTF16()
{
  // 2 BYTE UTF-16 Byte-order-Mark "\0xFE\0xFF"
  BYTE bom[4];

  bom[0] = (BYTE) 0xFE;
  bom[1] = (BYTE) 0xFF;
  bom[2] = 0;
  bom[3] = 0;
  return std::wstring((LPCWSTR) bom);
}

// Convert directly from LPCSTR (No 'T' !!) to XString
// UNICODE   -> char* to wchar_t*
// ANSI/MBCS -> char* to char*
// Optionally convert to UTF8
XString
LPCSTRToString(LPCSTR p_string,bool p_utf8 /*= false*/)
{
  if(p_utf8)
  {
    return DecodeStringFromTheWire(p_string);
  }
  else
  {
    // No conversions
    return XString(p_string);
  }
}

// Convert a XString (here ANSI/MBCS) to a PCSTR buffer
int
StringToLPCSTR(XString p_string,LPCSTR* p_buffer,int& p_size,bool p_utf8 /*= false*/)
{
  XString string = p_utf8 ? EncodeStringForTheWire(p_string) : p_string;
  p_size   = string.GetLength();
  *p_buffer = new char[p_size + 1];
  strncpy_s((char*)*p_buffer,p_size + 1,string.GetString(),p_size + 1);
  return p_size;
}

#endif // UNICODE

// Scan for UTF-8 chars in a string
bool
DetectUTF8(XString& p_string)
{
  const BYTE* bytes = reinterpret_cast<const BYTE*>(p_string.GetString());
  return DetectUTF8(bytes);
}

bool
DetectUTF8(const BYTE* bytes)
{
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
