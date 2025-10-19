/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Encoding.h
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
#pragma once

// Result of the defuse process
enum class BOMOpenResult
{
  NoString      // Empty input
 ,NoEncoding    // No encoding Byte-order-mark found
 ,BOM           // Byte-Order-Mark
 ,Incompatible  // Incompatible (not supported) encoding
};

// All possible MS-Windows code pages defined as an enumeration
// Also see the table in 'ConvertWideString.cpp'
//
enum class Encoding
{
    EN_ACP            =     0       // Default ANSI code page
   ,EN_OEMCP          =     1       // Default OEM code page
   ,EN_MACCP          =     2       // Default MAC code page
   ,EN_THREAD         =     3       // Current thread's ANSI code page
   ,EN_IMB037         =    37       // IBM EBCDIC US-Canada
   ,EN_SYMBOL         =    42       // Symbol translations
   ,CP_IBM437         =   437       // OEM United States
   ,CP_IBM500         =   500       // IBM EBCDIC International
   ,CP_ASMO708        =   708       // Arabic (ASMO 708)
// ,CP_ASMO709        =   709       // No longer supported by MS-Windows: Arabic (ASMO-449+, BCON V4)
// ,CP_ASMO710        =   710       // No longer supported by MS-Windows: Arabic - Transparent-Arabic
   ,CP_DOS720         =   720       // Arabic(Transparent ASMO); Arabic (DOS)
   ,CP_IBM737         =   737       // OEM Greek (Formerly 437G); Greek (DOS)
   ,CP_IBM775         =   775       // OEM Baltic; Baltic (DOS)
   ,CP_IBM850         =   850       // OEM Multilingual Latin 1; Western European (DOS)
   ,CP_IBM852         =   852       // OEM Latin 2; Central European (DOS)
   ,CP_IBM855         =   855       // OEM Cyrillic (primarily Russian)
   ,CP_IBM857         =   857       // OEM Turkish; Turkish (DOS)
   ,CP_IBM00858       =   858       // OEM Multilingual Latin 1 + Euro symbol
   ,CP_IBM860         =   860       // OEM Portuguese; Portuguese (DOS)
   ,CP_IBM861         =   861       // OEM Icelandic; Icelandic (DOS)
   ,CP_IBM862         =   862       // OEM Hebrew; Hebrew (DOS)
   ,CP_IBM863         =   863       // OEM French Canadian; French Canadian (DOS)
   ,CP_IBM864         =   864       // OEM Arabic; Arabic (DOS)
   ,CP_IBM865         =   865       // OEM Nordic; Nordic (DOS)
   ,CP_CP866          =   866       // OEM Russian; Cyrillic (DOS)
   ,CP_IBM869         =   869       // OEM Modern Greek; Greek, Modern (DOS)
   ,CP_IBM870         =   870       // IBM EBCDIC Multilingual/ROECE (Latin 2); IBM EBCDIC Multilingual Latin 2
   ,CP_WINDOWS874     =   874       // ANSI/OEM Thai (same as 28605, ISO 8859-15); Thai (Windows)
   ,CP_CP875          =   875       // IBM EBCDIC Greek Modern
   ,CP_SHIFTJIS       =   932       // ANSI/OEM Japanese; Japanese (Shift-JIS)
   ,CP_GB2312         =   936       // ANSI/OEM Simplified Chinese (PRC, Singapore); Chinese Simplified (GB2312)
   ,CP_KSC5601        =   949       // ANSI/OEM Korean (Unified Hangul Code) 1987
   ,CP_BIG5           =   950       // ANSI/OEM Traditional Chinese (Taiwan; Hong Kong SAR, PRC); Chinese Traditional (Big5)
   ,CP_IBM1026        =  1026       // IBM EBCDIC Turkish (Latin 5)
   ,CP_IBM01047       =  1047       // IBM EBCDIC Latin 1/Open System
   ,CP_IBM01140       =  1140       // IBM EBCDIC US-Canada (037 + Euro symbol); IBM EBCDIC (US-Canada-Euro)
   ,CP_IBM01141       =  1141       // IBM EBCDIC Germany (20273 + Euro symbol); IBM EBCDIC (Germany-Euro)
   ,CP_IBM01142       =  1142       // IBM EBCDIC Denmark-Norway (20277 + Euro symbol); IBM EBCDIC (Denmark-Norway-Euro)
   ,CP_IBM01143       =  1143       // IBM EBCDIC Finland-Sweden (20278 + Euro symbol); IBM EBCDIC (Finland-Sweden-Euro)
   ,CP_IBM01144       =  1144       // IBM EBCDIC Italy (20280 + Euro symbol); IBM EBCDIC (Italy-Euro)
   ,CP_IBM01145       =  1145       // IBM EBCDIC Latin America-Spain (20284 + Euro symbol); IBM EBCDIC (Spain-Euro)
   ,CP_IBM01146       =  1146       // IBM EBCDIC United Kingdom (20285 + Euro symbol); IBM EBCDIC (UK-Euro)
   ,CP_IBM01147       =  1147       // IBM EBCDIC France (20297 + Euro symbol); IBM EBCDIC (France-Euro)
   ,CP_IBM01148       =  1148       // IBM EBCDIC International (500 + Euro symbol); IBM EBCDIC (International-Euro)
   ,CP_IBM01149       =  1149       // IBM EBCDIC Icelandic (20871 + Euro symbol); IBM EBCDIC (Icelandic-Euro)
    // Much used on MS-Windows
   ,LE_UTF16          =  1200       // "UTF-16": Little-Endian UTF 16 bits (Intel & MS-Windows!)
                                    // Unicode UTF-16, little endian byte order (BMP of ISO 10646)
   ,BE_UTF16          =  1201       // "UnicodeFFFE": Big-Endian order UTF 16 bits (Motorola & Apple MacIntosh)
                                    // Unicode UTF-16, big endian byte order
   ,CP_WIN1250        =  1250       // ANSI Central European; Central European (Windows)
   ,CP_WIN1251        =  1251       // ANSI Cyrillic; Cyrillic (Windows)
   ,CP_WIN1252        =  1252       // ANSI Latin 1; Western European (Windows)
   ,CP_WIN1253        =  1253       // ANSI Greek; Greek (Windows)
   ,CP_WIN1254        =  1254       // ANSI Turkish; Turkish (Windows)
   ,CP_WIN1255        =  1255       // ANSI Hebrew; Hebrew (Windows)
   ,CP_WIN1256        =  1256       // ANSI Arabic; Arabic (Windows)
   ,CP_WIN1257        =  1257       // ANSI Baltic; Baltic (Windows)
   ,CP_WIN1258        =  1258       // ANSI/OEM Vietnamese; Vietnamese (Windows)
    // END of much used on MS-Windows
   ,CP_JOHAB          =  1361       // Korean (Johab)
   ,CP_MACINTOSH      = 10000       // MAC Roman; Western European (Mac)
   ,CP_MACJAPANESE    = 10001       // Japanese (Mac)
   ,CP_MACCHINESETRAD = 10002       // MAC Traditional Chinese (Big5); Chinese Traditional (Mac)
   ,CP_MACKOREAN      = 10003       // Korean (Mac)
   ,CP_MACARABIC      = 10004       // Arabic (Mac)
   ,CP_MACHEBREW      = 10005       // Hebrew (Mac)
   ,CP_MACGREEK       = 10006       // Greek (Mac)
   ,CP_MACCYRILLIC    = 10007       // Cyrillic (Mac)
   ,CP_MACCHINESEIMP  = 10008       // MAC Simplified Chinese (GB 2312); Chinese Simplified (Mac)
   ,CP_MACROMANIAN    = 10010       // Romanian (Mac)
   ,CP_MACUKRANIAN    = 10017       // Ukranian (Mac)
   ,CP_MACTHAI        = 10021       // Thai (Mac)
   ,CP_MACCE          = 10029       // MAC Latin 2; Central European (Mac)
   ,CP_MACICELANDIC   = 10079       // Icelandic (Mac)
   ,CP_MACTURKISH     = 10081       // Turkish (Mac)
   ,CP_MACCROATION    = 10082       // Croatian (Mac)
    // To some extent used on the internet
   ,LE_UTF32          = 12000       // "UTF-32": Unicode UTF-32, little endian byte order
                                    // Little-Endian UTF 32 bits
   ,BE_UTF32          = 12001       // "UTF-32BE": Unicode UTF-32, big endian byte order
                                    // Big-Endian UTF 32 bits
    // END of used on the internet
   ,CP_CHINESECNS     = 20000       // CNS Taiwan; Chinese Traditional (CNS)
   ,CP_CP20001        = 20001       // TCA Taiwan
   ,CP_CHINESEETEN    = 20002       // Eten Taiwan; Chinese Traditional (Eten)
   ,CP_CP20003        = 20003       // IBM5550 Taiwan
   ,CP_CP20004        = 20004       // TeleText Taiwan
   ,CP_CP20005        = 20005       // Want Taiwan
   ,CP_IA5            = 20105       // IA5 (IRV International Alphabet No. 5, 7-bit); Western European (IA5)
   ,CP_IA5GERMAN      = 20106       // IA5 German (7-bit)
   ,CP_IA5SWEDISH     = 20107       // IA5 Swedish (7-bit)
   ,CP_IA5NORWEGIAN   = 20108       // IA5 Norwegian (7-bit)
    // STIL IN USE !!
   ,CP_USASCII        = 20127       // US-ASCII (7-bit)
    // END of still in use
   ,CP_CP20261        = 20261       // T.61
   ,CP_CP20269        = 20269       // ISO 6937 Non-Spacing Accent
   ,CP_IBM273         = 20273       // IBM EBCDIC Germany
   ,CP_IBM277         = 20277       // IBM EBCDIC Denmark-Norway
   ,CP_IBM278         = 20278       // IBM EBCDIC Finland-Sweden
   ,CP_IBM280         = 20280       // IBM EBCDIC Italy
   ,CP_IBM284         = 20284       // IBM EBCDIC Latin America-Spain
   ,CP_IBM285         = 20285       // IBM EBCDIC United Kingdom
   ,CP_IBM290         = 20290       // IBM EBCDIC Japanese Katakana Extended
   ,CP_IBM297         = 20297       // IBM EBCDIC France
   ,CP_IBM420         = 20420       // IBM EBCDIC Arabic
   ,CP_IBM423         = 20423       // IBM EBCDIC Greek
   ,CP_IBM424         = 20424       // IBM EBCDIC Hebrew
   ,CP_KOREANEXT      = 20833       // IBM EBCDIC Korean Extended
   ,CP_IBMTHAI        = 20838       // IBM EBCDIC Thai
   ,CP_KOI8R          = 20866       // Russian (KOI8-R); Cyrillic (KOI8-R)
   ,CP_IBM871         = 20871       // IBM EBCDIC Icelandic
   ,CP_IBM880         = 20880       // IBM EBCDIC Cyrillic Russian
   ,CP_IBM905         = 20905       // IBM EBCDIC Turkish
   ,CP_IBM924         = 20924       // IBM EBCDIC Latin 1/Open System (1047 + Euro symbol
   ,CP_EUCJP          = 20932       // Japanese (JIS 0208-1990 and 0121-1990)
   ,CP_CP20936        = 20936       // Simplified Chinese (GB2312); Chinese Simplified (GB2312-80)
   ,CP_CP20949        = 20949       // Korean Wansung
   ,CP_CP1025         = 21025       // IBM EBCDIC Cyrillic Serbian-Bulgarian
   ,CP_KOI8U          = 21866       // Ukrainian (KOI8-U); Cyrillic (KOI8-U)
    // Still widely in use in Europe
   ,ISO_8859_1        = 28591       // ISO 8859-1 Latin 1; Western European (ISO)
   ,ISO_8859_2        = 28592       // ISO 8859-2 Central European; Central European (ISO)
   ,ISO_8859_3        = 28593       // ISO 8859-3 Latin 3
   ,ISO_8859_4        = 28594       // ISO 8859-4 Baltic
   ,ISO_8859_5        = 28595       // ISO 8859-5 Cyrillic
   ,ISO_8859_6        = 28596       // ISO 8859-6 Arabic
   ,ISO_8859_7        = 28597       // ISO 8859-7 Greek
   ,ISO_8859_8        = 28598       // ISO 8859-8 Hebrew; Hebrew (ISO-Visual)
   ,ISO_8859_9        = 28599       // ISO 8859-9 Turkish
   ,ISO_8859_13       = 28603       // ISO 8859-13 Estonian
   ,ISO_8859_15       = 28605       // ISO 8859-15 Latin 9
   ,ISO_XEUROPA       = 29001       // Europa 3
   ,ISO_8859_8I       = 38598       // ISO 8859-8 Hebrew; Hebrew (ISO-Logical)
   // END of widely used in Europe
   ,ISO_2022          = 50220       // ISO 2022 Japanese with no halfwidth Katakana; Japanese (JIS)
   ,ISO_CS2022JP      = 50221       // ISO 2022 Japanese with halfwidth Katakana; Japanese (JIS-Allow 1 byte Kana)
   ,ISO_2022JP        = 50222       // ISO 2022 Japanese JIS X 0201-1989; Japanese (JIS-Allow 1 byte Kana - SO/SI)
   ,ISO_2022KR        = 50225       // ISO 2022 Korean
   ,ISO_CP50227       = 50227       // ISO 2022 Simplified Chinese; Chinese Simplified (ISO 2022)
// ,CP_50229          = 50229       // No longer supported by MS-Windows: ISO 2022 Traditional Chinese
// ,CP_50230          = 50230       // No longer supported by MS-Windows: EBCDIC Japanese (Katakana) Extended
// ,CP_50231          = 50231       // No longer supported by MS-Windows: EBCDIC US-Canada and Japanese
// ,CP_50233          = 50233       // No longer supported by MS-Windows: EBCDIC Korean Extended and Korean
// ,CP_50235          = 50235       // No longer supported by MS-Windows: EBCDIC Simplified Chinese Extended and Simplified Chinese
// ,CP_50236          = 50236       // No longer supported by MS-Windows: EBCDIC Simplified Chinese
// ,CP_50237          = 50237       // No longer supported by MS-Windows: EBCDIC US-Canada and Traditional Chinese
// ,CP_50239          = 50239       // No longer supported by MS-Windows: EBCDIC Japanese (Latin) Extended and Japanese
   ,CP_eucjp          = 51932       // EUC Japanese
   ,CP_EUCCN          = 51936       // EUC Simplified Chinese; Chinese Simplified (EUC)
   ,CP_EUCKR          = 51949       // EUC Korean
// ,CP_51950          = 51950       // No longer supported by MS-Windows: EUC Traditional Chinese
   ,CP_HZGB2312       = 52936       // HZ-GB2312 Simplified Chinese; Chinese Simplified (HZ)
   ,GB_18030          = 54936       // GB18030 Simplified Chinese (4 byte); Chinese Simplified (GB18030)
                                    // Chinese government: Guojia Biazhun coding standard 18030
   ,GB_ISCII_DE       = 57002       // ISCII Devanagari
   ,GB_ISCII_BE       = 57003       // ISCII Bengali
   ,GB_ISCII_TA       = 57004       // ISCII Tamil
   ,GB_ISCII_TE       = 57005       // ISCII Telugu
   ,GB_ISCII_AS       = 57006       // ISCII Assamese
   ,GB_ISCII_OR       = 57007       // ISCII Oriya
   ,GB_ISCII_KA       = 57008       // ISCII Kannada
   ,GB_ISCII_MA       = 57009       // ISCII Malayalam
   ,GB_ISCII_GU       = 57010       // ISCII Gujarati
   ,GB_ISCII_PA       = 57011       // ISCII Punjabi
   ,UTF7              = 65000       // UTF 7 bits (Obsolete variable length encoding)
    // MOST USED STANDARD ON THE INTERNET !!
   ,UTF8              = 65001       // UTF 8 bits (General WEB standard)
};

//////////////////////////////////////////////////////////////////////////
//
// Most used encodings on the internet (statistics by Google/ChatGTP)
//
// 1) UTF-8
// 2) UTF-16
// 3) ISO-8859-1
// 4) US-ASCII
// 
// Less used encodings on the internet
// 
// 5) ISO-8859-*   (2,5,6,7 and 8)
// 6) Windows-125* (Windows-1252 + 0,1,2 and 6)
// 7) GB2312, GBK, GB18030
// 8) Big5
// 9) Shift-JIS, EUC-JP, ISO-2022-JP
// 10) KOI8-R and KOI8-U
// 11) ISO-2022-*
// 12) UTF-32
//

// Default encoding (opening files etc)
#ifdef _UNICODE
#define EncodingDefault Encoding::LE_UTF16
#else
#define EncodingDefault Encoding::EN_ACP
#endif
