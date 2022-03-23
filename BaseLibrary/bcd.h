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
using uchar  = unsigned char;
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
bcd floor(bcd p_number);
bcd ceil (bcd p_number);
bcd fabs (bcd p_number);
bcd sqrt (bcd p_number);
bcd log10(bcd p_number);
bcd log  (bcd p_number);
bcd exp  (bcd p_number);
bcd pow  (bcd p_number,bcd  p_power);
bcd frexp(bcd p_number,int* p_exponent);
bcd ldexp(bcd p_number,int  p_power);
bcd modf (bcd p_number,bcd* p_intpart);
bcd fmod (bcd p_number,bcd  p_divisor);

// Overloaded standard trigonometric functions on a bcd number
bcd sin  (bcd p_number);
bcd cos  (bcd p_number);
bcd tan  (bcd p_number);
bcd asin (bcd p_number);
bcd acos (bcd p_number);
bcd atan (bcd p_number);
bcd atan2(bcd p_y,bcd p_x);

// One-time initialization for printing numbers in the current locale
void InitValutaString();

// string format number and money format functions
extern bool g_locale_valutaInit;
extern char g_locale_decimalSep[];
extern char g_locale_thousandSep[];
extern char g_locale_strCurrency[];
extern int  g_locale_decimalSepLen;
extern int  g_locale_thousandSepLen;
extern int  g_locale_strCurrencyLen;

//////////////////////////////////////////////////////////////////////////
//
// The Binary Coded Decimal class
//
//////////////////////////////////////////////////////////////////////////

class bcd
{
public:
  enum class Sign     { Positive,    Negative    };
  enum class Format   { Engineering, Bookkeeping };
  enum class Operator { Addition,    Subtraction };

  // CONSTRUCTORS/DESTRUCTORS

  // Default constructor.
  bcd();

  // BCD from a char value
  bcd(const char p_value);

  // BCD from an unsigned char value
  bcd(const unsigned char p_value);

  // BCD from a short value
  bcd(const short p_value);

  // BCD from an unsigned short value
  bcd(const unsigned short p_value);

  // BCD from an integer
  bcd(const int p_value);

  // BCD from an unsigned integer
  bcd(const unsigned int p_value);

  // BCD from a long
  bcd(const long p_value, const long p_restValue = 0);

  // BCD from an unsigned long
  bcd(const unsigned long p_value, const unsigned long p_restValue = 0);

  // BCD from a 64bits int
  bcd(const int64 p_value,const int64 p_restvalue = 0);

  // BCD from an unsigned 64bits int
  bcd(const uint64 p_value,const int64 p_restvalue = 0);

  // Copy constructor.
  bcd(const bcd& icd);

  // BCD from a float
  bcd(const float p_value);

  // BCD from a double
  bcd(const double p_value);

  // BCD From a character string
  bcd(const char* p_string,bool p_fromDB = false);

  // BCD from a SQL_NUMERIC_STRUCT
  bcd(const SQL_NUMERIC_STRUCT* p_numeric);

  // CONSTANTS

  static bcd PI();     // Circumference/Radius ratio of a circle
  static bcd LN2();    // Natural logarithm of 2
  static bcd LN10();   // Natural logarithm of 10

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

  const bcd  operator+(const char*  p_value) const;
  const bcd  operator-(const char*  p_value) const;
  const bcd  operator*(const char*  p_value) const;
  const bcd  operator/(const char*  p_value) const;
  const bcd  operator%(const char*  p_value) const;

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

  bcd& operator+=(const char*  p_value);
  bcd& operator-=(const char*  p_value);
  bcd& operator*=(const char*  p_value);
  bcd& operator/=(const char*  p_value);
  bcd& operator%=(const char*  p_value);

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
  bcd& operator=(const char*   p_value);
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

  bool operator==(const char*  p_value) const;
  bool operator!=(const char*  p_value) const;
  bool operator< (const char*  p_value) const;
  bool operator> (const char*  p_value) const;
  bool operator<=(const char*  p_value) const;
  bool operator>=(const char*  p_value) const;

  // MAKING AN EXACT NUMERIC value
  
  // Set the mantissa/exponent/sign to the number zero (0)
  void    Zero();
  // Round to a specified fraction (decimals behind the .)
  void    Round(int p_precision = 0);
  // Truncate to a specified fraction (decimals behind the .)
  void    Truncate(int p_precision = 0);  
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
  bcd     Reciproke() const;
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
  bcd     ArcTangent2Points(bcd p_x) const;

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

  // Is bcd exactly 0.0?
  bool    IsNull() const; 
  // Is bcd nearly 0.0 (smaller than epsilon)
  bool    IsNearZero();
  // Gets the sign 0 (= 0.0), 1 (greater than 0) of -1 (smaller than 0)
  int     GetSign() const;
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
  XString DebugPrint(char* p_name);
#endif

private:

  // INTERNALS

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
  void    SetValueString(const char* p_string,bool p_fromDB = false);
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
  long    StringToLong(const char* p_string) const;
  // Convert a long to a string
  XString LongToString(long p_value) const;
  // Split the mantissa for floor/ceiling operations
  bcd     SplitMantissa() const;
  // Compare two mantissa
  int     CompareMantissa(const bcd& p_value) const;
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
  Sign   m_sign;                // 0 = Positive, 1 = Negative
  short  m_exponent;            // +/- 10E32767
  long   m_mantissa[bcdLength]; // Up to (bcdDigits * bcdLength) digits
};
