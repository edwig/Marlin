/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: bcd.h
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
//////////////////////////////////////////////////////////////////////////
//
// BCD
//
// Floating Point Precision Number class (Binary Coded Decimal)
// A number always has the format [sign][[digit]*][.[digit]*][E[sign][digits]+] where sign is either '+' or '-'
// Numbers are stored in 1E8 based mantissa with a digital . implied at the second position
// The mantissa array exists of a series of integers with 8 functional digits each
//
// Copyright (c) 2014-2022 ir W. E. Huisman
// Version 1.5 of 03-01-2022
//
#pragma once
#include <intsafe.h>    // Min/Max sizes of integer datatypes
#include <sqltypes.h>   // Needed for conversions of SQL_NUMERIC_STRUCT

// The ODBC standard has a maximum of 38 decimal places
// At least one database (Oracle) implements these numbers
#ifndef SQLNUM_MAX_PREC
#define SQLNUM_MAX_PREC 38
#endif

// Constants that controls the actual precision:
const int bcdBase      = 100000000L; // Base of the numbers in m_mantissa
const int bcdDigits    = 8;          // Number of digits in one mantissa element
const int bcdLength    = 5;          // Number of elements in the mantissa    
const int bcdPrecision = bcdDigits * bcdLength;
// When rethinking one of these four constants, 
// be sure to edit the following points in the implementation class
// - The constants:   PI, LN2, LN10
// - The conversions: AsLong, AsInt64, AsNumeric
// - The setters:     SetValueLong, SetValueInt64, SetValueNumeric
// - Some generals:   DebugPrint ("%08d")

// Handy typedefs of used basic datatypes
using ushort = unsigned short;
using ulong  = unsigned long;

#ifndef int64
using int64  = __int64;
#endif

#ifndef uint64
using uint64 = unsigned __int64;
#endif

// Forward declaration of our class
class bcd;

// Overloaded standard mathematical functions
bcd floor(const bcd& p_number);
bcd ceil (const bcd& p_number);
bcd fabs (const bcd& p_number);
bcd sqrt (const bcd& p_number);
bcd log10(const bcd& p_number);
bcd log  (const bcd& p_number);
bcd exp  (const bcd& p_number);
bcd pow  (const bcd& p_number,const bcd& p_power);
bcd frexp(const bcd& p_number,int* p_exponent);
bcd ldexp(const bcd& p_number,int  p_power);
bcd modf (const bcd& p_number,bcd* p_intpart);
bcd fmod (const bcd& p_number,const bcd& p_divisor);

// Overloaded standard trigonometric functions on a bcd number
bcd sin  (const bcd& p_number);
bcd cos  (const bcd& p_number);
bcd tan  (const bcd& p_number);
bcd asin (const bcd& p_number);
bcd acos (const bcd& p_number);
bcd atan (const bcd& p_number);
bcd atan2(const bcd& p_y,const bcd& p_x);

// One-time initialization for printing numbers in the current locale
void InitValutaString();

// string format number and money format functions
extern bool  g_locale_valutaInit;
extern TCHAR g_locale_decimalSep[];
extern TCHAR g_locale_thousandSep[];
extern TCHAR g_locale_strCurrency[];
extern int   g_locale_decimalSepLen;
extern int   g_locale_thousandSepLen;
extern int   g_locale_strCurrencyLen;

//////////////////////////////////////////////////////////////////////////
//
// The Binary Coded Decimal class
//
//////////////////////////////////////////////////////////////////////////

class bcd
{
public:
  // CONSTRUCTORS/DESTRUCTORS

  // Default constructor.
  bcd();

  // Copy constructor.
  bcd(const bcd& icd);

  // BCD from a char value
  explicit bcd(const TCHAR p_value);

#ifndef UNICODE
  // BCD from an unsigned char value
  explicit bcd(const _TUCHAR p_value);
#endif
  // BCD from a short value
  explicit bcd(const short p_value);

  // BCD from an unsigned short value
  explicit bcd(const unsigned short p_value);

  // BCD from an integer
  explicit bcd(const int p_value);

  // BCD from an unsigned integer
  explicit bcd(const unsigned int p_value);

  // BCD from a long
  explicit bcd(const long p_value, const long p_restValue = 0);

  // BCD from an unsigned long
  explicit bcd(const unsigned long p_value, const unsigned long p_restValue = 0);

  // BCD from a 64bits int
  explicit bcd(const int64 p_value,const int64 p_restvalue = 0);

  // BCD from an unsigned 64bits int
  explicit bcd(const uint64 p_value,const int64 p_restvalue = 0);

  // BCD from a float
  explicit bcd(const float p_value);

  // BCD from a double
  explicit bcd(const double p_value);

  // BCD From a character string
  explicit bcd(PCTSTR p_string,bool p_fromDB = false);

  // BCD from a SQL_NUMERIC_STRUCT
  explicit bcd(const SQL_NUMERIC_STRUCT* p_numeric);

  // ENUMERATIONS

  // Keep sign status in this order!
  enum class Sign
  {
     Positive  // bcd number >= 0
    ,Negative  // bcd number  < 0
    ,ISNULL    // bcd number is NULL in the database
    ,MIN_INF   // bcd number in negative infinity
    ,INF       // bcd number in positive infinity
    ,NaN       // Not a Number (e.g. a string)
  };
  // Formatting of a string (As<X>String())
  enum class Format
  {
     Engineering
    ,Bookkeeping
  };
  // For internal processing
  enum class Operator
  {
     Addition
    ,Subtraction
  };

  // BCD constructs as a NULL from the database
  explicit bcd(const bcd::Sign p_sign);

  // CONSTANTS

  static bcd PI();     // Circumference/Radius ratio of a circle
  static bcd LN2();    // Natural logarithm of 2
  static bcd LN10();   // Natural logarithm of 10

  // ERROR HANDLING

  // BCD throws on error or sets status (-INF, INF, NAN)
  // BEWARE: Not thread safe to change in flight
  // Applications must use ONE (1) setting at startup
  static void ErrorThrows(bool p_throws = true);

  // OPERATORS

  // Standard mathematical operators
  const bcd  operator+(const bcd&   p_value) const;
  const bcd  operator-(const bcd&   p_value) const;
  const bcd  operator*(const bcd&   p_value) const;
  const bcd  operator/(const bcd&   p_value) const;
  const bcd  operator%(const bcd&   p_value) const;

  const bcd  operator+(const int    p_value) const;
  const bcd  operator-(const int    p_value) const;
  const bcd  operator*(const int    p_value) const;
  const bcd  operator/(const int    p_value) const;
  const bcd  operator%(const int    p_value) const;

  const bcd  operator+(const double p_value) const;
  const bcd  operator-(const double p_value) const;
  const bcd  operator*(const double p_value) const;
  const bcd  operator/(const double p_value) const;
  const bcd  operator%(const double p_value) const;

  const bcd  operator+(LPCTSTR p_value) const;
  const bcd  operator-(LPCTSTR p_value) const;
  const bcd  operator*(LPCTSTR p_value) const;
  const bcd  operator/(LPCTSTR p_value) const;
  const bcd  operator%(LPCTSTR p_value) const;

  // Standard math/assignment operators
  bcd& operator+=(const bcd& p_value);
  bcd& operator-=(const bcd& p_value);
  bcd& operator*=(const bcd& p_value);
  bcd& operator/=(const bcd& p_value);
  bcd& operator%=(const bcd& p_value);

  bcd& operator+=(const int p_value);
  bcd& operator-=(const int p_value);
  bcd& operator*=(const int p_value);
  bcd& operator/=(const int p_value);
  bcd& operator%=(const int p_value);

  bcd& operator+=(const double p_value);
  bcd& operator-=(const double p_value);
  bcd& operator*=(const double p_value);
  bcd& operator/=(const double p_value);
  bcd& operator%=(const double p_value);

  bcd& operator+=(LPCTSTR p_value);
  bcd& operator-=(LPCTSTR p_value);
  bcd& operator*=(LPCTSTR p_value);
  bcd& operator/=(LPCTSTR p_value);
  bcd& operator%=(LPCTSTR p_value);

  // Prefix unary minus (negation)
  bcd  operator-() const;

  // Prefix/Postfix increment/decrement operators
  bcd  operator++(int);  // Postfix increment
  bcd& operator++();     // Prefix  increment
  bcd  operator--(int);  // Postfix decrement
  bcd& operator--();     // Prefix  decrement

  // Assignment operators
  bcd& operator=(const bcd&    p_value);
  bcd& operator=(const int     p_value);
  bcd& operator=(const double  p_value);
  bcd& operator=(const PCTSTR  p_value);
  bcd& operator=(const __int64 p_value);

  // comparison operators
  bool operator==(const bcd&   p_value) const;
  bool operator!=(const bcd&   p_value) const;
  bool operator< (const bcd&   p_value) const;
  bool operator> (const bcd&   p_value) const;
  bool operator<=(const bcd&   p_value) const;
  bool operator>=(const bcd&   p_value) const;

  bool operator==(const int    p_value) const;
  bool operator!=(const int    p_value) const;
  bool operator< (const int    p_value) const;
  bool operator> (const int    p_value) const;
  bool operator<=(const int    p_value) const;
  bool operator>=(const int    p_value) const;

  bool operator==(const double p_value) const;
  bool operator!=(const double p_value) const;
  bool operator< (const double p_value) const;
  bool operator> (const double p_value) const;
  bool operator<=(const double p_value) const;
  bool operator>=(const double p_value) const;

  bool operator==(LPCTSTR p_value) const;
  bool operator!=(LPCTSTR p_value) const;
  bool operator< (LPCTSTR p_value) const;
  bool operator> (LPCTSTR p_value) const;
  bool operator<=(LPCTSTR p_value) const;
  bool operator>=(LPCTSTR p_value) const;

  // MAKING AN EXACT NUMERIC value
  
  // Set the mantissa/exponent/sign to the number zero (0)
  void    Zero();
  // Set to database NULL
  void    SetNULL();
  // Round to a specified fraction (decimals behind the .)
  void    Round(int p_precision = 0);
  // Truncate to a specified fraction (decimals behind the .)
  void    Truncate(int p_precision = 0);  
  // Change length and precision
  void    SetLengthAndPrecision(int p_precision = bcdPrecision,int p_scale = (bcdPrecision / 2));
  // Change the sign
  void    Negate();
  
  // MATHEMATICAL FUNCTIONS

  // Value before the decimal point
  bcd     Floor() const;
  // Value behind the decimal point
  bcd     Fraction() const;
  // Value after the decimal point
  bcd     Ceiling() const;
  // Square root of the bcd
  bcd     SquareRoot() const;
  // This bcd to the power x
  bcd     Power(const bcd& p_power) const;
  // Absolute value (ABS)
  bcd     AbsoluteValue() const;
  // Reciproke / Inverse = 1/x
  bcd     Reciprocal() const;
  // Natural logarithm
  bcd     Log() const;
  // Exponent e tot the power 'this number'
  bcd     Exp() const;
  // Log with base 10
  bcd     Log10() const;
  // Ten Power
  bcd     TenPower(int n);

  // TRIGONOMETRIC FUNCTIONS

  // Sinus of the angle
  bcd     Sine() const;
  // Cosine of the angle
  bcd     Cosine() const;
  // Tangent of the angle
  bcd     Tangent() const;
  // Arc sines (angle) of the ratio
  bcd     ArcSine() const;
  // Arc cosine (angle) of the ratio
  bcd     ArcCosine() const;
  // Arctangent (angle) of the ratio
  bcd     ArcTangent() const;
  // Angle of two points (x,y)
  bcd     ArcTangent2Points(const bcd& p_x) const;

  // GET AS SOMETHING DIFFERENT

  // Get as a double
  double  AsDouble() const;
  // Get as a short
  short   AsShort() const;
  // Get as an unsigned short
  ushort  AsUShort() const;
  // Get as a long
  long    AsLong() const;
  // Get as an unsigned long
  ulong   AsULong() const;
  // Get as a 64bits long
  int64   AsInt64() const;
  // Get as an unsigned 64 bits long
  uint64  AsUInt64() const;
  // Get as a mathematical string
  XString AsString(bcd::Format p_format = Format::Bookkeeping,bool p_printPositive = false,int p_decimals = 2) const;
  // Get as a display string (by desktop locale)
  XString AsDisplayString(int p_decimals = 2) const;
  // Get as an ODBC SQL NUMERIC(p,s)
  void    AsNumeric(SQL_NUMERIC_STRUCT* p_numeric) const;

  // GETTER FUNCTIES

  // Is bcd exactly 0.0? (used to be called IsNull)
  bool    IsZero() const; 
  // Is bcd a database NULL
  bool    IsNULL() const;
  // Not an (-)INF or a NAN
  bool    IsValid() const;
  // Is bcd nearly 0.0 (smaller than epsilon)
  bool    IsNearZero();
  // Gets the sign 0 (= 0.0), 1 (greater than 0) of -1 (smaller than 0)
  int     GetSign() const;
  // Gets Signed status Positive, Negative, -INF, INF, NaN
  Sign    GetStatus() const;
  // Total length (before and after decimal point)
  int     GetLength() const;
  // Total precision (length after the decimal point)
  int     GetPrecision() const;
  // Get the max size of a bcd
  static int GetMaxSize(int p_precision = 0);
  // Gets the fact that it fits in a long
  bool    GetFitsInLong() const;
  // Gets the fact that it fits in an int64
  bool    GetFitsInInt64() const;
  // Decimal part (behind the decimal point) is not "000" (zeros)
  bool    GetHasDecimals() const;
  // Gets the exponent
  int     GetExponent() const;
  // Gets the mantissa
  bcd     GetMantissa() const;

  // FILE STREAM FUNCTIONS
  bool    WriteToFile (FILE* p_fp);
  bool    ReadFromFile(FILE* p_fp);

#ifdef _DEBUG
  // Debug print of the mantissa
  XString DebugPrint(PTCHAR p_name);
#endif

private:

  // INTERNALS

  // Set infinity for overflows
  bcd     SetInfinity(XString p_reason = _T("")) const;
  // Sets one integer in this bcd number
  void    SetValueInt(const int p_value);
  // Sets one or two longs in this bcd number
  void    SetValueLong(const long p_value, const long p_restValue);
  // Sets one or two longs in this bcd number
  void    SetValueInt64 (const  int64 p_value,const int64 p_restValue);
  void    SetValueUInt64(const uint64 p_value,const int64 p_restValue);
  // Sets the value from a double
  void    SetValueDouble(const double p_value);
  // Sets the value from a string
  void    SetValueString(LPCTSTR p_string,bool p_fromDB = false);
  // Sets the value from a SQL NUMERIC
  void    SetValueNumeric(const SQL_NUMERIC_STRUCT* p_numeric);
  // Take the absolute value of a long
  long    long_abs(const long p_value) const;
  // Normalize the mantissa/exponent
  void    Normalize(int p_startExponent = 0);
  // Multiply the mantissa by 10
  void    Mult10(int p_times = 1);
  // Divide the mantissa by 10
  void    Div10(int p_times = 1);
  // Shift mantissa 1 position right
  void    ShiftRight();
  // Shift mantissa 1 position left
  void    ShiftLeft();
  // Convert a string to a single long value
  long    StringToLong(LPCTSTR p_string) const;
  // Convert a long to a string
  XString LongToString(long p_value) const;
  // Split the mantissa for floor/ceiling operations
  bcd     SplitMantissa() const;
  // Compare two mantissa
  int     CompareMantissa(const bcd& p_value) const;
  // Calculate the precision and scale for a SQL_NUMERIC
  void    CalculatePrecisionAndScale(SQLCHAR& p_precision,SQLCHAR& p_scale) const;
  // Stopping criterion for internal iterations
  bcd&    Epsilon(long p_fraction) const;

  // BASIC OPERATIONS

  // Addition operation
  bcd Add(const bcd& p_number) const;
  // Subtraction operation
  bcd Sub(const bcd& p_number) const;
  // Multiplication
  bcd Mul(const bcd& p_number) const;
  // Division
  bcd Div(const bcd& p_number) const;
  // Modulo
  bcd Mod(const bcd& p_number) const;

  // Helpers for the basic operations

  // Position arguments and signs for the next operation
  void PositionArguments(bcd& arg1,bcd& arg2,Sign& signResult,Operator& operatorKind) const;
  // Calculate the sign for multiplication or division
  Sign CalculateSign(const bcd& p_arg1, const bcd& p_arg2) const;
  // Addition of two mantissa (no signs/exponents)
  bcd  PositiveAddition(bcd& arg1,bcd& arg2) const;
  // Subtraction of two mantissa (no signs/exponents)
  bcd  PositiveSubtraction(bcd& arg1,bcd& arg2) const;
  // Multiplication of two mantissa (no signs)
  bcd  PositiveMultiplication(const bcd& p_arg1,const bcd& p_arg2) const;
  // Division of two mantissa (no signs)
  bcd  PositiveDivision(bcd& p_arg1,bcd& p_arg2) const;

  // STORAGE OF THE NUMBER
  Sign          m_sign;                // 0 = Positive, 1 = Negative (INF, NaN)
  short         m_exponent;            // +/- 10E32767
  long          m_mantissa[bcdLength]; // Up to (bcdDigits * bcdLength) digits
};
