/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: bcd.cpp
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
//////////////////////////////////////////////////////////////////////////
//
//  BCD
// 
//  Floating Point Precision Number class (Binary Coded Decimal)
//  An Arbitrary float always has the format [sign][digit][.[digit]*][E[sign][digits]+] 
//  where sign is either '+' or '-' or is missing ('+' implied)
//  And is always stored in normalized mode after an operation or conversion
//  with an implied decimal point between the first and second position
//
// Copyright (c) 2014-2022 ir W. E. Huisman
// Version 1.5 of 03-01-2022
//
//  Examples:
//  E+03 15456712 45000000 00000000 -> 1545.671245
//  E+01 34125600 00000000 00000000 -> 3.41256
//  E-05 78976543 12388770 00000000 -> 0.0000789765431238877
//
//////////////////////////////////////////////////////////////////////////

#include "pch.h"            // Precompiled headers
#include "bcd.h"            // OUR INTERFACE
#include "StdException.h"   // Exceptions
#include <math.h>           // Still needed for conversions of double
#include <locale.h>
#include <winnls.h>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

// Theoretical maximum of numerical separators
#define SEP_LEN 10

// string format number and money format functions
bool  g_locale_valutaInit = false;
TCHAR g_locale_decimalSep [SEP_LEN + 1];
TCHAR g_locale_thousandSep[SEP_LEN + 1];
TCHAR g_locale_strCurrency[SEP_LEN + 1];
int   g_locale_decimalSepLen   = 0;
int   g_locale_thousandSepLen  = 0;
int   g_locale_strCurrencyLen  = 0;

// Error handling throws or we silently return -INF, INF, NaN
bool g_throwing = true;

// One-time initialization for printing numbers in the current locale
void 
InitValutaString()
{
  if(g_locale_valutaInit == false)
  {
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,  g_locale_decimalSep, SEP_LEN);
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, g_locale_thousandSep,SEP_LEN);
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SCURRENCY, g_locale_strCurrency,SEP_LEN);
    g_locale_decimalSepLen  = (int)_tclen(g_locale_decimalSep);
    g_locale_thousandSepLen = (int)_tclen(g_locale_thousandSep);
    g_locale_strCurrencyLen = (int)_tclen(g_locale_strCurrency);

    g_locale_valutaInit = true;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// CONSTRUCTORS OF BCD
//
//////////////////////////////////////////////////////////////////////////

// bcd::bcd
// Description: Default constructor
// Technical:   Initialize the number at zero (0)
bcd::bcd()
{
  Zero();
}

// bcd::bcd(bcd& arg)
// Description: Copy constructor of a bcd
// Technical:   Copies all data members
bcd::bcd(const bcd& p_arg)
{
  m_sign      = p_arg.m_sign;
  m_exponent  = p_arg.m_exponent;
  // Create and copy mantissa
  memcpy(m_mantissa,p_arg.m_mantissa,bcdLength * sizeof(long));
}

// bcd::bcd(value)
// Description: BCD from a char value
bcd::bcd(const TCHAR p_value)
{
  SetValueInt((int)p_value);
}

#ifndef UNICODE
// bcd::bcd(value)
// Description: BCD from an unsigned char value
bcd::bcd(const _TUCHAR p_value)
{
  SetValueInt((int)p_value);
}
#endif

// bcd::bcd(value)
// Description: BCD from a short value
// 
bcd::bcd(const short p_value)
{
  SetValueInt((int)p_value);
}

// bcd::bcd(value)
// Description: BCD from an unsigned short value
//
bcd::bcd(const unsigned short p_value)
{
  SetValueInt((int)p_value);
}

// bcd::bcd(value)
// BCD from an integer
bcd::bcd(const int p_value)
{
  SetValueInt(p_value);
}

// bcd::bcd(value)
// BCD from an unsigned integer
bcd::bcd(const unsigned int p_value)
{
  SetValueInt64((int64)p_value,0);
}

// bcd::bcd(value,value)
// Description: Construct a BCD from a long and an option long
// Technical:   See description of SetValueLong
//
bcd::bcd(const long p_value, const long p_restValue /*= 0*/)
{
  SetValueLong(p_value,p_restValue);
}

// bcd::bcd(value,value)
// Description: Construct a BCD from an unsigned long and an unsigned optional long
// Technical:   See description of SetValueLong
bcd::bcd(const unsigned long p_value, const unsigned long p_restValue /*= 0*/)
{
  SetValueInt64((int64)p_value,(int64)p_restValue);
}

// bcd::bcd(value,value)
// Description: Construct a BCD from a 64bit long and an optional long
// Technical:   See description of SetValueInt64
//
bcd::bcd(const int64 p_value,const int64 p_restvalue /*= 0*/)
{
  SetValueInt64(p_value,p_restvalue);
}

bcd::bcd(const uint64 p_value,const int64 p_restvalue)
{
  SetValueUInt64(p_value,p_restvalue);
}

// bcd::bcd(float)
// Description: Construct a bcd from a float
// 
bcd::bcd(const float p_value)
{
  SetValueDouble((double)p_value);
}

// bcd::bcd(double)
// Description: Construct a bcd from a double
// 
bcd::bcd(const double p_value)
{
  SetValueDouble(p_value);
}

// BCD From a character string
// Description: Assignment-constructor from an elementary character data pointer
// Parameters:  p_string -> Input character pointer (containing a number)
//              p_fromDB -> Input comes from a  database (always American format)
bcd::bcd(LPCTSTR p_string,bool p_fromDB /*= false*/)
{
  SetValueString(p_string,p_fromDB);
}

// bcd::bcd(numeric)
// Description: Assignment-constructor for bcd from a SQL NUMERIC
// Parameters:  p_numeric -> Input from a SQL ODBC database
//                           from a NUMERIC field
//
bcd::bcd(const SQL_NUMERIC_STRUCT* p_numeric)
{
  SetValueNumeric(p_numeric);
}

// bcd::bcd(Sign)
// Description: Construct a bcd from a NULL in the database
// Parameters:  p_sign : BUT GETS IGNORED!!
//
bcd::bcd(const bcd::Sign /*p_sign*/)
{
  Zero();
  // We ignore the argument!!
  m_sign = Sign::ISNULL;
}

//////////////////////////////////////////////////////////////////////////
//
// END OF CONSTRUCTORS OF BCD
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// CONSTANTS OF BCD
//
//////////////////////////////////////////////////////////////////////////

// bcd::PI
// Description: Circumference/Radius ratio of a circle
// Technical:   Nature constant that never changes
bcd
bcd::PI()
{
  bcd pi;

  // PI in 120 decimals: 
  // +31415926_53589793_23846264_33832795_02884197
  //  16939937 51058209 74944592 30781640 62862089 
  //  98628034 82534211 70679821 48086513 28230665
  pi.m_mantissa[0] = 31415926L;
  pi.m_mantissa[1] = 53589793L;
  pi.m_mantissa[2] = 23846264L;
  pi.m_mantissa[3] = 33832795L;
  pi.m_mantissa[4] =  2884197L;

  return pi;
}

// bcd::LN2
// Description: Natural logarithm of two
// Technical:  Mathematical constant that never changes
bcd
bcd::LN2()
{
  bcd ln2;

  // LN2 in 120 decimals: Use if you expand bcdLength
  // +0.69314718_05599453_09417232_12145817_65680755
  //    00134360 25525412 06800094 93393621 96969471 
  //    56058633 26996418 68754200 14810205 70685714
  ln2.m_exponent = -1;
  ln2.m_mantissa[0] = 69314718L;
  ln2.m_mantissa[1] =  5599453L;
  ln2.m_mantissa[2] =  9417232L;
  ln2.m_mantissa[3] = 12145817L;
  ln2.m_mantissa[4] = 65680755L;

  return ln2;
}

// bcd::LN10
// Description: Natural logarithm of ten
// Technical:   Mathematical constant that never changes
bcd
bcd::LN10()
{
  bcd ln10;

  // LN10 in 120 decimals: Use if you expand bcdLength
  // +2.3025850_92994045_68401799_14546843_64207601
  //   10148862 87729760 33327900 96757260 96773524 
  //   80235997 20508959 82983419 67784042 28624865
  ln10.m_mantissa[0] = 23025850L;
  ln10.m_mantissa[1] = 92994045L;
  ln10.m_mantissa[2] = 68401799L;
  ln10.m_mantissa[3] = 14546843L;
  ln10.m_mantissa[4] = 64207601L;

  return ln10;
}

 // Maximum number a bcd can hold
bcd
bcd::MIN_BCD()
{
  bcd min = MAX_BCD();
  // Turn the sign around
  min.m_sign = Sign::Negative;

  return min;
}

// Minimum number a bcd can hold
bcd
bcd::MAX_BCD()
{
  bcd max;

  max.m_sign        = Sign::Positive;
  max.m_mantissa[0] = 99999999L;
  max.m_mantissa[1] = 99999999L;
  max.m_mantissa[2] = 99999999L;
  max.m_mantissa[3] = 99999999L;
  max.m_mantissa[4] = 99999999L;
  max.m_exponent    = SHRT_MAX;

  return max;
}

// Smallest number a bcd can hold
bcd
bcd::MIN_EPSILON()
{
  bcd eps;

  eps.m_sign        = Sign::Positive;
  eps.m_mantissa[0] = 1L;
  eps.m_exponent    = SHRT_MIN;

  return eps;
}

//////////////////////////////////////////////////////////////////////////
//
// END OF CONSTANTS OF BCD
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// ERROR HANDLING
//
//////////////////////////////////////////////////////////////////////////

/*static */ void 
bcd::ErrorThrows(bool p_throws /*= true*/)
{
  g_throwing = p_throws;
}

//////////////////////////////////////////////////////////////////////////
//
// OPERATORS OF BCD
//
//////////////////////////////////////////////////////////////////////////

// bcd::+
// Description: Addition operator
//
const bcd
bcd::operator+(const bcd& p_value) const
{
  return Add(p_value);
}

const bcd
bcd::operator+(const int p_value) const
{
  return Add(bcd(p_value));
}

const bcd
bcd::operator+(const double p_value) const
{
  return Add(bcd(p_value));
}

const bcd
bcd::operator+(LPCTSTR p_value) const
{
  return Add(bcd(p_value));
}

// bcd::-
// Description: Subtraction operator
const bcd
bcd::operator-(const bcd& p_value) const
{
  return Sub(p_value);
}

const bcd
bcd::operator-(const int p_value) const
{
  return Sub(bcd(p_value));
}

const bcd
bcd::operator-(const double p_value) const
{
  return Sub(bcd(p_value));
}

const bcd
bcd::operator-(LPCTSTR p_value) const
{
  return Sub(bcd(p_value));
}

// bcd::*
// Description: Multiplication operator
const bcd
bcd::operator*(const bcd& p_value) const
{
  return Mul(p_value);
}

const bcd
bcd::operator*(const int p_value) const
{
  return Mul(bcd(p_value));
}

const bcd
bcd::operator*(const double p_value) const
{
  return Mul(bcd(p_value));
}

const bcd
bcd::operator*(LPCTSTR p_value) const
{
  return Mul(bcd(p_value));
}

// bcd::/
// Description: Division operator
const bcd
bcd::operator/(const bcd& p_value) const
{
  return Div(p_value);
}

const bcd
bcd::operator/(const int p_value) const
{
  return Div(bcd(p_value));
}

const bcd
bcd::operator/(const double p_value) const
{
  return Div(bcd(p_value));
}

const bcd
bcd::operator/(LPCTSTR p_value) const
{
  return Div(bcd(p_value));
}

// bcd::%
// Description: Modulo operator
const bcd
bcd::operator%(const bcd& p_value) const
{
  return Mod(p_value);
}

const bcd  
bcd::operator%(const int p_value) const
{
  return Mod(bcd(p_value));
}

const bcd
bcd::operator%(const double p_value) const
{
  return Mod(bcd(p_value));
}

const bcd
bcd::operator%(LPCTSTR p_value) const
{
  return Mod(bcd(p_value));
}

// bcd::operator +=
// Description: Operator to add a bcd to this one
bcd&
bcd::operator+=(const bcd& p_value)
{
  *this = Add(p_value);
  return *this;
}

bcd&
bcd::operator+=(const int p_value)
{
  *this = Add(bcd(p_value));
  return *this;
}

bcd&
bcd::operator+=(const double p_value)
{
  *this = Add(bcd(p_value));
  return *this;
}

bcd&
bcd::operator+=(LPCTSTR p_value)
{
  *this = Add(bcd(p_value));
  return *this;
}

// bcd::operator -=
// Description: Operator to subtract a bcd from this one
bcd&
bcd::operator-=(const bcd& p_value)
{
  *this = Sub(p_value);
  return *this;
}

bcd& 
bcd::operator-=(const int p_value)
{
  *this = Sub(bcd(p_value));
  return *this;
}

bcd&
bcd::operator-=(const double p_value)
{
  *this = Sub(bcd(p_value));
  return *this;
}

bcd&
bcd::operator-=(LPCTSTR p_value)
{
  *this = Sub(bcd(p_value));
  return *this;
}

// bcd::operator *=
// Description: Operator to multiply a bcd with this one
bcd& 
bcd::operator*=(const bcd& p_value)
{
  *this = Mul(p_value);
  return *this;
}

bcd&
bcd::operator*=(const int p_value)
{
  *this = Mul(bcd(p_value));
  return *this;
}

bcd&
bcd::operator*=(const double p_value)
{
  *this = Mul(bcd(p_value));
  return *this;
}

bcd&
bcd::operator*=(LPCTSTR p_value)
{
  *this = Mul(bcd(p_value));
  return *this;
}

// bcd::operator /=
// Description: Operator to divide a bcd with another
bcd& 
bcd::operator/=(const bcd& p_value)
{
  *this = Div(p_value);
  return *this;
}

bcd&
bcd::operator/=(const int p_value)
{
  *this = Div(bcd(p_value));
  return *this;
}

bcd&
bcd::operator/=(const double p_value)
{
  *this = Div(bcd(p_value));
  return *this;
}

bcd&
bcd::operator/=(LPCTSTR p_value)
{
  *this = Div(bcd(p_value));
  return *this;
}

// bcd::operator %=
// Description: Operator to do a modulo on this one
bcd& 
bcd::operator%=(const bcd& p_value)
{
  *this = Mod(p_value);
  return *this;
}

bcd&
bcd::operator%=(const int p_value)
{
  *this = Mod(bcd(p_value));
  return *this;
}

bcd&
bcd::operator%=(const double p_value)
{
  *this = Mod(bcd(p_value));
  return *this;
}

bcd&
bcd::operator%=(LPCTSTR p_value)
{
  *this = Mod(bcd(p_value));
  return *this;
}

// bd::-
// Description: prefix unary minus (negation)
//
bcd  
bcd::operator-() const
{
  bcd result(*this);

  // Null can never be negative
  if(!result.IsZero() && result.IsValid() && !result.IsNULL())
  {
    // Swap signs
    if(result.m_sign == Sign::Positive)
    {
      result.m_sign = Sign::Negative;
    }
    else
    {
      result.m_sign = Sign::Positive;
    }
  }
  return result;
}

// bcd::postfix ++
//
bcd 
bcd::operator++(int)
{
  // Return result first, than do the add 1
  bcd res(*this);
  ++*this;
  return res;
}

// bcd::prefix++
bcd& 
bcd::operator++()
{
  //++x is equal to x+=1
  bcd number_1(1);
  *this += number_1;
  return *this;
}

// bcd::Postfix decrement
//
bcd 
bcd::operator--(int)
{
  // Return result first, than do the subtract
  bcd res(*this);
  --*this;
  return res;
}

// bcd::Prefix  decrement
bcd& 
bcd::operator--()
{
  // --x is equal to x-=1
  bcd number_1(1);
  *this -= number_1;
  return *this;
}

// bcd::=
// Description: Assignment operator from another bcd
bcd& 
bcd::operator=(const bcd& p_value)
{
  if(this != &p_value)
  {
    m_sign      = p_value.m_sign;
    m_exponent  = p_value.m_exponent;
    memcpy(m_mantissa,p_value.m_mantissa,bcdLength * sizeof(long));
  }
  return *this;
}

// bcd::=
// Description: Assignment operator from a long
bcd& 
bcd::operator=(const int p_value)
{
  SetValueLong(p_value,0);
  return *this;
}

// bcd::=
// Description: Assignment operator from a double
bcd& 
bcd::operator=(const double p_value)
{
  SetValueDouble(p_value);
  return *this;
}

// bcd::=
// Description: Assignment operator from a string
bcd& 
bcd::operator=(const PCTSTR p_value)
{
  SetValueString(p_value);
  return *this;
}

// bcd::=
// Description: Assignment operator from an __int64
bcd&
bcd::operator=(const __int64 p_value)
{
  SetValueInt64(p_value,0);
  return *this;
}

// bcd::operator==
// Description: Equality comparison of two bcd numbers
//
bool 
bcd::operator==(const bcd& p_value) const
{
  // Shortcut: the same number is equal to itself
  if(this == &p_value)
  {
    return true;
  }
  // Signs must be equal
  if(m_sign != p_value.m_sign)
  {
    return false;
  }
  // Exponents must be equal
  if(m_exponent != p_value.m_exponent)
  {
    return false;
  }
  // Mantissa's must be equal
  for(int ind = 0;ind < bcdLength; ++ind)
  {
    if(m_mantissa[ind] != p_value.m_mantissa[ind])
    {
      return false;
    }
  }
  // Everything is equal
  return true;
}

bool
bcd::operator==(const int p_value) const
{
  bcd value(p_value);
  return *this == value;
}

bool
bcd::operator==(const double p_value) const
{
  bcd value(p_value);
  return *this == value;
}

bool
bcd::operator==(LPCTSTR p_value) const
{
  bcd value(p_value);
  return *this == value;
}

// bcd::operator!=
// Description: Inequality comparison of two bcd numbers
//
bool 
bcd::operator!=(const bcd& p_value) const
{
  // (x != y) is equal to !(x == y)
  return !(*this == p_value);
}

bool
bcd::operator!=(const int p_value) const
{
  bcd value(p_value);
  return !(*this == value);
}

bool
bcd::operator!=(const double p_value) const
{
  bcd value(p_value);
  return !(*this == value);
}

bool
bcd::operator!=(LPCTSTR p_value) const
{
  bcd value(p_value);
  return !(*this == value);
}

bool
bcd::operator<(const bcd& p_value) const
{
  // Check if we can do a comparison
  // Infinity compares to nothing!!
  if(!IsValid() || !p_value.IsValid() || IsNULL() || p_value.IsNULL())
  {
    return false;
  }

  // Shortcut: Negative numbers are smaller than positive ones
  if(m_sign != p_value.m_sign)
  {
    // Signs are not equal 
    // If this one is negative "smaller than" is true
    // If this one is positive "smaller than" is false
    return (m_sign == Sign::Negative);
  }

  // Issue #2 at github
  // Zero is always smaller than everything else
  if(IsZero() && !p_value.IsZero())
  {
    return (m_sign == Sign::Positive);
  }

  // Shortcut: If the exponent differ, the mantissa's don't matter
  if(m_exponent != p_value.m_exponent)
  {
    if(m_exponent < p_value.m_exponent)
    {
      return (m_sign == Sign::Positive);
    }
    else
    {
      return (m_sign == Sign::Negative);
    }
  }

  // Signs are the same and exponents are the same
  // Now compare the mantissa
  for(int ind = 0;ind < bcdLength; ++ind)
  {
    // Find the first position not equal to the other
    if(m_mantissa[ind] != p_value.m_mantissa[ind])
    {
      // Result by comparing the mantissa positions
      if(m_sign == Sign::Positive)
      {
        return (m_mantissa[ind] < p_value.m_mantissa[ind]);
      }
      else
      {
        return (m_mantissa[ind] > p_value.m_mantissa[ind]);
      }
    }
  }
  // Numbers are exactly the same
  return false;
}

bool
bcd::operator<(const int p_value) const
{
  bcd value(p_value);
  return *this < value;
}

bool
bcd::operator<(const double p_value) const
{
  bcd value(p_value);
  return *this < value;
}

bool
bcd::operator<(LPCTSTR p_value) const
{
  bcd value(p_value);
  return *this < value;
}

bool
bcd::operator>(const bcd& p_value) const
{
  // Check if we can do a comparison
  // Infinity compares to nothing!!
  if(!IsValid() || !p_value.IsValid() || IsNULL() || p_value.IsNULL())
  {
    return false;
  }

  // Shortcut: Negative numbers are smaller than positive ones
  if(m_sign != p_value.m_sign)
  {
    // Signs are not equal. 
    // If this one is positive "greater than" is true
    // If this one is negative "greater than" is false
    return (m_sign == Sign::Positive);
  }
  // Shortcut: if value is zero
  if(IsZero())
  {
    return (p_value.m_sign == Sign::Negative);
  }
  // Shortcut: If the exponent differ, the mantissa's don't matter
  if(m_exponent != p_value.m_exponent)
  {
    if(m_exponent > p_value.m_exponent || p_value.IsZero())
    {
      return (m_sign == Sign::Positive);
    }
    else
    {
      return (m_sign == Sign::Negative);
    }
  }
  // Signs are the same and exponents are the same
  // Now compare the mantissa
  for(int ind = 0;ind < bcdLength; ++ind)
  {
    // Find the first position not equal to the other
    if(m_mantissa[ind] != p_value.m_mantissa[ind])
    {
      // Result by comparing the mantissa positions
      if(m_sign == Sign::Positive)
      {
        return (m_mantissa[ind] > p_value.m_mantissa[ind]);
      }
      else
      {
        return (m_mantissa[ind] < p_value.m_mantissa[ind]);
      }
    }
  }
  // Numbers are exactly the same
  return false;
}

bool
bcd::operator>(const int p_value) const
{
  bcd value(p_value);
  return *this > value;
}

bool
bcd::operator>(const double p_value) const
{
  bcd value(p_value);
  return *this > value;
}

bool
bcd::operator>(LPCTSTR p_value) const
{
  bcd value(p_value);
  return *this > value;
}

bool
bcd::operator<=(const bcd& p_value) const
{
  // (x <= y) equals !(x > y)
  return !(*this > p_value);
}

bool
bcd::operator<=(const int p_value) const
{
  bcd value(p_value);
  return !(*this > value);
}

bool
bcd::operator<=(const double p_value) const
{
  bcd value(p_value);
  return !(*this > value);
}

bool
bcd::operator<=(LPCTSTR p_value) const
{
  bcd value(p_value);
  return !(*this > value);
}

bool
bcd::operator>=(const bcd& p_value) const
{
  // (x >= y) equals !(x < y)
  return !(*this < p_value);
}

bool
bcd::operator>=(const int p_value) const
{
  bcd value(p_value);
  return !(*this < value);
}

bool
bcd::operator>=(const double p_value) const
{
  bcd value(p_value);
  return !(*this < value);
}

bool
bcd::operator>=(LPCTSTR p_value) const
{
  bcd value(p_value);
  return !(*this < value);
}

//////////////////////////////////////////////////////////////////////////
//
// END OF OPERATORS OF BCD
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// MAKING AN EXACT NUMERIC
//
//////////////////////////////////////////////////////////////////////////

// bcd::Zero
// Description: Make empty
// Technical:   Set the mantissa/exponent/sign to the number zero (0)

void
bcd::Zero()
{
  m_sign = Sign::Positive;
  m_exponent = 0;
  memset(m_mantissa,0,bcdLength * sizeof(long));
}

// Set to database NULL
void
bcd::SetNULL()
{
  Zero();
  m_sign = Sign::ISNULL;
}

// Round to a specified fraction (decimals behind the .)
void     
bcd::Round(int p_precision /*=0*/)
{
  // Check if we can do a round
  if(!IsValid() || IsNULL())
  {
    return;
  }

  // Precision is dependent on the exponent
  int precision = p_precision + m_exponent;

  // Quick optimization
  if(precision < 0)
  {
    Zero();
    return;
  }
  if(precision > bcdPrecision)
  {
    // Nothing to be done
    return;
  }

  // Walk the mantissa
  int mant = precision / bcdDigits;
  int pos  = precision % bcdDigits;

  int mm = m_mantissa[mant];

  if(pos == (bcdDigits - 1))
  {
    // Last position in the mantissa part
    if(mant == (bcdLength - 1))
    {
      // Nothing to do. No rounding possible
      return;
    }
    else
    {
      int mn = m_mantissa[mant + 1];
      int digitNext  = mn / (bcdBase / 10);

      if(digitNext >= 5)
      {
        // The rounding in optimized form
        m_mantissa[mant] = ++mm;
      }
    }
  }
  else
  {
    // In-between Optimalization
    int base = bcdBase;
    // Calculate base
    for(int p2 = 0;p2 <= pos; ++p2)
    {
      base /= 10;
    }
    // Rounding this digit
    int mantBefore = mm / base;
    // Next rounding digit
    int digitNext = mm % base;
    digitNext /= (base / 10);

    // Rounding
    if(digitNext >= 5)
    {
      // Calculate new mantissa
      ++mantBefore;
      int newMant = mantBefore * base;
      if(pos >= 0)
      {
        m_mantissa[mant] = newMant;
      }
    }
    // Strip everything in the mantissa behind the position
    m_mantissa[mant] /= base;
    m_mantissa[mant] *= base;
  }

  // Strip the higher mantissa's
  for(int m1 = mant + 1;m1 < bcdLength; ++m1)
  {
    m_mantissa[m1] = 0;
  }
  Normalize();
}

// Truncate to a specified fraction (decimals behind the .)
void
bcd::Truncate(int p_precision /*=0*/)
{
  // Check if we can do a truncate
  if(!IsValid() || IsNULL())
  {
    return;
  }

  // Precision is dependent on the exponent
  int precision = p_precision + m_exponent;

  // Quick optimization
  if(precision <= 0)
  {
    // Number totally truncated
    Zero();
    return;
  }
  if(precision > bcdPrecision)
  {
    // Nothing to truncate
    return;
  }

  // Walk the mantissa
  int mant = precision / bcdDigits;
  int pos  = precision % bcdDigits;

  // Strip everything in the mantissa behind the position
  if(pos < (bcdDigits - 1))
  {
    // Calculate base
    int base = bcdBase;
    for(int p2 = 0;p2 <= pos; ++p2)
    {
      base /= 10;
    }

    m_mantissa[mant] /= base;
    m_mantissa[mant] *= base;
  }

  // Strip the higher mantissa's
  if(mant < (bcdLength - 1))
  {
    for(int m1 = mant + 1;m1 < bcdLength; ++m1)
    {
      m_mantissa[m1] = 0;
    }
  }
}

// Change the sign
void
bcd::Negate()
{
  // Check if we can do a negation
  if(!IsValid() || IsNULL())
  {
    return;
  }

  if(IsZero())
  {
    m_sign = Sign::Positive;
  }
  else
  {
    m_sign = (m_sign == Sign::Positive) ? Sign::Negative : Sign::Positive;
  }
}

// Change length and precision
void
bcd::SetLengthAndPrecision(int p_precision /*= bcdPrecision*/,int p_scale /*= (bcdPrecision / 2)*/)
{
  // Check if we can set these
  if(!IsValid() || IsNULL())
  {
    return;
  }

  if(IsZero())
  {
    // Optimize for NULL situation
    return;
  }

  if(m_exponent > p_precision)
  {
    XString error;
    error.Format(_T("Overflow in BCD at set precision and scale as NUMERIC(%d,%d)"),p_precision,p_scale);
    *this = SetInfinity(error);
    return;
  }

  // Calculate the mantissa position to truncate
  int mantpos = m_exponent + p_scale + 1;

  // Truncating to zero?
  if(mantpos <= 0)
  {
    m_exponent = 0;
    memset(m_mantissa,0,sizeof(long) * bcdLength);
    return;
  }
  int mant = mantpos / bcdDigits;
  int mpos = mantpos % bcdDigits;

  // Strip this on mantissa part
  if(mpos)
  {
    static const int significations[] = {1,10000000,1000000,100000,10000,1000,100,10,1};
    int  significant = significations[mpos];
    int64       accu = m_mantissa[mant] / significant;
    m_mantissa[mant] = (long) (accu * significant);
  }
  // Strip the rest of the mantissa
  for(int index = mant + 1;index < bcdLength; ++index)
  {
    m_mantissa[index] = 0;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// END OF MAKING AN EXACT NUMERIC
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// MATHEMATICAL FUNCTIONS FOR BCD
//
//////////////////////////////////////////////////////////////////////////

// bcd::Floor
// Value before the decimal point
bcd
bcd::Floor() const
{
  bcd result;
  bcd minusOne(-1L);

  // Check if we can do a floor
  if(IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  if(!IsValid())
  {
    return SetInfinity(_T("BCD: Infinity doesn't have a floor"));
  }

  // Shortcut: If number is null. Floor is always zero
  if(IsZero())
  {
    return result;
  }
  // Shortcut: if number is a fraction: floor is always zero
  if(m_exponent < 0)
  {
    return m_sign == Sign::Positive ? result : minusOne;
  }
  // Shortcut: If number is too big, it's just this number
  if(m_exponent > bcdDigits * bcdLength)
  {
    return *this;
  }
  // Find the floor
  result = SplitMantissa();

  // Take care of sign
  if(m_sign == Sign::Negative)
  {
    // Floor is 1 smaller
    result -= bcd(1L);
  }
  return result;
}

// bcd::Fraction
// Description: Value behind the decimal point
bcd     
bcd::Fraction() const
{
  return (*this) - Floor();
}

// Value after the decimal point
bcd     
bcd::Ceiling() const
{
  bcd result;
  bcd one(1);

  // Check if we can do a ceiling
  if(IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  if(!IsValid())
  {
    return SetInfinity(_T("BCD: Infinity does not have a ceiling."));
  }

  // Shortcut: If number is null. Ceiling is always zero
  if(IsZero())
  {
    return result;
  }
  // Shortcut: if number is a fraction: ceiling is one or zero
  if(m_exponent < 0)
  {
    return m_sign == Sign::Positive ? one : result;
  }
  // Shortcut: If number is too big, it's just this number
  if(m_exponent > bcdDigits * bcdLength)
  {
    return *this;
  }
  // Find the floor
  result = SplitMantissa();

  // Take care of sign
  if(m_sign == Sign::Positive)
  {
    // Floor is 1 smaller
    result += one;
  }
  return result;
}

#ifndef ALT_SQUAREROOT

// bcd::SquareRoot
// Description: Do the square root of the bcd
// Technical:   Do first approximation by sqrt(double)
//              Then use Newton's equation


bcd
bcd::SquareRoot() const
{
  bcd number;
  bcd half(_T("0.5"));
  bcd two(2);
  bcd three(3);

  // Check if we can do this
  if(IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  if(!IsValid())
  {
    return SetInfinity(_T("BCD: Infinity does not have a square root!"));
  }

  // Optimalization sqrt(0) = 0
  if(IsZero())
  {
    return number;
  }
  // Tolerance criterion epsilon
  bcd epsilon = Epsilon(10);

  number = *this; // Number to get the root from
  if(number.GetSign() == -1)
  {
    return SetInfinity(_T("BCD: Cannot get a square root from a negative number."));
  }
  // Reduction by dividing through square of a whole number
  // for speed a power of two
  bcd reduction(1L);
  bcd hunderd(100L);
  while(number / (reduction * reduction) > hunderd)
  {
    reduction *= two;
  }
  // Reduce by dividing by the square of the reduction
  // (reduction is really sqrt(reduction)
  number /= (reduction * reduction);

  // First quick guess
  double approximation1 = number.AsDouble();
  double approximation2 = 1 / ::sqrt(approximation1);
  bcd    result(approximation2);
  bcd    between;

  // Newton's iteration
  // U(n) = U(3-VU^2)/2
  while(true)
  {
    between  = number * result * result;  // VU^2
    between  = three - between;           // 3-VU^2
    between *= half;                      // (3-VU^2)/2

    if(between.Fraction() < epsilon)
    {
      break;
    }
    result *= between;
  }
  // End result from number * 1/sqrt
  result *= number;
  // Reapply reduction by multiplying to the result
  result *= reduction;

  return result;
}

#else

// Description: Square root of the number
// What it does: Does an estimation through a double and then
// Uses the Newton estimation to calculate the root
bcd
bcd::SquareRoot() const
{
  bcd number(0L, 0L);
  bcd half(_T("0.5"));
  bcd two(2L);
  bcd three(3L);
  int sqrti = 0;

  // Optimization: sqrt(0) = 0
  if (IsNULL())
  {
    return number;
  }

  // Getting the breaking criterion
  bcd epsilon = Epsilon(10);

  number = *this; // Number to get the square-root from
  if (number.GetSign() == -1)
  {
    return SetInfinity(_T("BCD: Infinity does not have a square root!"));
  }
  // First estimate
  double estimate1 = number.AsDouble() / 2;
  double estimate2 = 1 / sqrt(estimate1);
  bcd result(estimate1);
  bcd between;

  // Newton's iteration
  bcd last_result(_T("0.0"));

  while (true)
  {
    result = (result + number / result) / two;
    between = last_result - result;
    if (between.AbsoluteValue() < epsilon)
    {
      break;
    }

    last_result = result;
  }
  return result;
}
#endif

// bcd::Power
// Description: Get BCD number to a power
// Technical:   x^y = exp(y * ln(x))
bcd
bcd::Power(const bcd& p_power) const
{
  bcd result;

  // Check if we can do this
  if(IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  if(!IsValid())
  {
    return SetInfinity(_T("BCD: Can not take a power of infinity!"));
  }

  result = this->Log() * p_power;
  result = result.Exp();

  return result;
}

// bcd::AbsoluteValue
// Description: Return the absolute value
bcd
bcd::AbsoluteValue() const
{
  // Check if we can do this
  if(IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  if(!IsValid())
  {
    return SetInfinity(_T("BCD: Can change the sign of infinity!"));
  }
  bcd result(*this);
  result.m_sign = Sign::Positive;
  return result;
}

// bcd::Reciprocal
// Description: Reciprocal / Inverse = 1/x
bcd     
bcd::Reciprocal() const
{
  // Check if we can do this
  if(IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  if(!IsValid())
  {
    return SetInfinity(_T("BCD: Can do the reciprocal of infinity!"));
  }
  bcd result = bcd(1) / *this;
  return result;
}

// bcd::Log
// Description: Natural logarithm
// Technical:   Use a Taylor series until their is no more change in the result
//              Equivalent with the same standard C function call
//              ln(x) == 2( z + z^3/3 + z^5/5 ...
//              z = (x-1)/(x+1)
bcd     
bcd::Log() const
{
  long k;
  long expo = 0;
  bcd res, number, z2;
  bcd number10(10L);
  bcd fast(_T("1.2"));
  bcd one(1L);
  bcd epsilon = Epsilon(5);

  // Check if we can do this
  if(IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  if((GetSign() == -1) || !IsValid())
  { 
    return SetInfinity(_T("BCD: Cannot calculate a natural logarithm of a number <= 0"));
  }
  // Bring number under 10 and save exponent
  number = *this;
  while(number > number10)
  {
    number /= number10;
    ++expo;
  }
  // In order to get a fast Taylor series result we need to get the fraction closer to one
  // The fraction part is [1.xxx-9.999] (base 10) OR [1.xxx-255.xxx] (base 256) at this point
  // Repeat a series of square root until 'number' < 1.2
  for(k = 0; number > fast; k++)
  {
    number = number.SquareRoot();
  }
  // Calculate the fraction part now at [1.xxx-1.1999]
  number = (number - one) / (number + one);
  z2     = number * number;
  res    = number;
  // Iterate using Taylor series ln(x) == 2( z + z^3/3 + z^5/5 ... )
  bcd between;
  for(long stap = 3; ;stap += 2)
  {
    number *= z2;
    between = number / bcd(stap);
    // Tolerance criterion
    if(between.AbsoluteValue() < epsilon)
    {
      break;
    }
    res += between;
  }
  // Re-add powers of two (comes from  " < 1.2")
  res *= bcd(::pow(2.0,(double)++k));

  // Re-apply the exponent
  if(expo != 0)
  {
    // Ln(x^y) = Ln(x) + Ln(10^y) = Ln(x) + y * ln(10)
    res += bcd(expo) * bcd::LN10();
  }
  return res;
}

// bcd::exp
// Description: Exponent e tot the power 'this number'
// Technical:   Use a Taylor series until their is no more change in the result
//              exp(x) == 1 + x + x^2/2!+x^3/3!+....
//              Equivalent with the same standard C function call
//
bcd
bcd::Exp() const
{
  long step, k = 0;
  bcd between, result, number;
  bcd half(_T("0.5"));
  bcd epsilon = Epsilon(5);

  // Check if we can do this
  if(IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  if(!IsValid())
  {
    return SetInfinity(_T("BCD: Cannot take the exponent of infinity!"));
  }
  number = *this;

  // Can not calculate: will always be one!
  if(number.IsZero())
  {
    return bcd(1);
  }

  if(number.GetSign () < 0 )
  {
    number = -number;;
  }
  for( k = 0; number > half; )
  {
    long expo = number.GetExponent();
    if( expo > 0 )
    {
      step   = 3 * min( 10, expo );  // 2^3
      result = bcd((long) (1 << step) );
      result = result.Reciprocal();
      k += step;
    }
    else
    {
      result = half;
      k++;
    }

    number *= result;
  }

  // Do first two iterations
  result  = bcd(1L) + number;
  between  = number * number * half;
  result += between;
  // Now iterate 
  for(step = 3; ;step++)
  {
    between *= number / bcd(step);
    // Tolerance criterion
    if(between < epsilon)
    {
      break;
    }
    result += between;
  }
  // Re-add powers of myself
  for( ; k > 0; k-- )
  {
    result *= result;
  }
  // Take care of the sign
  if(this->GetSign() < 0 )
  {
    result = bcd(1) / result;
  }
  return result;
}

// bcd::Log10
// Description: Logarithm in base 10
// Technical:   log10 = ln(x) / ln(10);
bcd     
bcd::Log10() const
{
  bcd res;

  // Check if we can do a LOG10
  if(IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  if(!IsValid())
  {
    return SetInfinity(_T("BCD: Cannot take the Log10 of infinity!"));
  }
  if(GetSign() <= 0) 
  { 
    return SetInfinity(_T("BCD: Cannot get a 10-logarithm of a number <= 0"));
  }
  res = *this;
  res = res.Log() / LN10();

  return res;
}

// Ten Power
// Description: bcd . 10^n
// Technical:   add n to the exponent of the number
bcd
bcd::TenPower(int n)
{
  // Check if we can do this
  if(IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  if(!IsValid())
  {
    return SetInfinity(_T("BCD: Cannot take the 10th power of infinity!"));
  }
  bcd res = *this;
  res.m_exponent += (short)n;
  res.Normalize();
  
  return res;
}

//////////////////////////////////////////////////////////////////////////
//
// END OF MATHEMATICAL FUNCTIONS FOR BCD
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// BCD TRIGONOMETRIC FUNCTIONS
//
//////////////////////////////////////////////////////////////////////////

// bcd::Sine
// Description: Sine of the angle
// Technical:   Use the Taylor series: Sin(x) = x - x^3/3! + x^5/5! ...
//              1) Reduce x to between 0..2*PI 
//              2) Reduce further until x between 0..PI by means of sin(x+PI) = -Sin(x)
//              3) Then do the Taylor expansion series
// Reduction is needed to speed up the Taylor expansion and reduce rounding errors
//
bcd
bcd::Sine() const
{
  int sign;
  bcd number;
  bcd pi,pi2;
  bcd between;
  bcd epsilon = Epsilon(3);

  // Check if we can do this
  if(IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  if(!IsValid())
  {
    return SetInfinity(_T("BCD: Cannot take the sine of infinity!"));
  }

  number = *this;
  sign = number.GetSign();
  if( sign < 0 )
  {
    number = -number;
  }
  // Reduce the argument until it is between 0..2PI 
  pi2 = PI() * bcd(2);
  if(number > pi2)
  {
    between = number / pi2; 
    between = between.Floor();
    number -= between * pi2;
  }
  if(number < bcd(0))
  {
    number += pi2;
  }
  // Reduce further until it is between 0..PI
  pi = PI();
  if(number > pi)
  { 
    number -= pi; 
    sign *= -1; 
  }

  // Now iterate with Taylor expansion
  // Sin(x) = x - x^3/3! + x^5/5! ...
  bcd square = number * number;
  bcd result = number;
  between    = number;

  for(long step = 3; ;step += 2)
  {
    between *= square;
    between /= bcd(step) * bcd(step - 1);
    between  = -between; // Switch sign each step

//     // DEBUGGING
//     printf("Step: %d\n",step);
//     result.DebugPrint("Result");
//     between.DebugPrint("Between");

    result  += between;

//     // DEBUGGING
//     result.DebugPrint("Between+");

//     // DEBUGGING
//     printf("%02d = %40s = %40s\n",step,between.AsString(),result.AsString());

    // Check tolerance criterion
    if(between.AbsoluteValue() < epsilon)
    {
      break;
    }
  }
  // Reapply the right sign
  if(sign < 0)
  {
    result = -result;
  }
  return result;
}

// bcd::Cosine
// Description: Cosine of the angle
// Technical:   Use the Taylor series. Cos(x) = 1 - x^2/2! + x^4/4! - x^6/6! ...
//              1) However first reduce x to between 0..2*PI
//              2) Then reduced it further to between 0..PI using cos(x)=Cos(2PI-x) for x >= PI
//              3) Now use the trisection identity cos(3x)=-3*cos(x)+4*cos(x)^3
//                 until argument is less than 0.5
//              4) Finally use Taylor 
//
bcd
bcd::Cosine() const
{
  long trisection, step;
  bcd between, result, number, number2;
  bcd c05(_T("0.5")), c1(1), c2(2), c3(3), c4(4);
  bcd epsilon = Epsilon(2);

  // Check if we can do this
  if(IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  if(!IsValid())
  {
    return SetInfinity(_T("BCD: Cannot take the cosine of infinity!"));
  }

  number = *this;

  // Reduce argument to between 0..2PI
  result  = PI();
  result *= c2;
  if(number.AbsoluteValue() > result )
  {
    between = number / result; 
    between = between.Floor();
    number -= between * result;
  }
  if(number.GetSign() < 0)
  {
    number += result;
  }
  // Reduced it further to between 0..PI. u==2PI
  between = PI();
  if( number > between )
  {
    number = result - number;
  }

  // Breaking criterion, cosine close to 1 if number is close to zero
  if(number.AbsoluteValue() < epsilon)
  {
    return (result = 1);
  }

  // Now use the trisection identity cos(3x)=-3*cos(x)+4*cos(x)^3
  // until argument is less than 0.5
  for( trisection = 0, between = c1; number / between > c05; ++trisection)
  {
    between *= c3;
  }
  number /= between;

  // First step of the iteration
  number2 = number * number;
  between = c1;
  result = between;

  // Iterate with Taylor expansion
  for(step=2; ;step += 2)
  {
    number   = number2; 
    number  /= bcd(step);
    number  /= bcd(step-1);
    between *= number;
    between  = -between;  // r.change_sign();
    // Tolerance criterion
    if(between.AbsoluteValue() < epsilon)
    {
      break;
    }
    result += between;
  }

  // Reapply the effects of the trisection again
  for( ;trisection > 0; --trisection)
  {
    result *= ( c4 * result * result - c3 );
  }
  return result;
}

// bcd::Tangent
// Description: Tangent of the angle
// Technical:   Use the identity tan(x)=Sin(x)/Sqrt(1-Sin(x)^2)
//              However first reduce x to between 0..2*PI
//
bcd
bcd::Tangent() const
{
  bcd result, between, number;
  bcd two(2), three(3);

  // Check if we can do this
  if(IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  if(!IsValid())
  {
    return SetInfinity(_T("BCD: Cannot take the tangent of infinity!"));
  }

  number = *this;

  // Reduce argument to between 0..2PI
  bcd pi     = PI();
  bcd twopi  = two * pi;
  if(number.AbsoluteValue() > twopi )
  {
    between  = number / twopi; 
    between  = between.Floor();
    number  -= between * twopi;
  }

  if(number.GetSign() < 0)
  {
    number += twopi;
  }
  bcd halfpi     = pi / two;
  bcd oneandhalf = three * halfpi;
  if( number == halfpi || number == oneandhalf)
  { 
    return SetInfinity(_T("BCD: Cannot calculate a tangent from a angle of 1/2 pi or 3/2 pi"));
  }
  // Sin(x)/Sqrt(1-Sin(x)^2)
  result     = number.Sine(); 
  bcd square = result * result;
  bcd divide = bcd(1) - square;
  bcd root   = sqrt(divide);
  result    /= root;

  // Correct for the sign
  if(number > halfpi && number < oneandhalf)
  {
    result = -result;
  }
  return result;
}

// bcd::Asine
// Description: ArcSine (angle) of the ratio
// Technical:   Use Newton by solving the equation Sin(y)=x. Then y is Arcsine(x)
//              Iterate by Newton y'=y-(sin(y)-x)/cos(y). 
//              With initial guess using standard double precision arithmetic.
// 
bcd     
bcd::ArcSine() const
{
  long step, reduction, sign;
  double d;
  bcd between, number, result, factor;
  bcd c1(1);
  bcd c2(2);
  bcd c05(_T("0.5"));
  bcd epsilon = Epsilon(5);

  // Check if we can do this
  if(IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  if(!IsValid())
  {
    return SetInfinity(_T("BCD: Cannot take the arcsine of infinity!"));
  }

  number = *this;
  if(number > c1 || number < -c1)
  {
    return SetInfinity(_T("BCD: Cannot calculate an arcsine from a number > 1 or < -1"));
  }

  // Save the sign
  sign = number.GetSign();
  if(sign < 0)
  {
    number = -number;
  }

  // Reduce the argument to below 0.5 to make the newton run faster
  for(reduction = 0; number > c05; ++reduction)
  {
    number /= sqrt(c2) * sqrt( c1 + sqrt( c1 - number * number ));
  }
  // Quick approximation of the asin
  d = ::asin(number.AsDouble());
  result = bcd( d );
  factor = bcd( 1.0 / ::cos(d)); // Constant factor 

  // Newton Iteration
  for( step=0;; step++)
  {
    between = ( result.Sine() - number ) * factor;
    // if( y - r == y )
    if(between.AbsoluteValue() < epsilon)
    {
      break;
    }
    result -= between;
  }

  // Repair the reduction in the result
  result *= bcd((long) (1 << reduction) );

  // Take care of sign
  if( sign < 0 )
  {
    result = -result;
  }

  // this is the result
  return result;
}

// bcd::ArcCosine
// Description: ArcCosine (angle) of the ratio
// Technical:   Use ArcCosine(x) = PI/2 - ArcSine(x)
bcd     
bcd::ArcCosine() const
{
  bcd y;

  // Check if we can do this
  if(IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  if(!IsValid())
  {
    return SetInfinity(_T("BCD: Cannot take the arc-cosine of infinity!"));
  }

  y  = PI();
  y /= bcd(2L);
  y -= ArcSine();

  return y;
}

// bcd::ArcTangent
// Description: ArcTangent (angle) of the ratio
// Technical:   Use the Taylor series. ArcTan(x) = x - x^3/3 + x^5/5 ...
//              However first reduce x to abs(x)< 0.5 to improve taylor series
//              using the identity. ArcTan(x)=2*ArcTan(x/(1+sqrt(1+x^2)))
//
bcd
bcd::ArcTangent() const
{
  bcd  result, square;
  bcd  between1,between2;
  bcd  half(_T("0.5"));
  bcd  one(1);
  bcd  epsilon = Epsilon(5);
  long k = 2;

  // Check if we can do this
  if(IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  if(!IsValid())
  {
    return SetInfinity(_T("BCD: Cannot take the arc-tangent of infinity!"));
  }

  result   = *this;
  // Transform the solution to ArcTan(x)=2*ArcTan(x/(1+sqrt(1+x^2)))
  result = result / ( one + sqrt( one + (result * result)));
  if( result.AbsoluteValue() > half) // if still to big then do it again
  {
    k = 4;
    result = result / (one + sqrt(one + (result * result)));
  }
  square = result * result;
  between1  = result;
  // Now iterate using Taylor expansion
  for(long step = 3;; step += 2)
  {
    between1 *= square;
    between1  = -between1;
    between2  = between1 / bcd(step);
    // Tolerance criterion
    if(between2.AbsoluteValue() < epsilon)
    {
      break;
    }
    result += between2;
  }

  // Reapply the reduction/transformation
  result *= bcd(k);

  // this is the result
  return result;
}

// bcd::Atan2
// Description: Angle of two points
// Technical:   return the angle (in radians) from the X axis to a point (y,x).
//              use atan() to calculate atan2()
//
bcd
bcd::ArcTangent2Points(const bcd& p_x) const
{
  bcd result;
  bcd number = *this;
  bcd nul, c05(_T("0.5"));

  // Check if we can do this
  if(IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  if(!IsValid() || !p_x.IsValid())
  {
    return SetInfinity(_T("BCD: Cannot take the arc-tangent-2 of infinity!"));
  }

  if( p_x == nul && number == nul)
  {
    return nul;
  }
  if( p_x == nul )
  {
    result = PI();
    if( number < nul )
    {
      result *= -c05;
    }
    else
    {
      result *= c05;
    }
  }
  else
  {
    if( number == nul )
    {
      if( p_x < nul )
      {
        result = PI();
      }
      else
      {
        result = nul;
      }
    }
    else
    {
      result = bcd( number / p_x ).ArcTangent();
      if( p_x < nul  && number < nul )
      {
        result -= PI();
      }
      if( p_x < nul && number >= nul )
      {
        result += PI();
      }
    }
  }
  return result;
}

//////////////////////////////////////////////////////////////////////////
//
// END OF BCD TRIGONOMETRIC FUNCTIONS
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// GET AS SOMETHING DIFFERENT THAN THE BCD
//
//////////////////////////////////////////////////////////////////////////

// bcd::AsDouble
// Description: Get as a double
//
double  
bcd::AsDouble() const
{
  double result = 0.0;

  // Check if we have a result
  if(!IsValid() || IsNULL())
  {
    return result;
  }

  if(bcdDigits >= 8)
  {
    // SHORTCUT FOR PERFORMANCE: 
    // A double cannot have more than 16 digits
    // so everything over 16 digits gets discarded
    result  = ((double)m_mantissa[0]) * 10.0 / (double) bcdBase;
    result += ((double)m_mantissa[1]) * 10.0 / (double) bcdBase / (double) bcdBase;
    //result += ((double)m_mantissa[2]) / bcdBase / bcdBase / bcdBase;
  }
  else
  {
    // Works for ALL implementations of bcdDigits  and bcdLength
    // Get the mantissa into the result
    double factor = 1.0;
    for(int ind = 0; ind < bcdLength; ++ind)
    {
      long base    = bcdBase / 10;
      long between = m_mantissa[ind];

      for(int pos = bcdDigits; pos > 0; --pos)
      {
        // Get next number
        long number = between / base;
        between %= base;
        base /= 10;

        // Set in the result
        result += (double)number * factor;
        factor /= 10;
      }
    }
  }

  // Take care of exponent
  if(m_exponent)
  {
    result *= ::pow(10.0,m_exponent);
  }
  // Take care of the sign
  if(m_sign == Sign::Negative)
  {
    result = -result;
  }
  return result;
}

// bcd::AsShort
// Description: Get as a short
short   
bcd::AsShort() const
{
  // Check if we have a result
  if(!IsValid() || IsNULL())
  {
    return 0;
  }

  // Quick check for zero
  if(m_exponent < 0)
  {
    return 0;
  }
  int exponent = bcdDigits - m_exponent - 1;

  // Get from the mantissa
  int result = m_mantissa[0];

  // Adjust to exponent
  while(exponent--)
  {
    result /= 10;
  }

  // Take care of sign and over/under flows
  if(m_sign == Sign::Positive)
  {
    if(result > SHORT_MAX)
    {
      throw StdException(_T("BCD: Overflow in conversion to short number."));
    }
  }
  else
  {
    if(result < SHORT_MIN)
    {
      throw StdException(_T("BCD: Underflow in conversion to short number."));
    }
    result = -result;
  }
  return (short) result;
}

// bcd::AsUShort
// Description: Get as an unsigned short
ushort  
bcd::AsUShort() const
{
  // Check if we have a result
  if(!IsValid() || IsNULL())
  {
    return 0;
  }

  // Check for unsigned
  if(m_sign == Sign::Negative)
  {
    throw StdException(_T("BCD: Cannot convert a negative number to an unsigned short number."));
  }
  // Quick check for zero
  if(m_exponent < 0)
  {
    return 0;
  }

  // Get from the mantissa
  int result = m_mantissa[0];

  // Adjust to exponent
  int exponent = bcdDigits - m_exponent - 1;
  while(exponent--)
  {
    result /= 10;
  }

  // Take care of overflow
  if(result > USHORT_MAX)
  {
    throw StdException(_T("BCD: Overflow in conversion to unsigned short number."));
  }

  return (short)result;
}

// bcd::AsLong
// Description: Get as a long
long    
bcd::AsLong() const
{
  // Check if we have a result
  if(!IsValid() || IsNULL())
  {
    return 0;
  }

  // Quick optimization for really small numbers
  if(m_exponent < 0)
  {
    return 0L;
  }

  // Get from the mantissa
  int64 result = ((int64)m_mantissa[0] * bcdBase) + ((int64)m_mantissa[1]);

  // Adjust to exponent
  int exponent = 2 * bcdDigits - m_exponent - 1;
  if(exponent > 0)
  {
    while(exponent--)
    {
      result /= 10;
    }
  }

  // Take care of sign and over/under flows
  if(m_sign == Sign::Positive)
  {
    if(result > LONG_MAX)
    {
      throw StdException(_T("BCD: Overflow in conversion to integer number."));
    }
  }
  else
  {
    if(result < LONG_MIN)
    {
      throw StdException(_T("BCD: Underflow in conversion to integer number."));
    }
    result = -result;
  }
  return (long) result;
}

// bcd::AsULong
// Get as an unsigned long
ulong   
bcd::AsULong() const
{
  // Check if we have a result
  if(!IsValid() || IsNULL())
  {
    return 0;
  }

  // Check for unsigned
  if(m_sign == Sign::Negative)
  {
    throw StdException(_T("BCD: Cannot convert a negative number to an unsigned long."));
  }

  // Quick optimization for really small numbers
  if(m_exponent < 0)
  {
    return 0L;
  }

  // Get from the mantissa
  int64 result = ((int64)m_mantissa[0] * bcdBase) + ((int64)m_mantissa[1]);

  // Adjust to exponent
  int exponent = 2 * bcdDigits - m_exponent - 1;
  while(exponent--)
  {
    result /= 10;
  }

  // Take care of overflow
  if(result > ULONG_MAX)
  {
    throw StdException(_T("BCD: Overflow in conversion to unsigned long integer."));
  }
  return (long)result;
}

// bcd::AsInt64
// Description: Get as a 64 bits long number
//
int64
bcd::AsInt64() const
{
  // Check if we have a result
  if(!IsValid() || IsNULL())
  {
    return 0;
  }

  // Quick optimization for really small numbers
  if(m_exponent < 0)
  {
    return 0L;
  }
  int64 result1 = 0L;
  int64 result2 = 0L;
  int exponent  = 4 * bcdDigits - m_exponent - 1;
  int64 base2   = (int64)bcdBase * ((int64)bcdBase);
  int64 base    = base2 / 10;

  // Get from the mantissa
  result1 = ((int64)m_mantissa[0] * bcdBase) + ((int64)m_mantissa[1]);
  result2 = ((int64)m_mantissa[2] * bcdBase) + ((int64)m_mantissa[3]);

  // Adjust to exponent
  while(exponent--)
  {
    int64 carry = result1 %10;
    result1 /= 10;
    result2 /= 10;
    result2 += carry * base;
  }

  // Take care of overflow
  if(result1 > (LLONG_MAX / base2))
  {
    throw StdException(_T("BCD: Overflow in conversion to 64 bits integer number."));
  }
  result2 += (result1 * base2);

  // Take care of sign 
  if(m_sign == Sign::Negative)
  {
    result2 = -result2;
  }
  return result2;
}

// Get as an unsigned 64 bits long
uint64  
bcd::AsUInt64() const
{
  // Check if we have a result
  if(!IsValid() || IsNULL())
  {
    return 0;
  }

  // Check for negative
  if(m_sign == Sign::Negative)
  {
    throw StdException(_T("BCD: Cannot convert a negative number to an unsigned 64 bits integer"));
  }
  // Quick optimization for really small numbers
  if(m_exponent < 0)
  {
    return 0L;
  }
  uint64 result1 = 0L;
  uint64 result2 = 0L;
  int exponent   = 4 * bcdDigits - m_exponent - 1;
  uint64 base2   = (uint64)bcdBase * ((uint64)bcdBase);
  uint64 base    = base2 / 10;

  // Get from the mantissa
  result1 = ((uint64)m_mantissa[0] * bcdBase) + ((uint64)m_mantissa[1]);
  result2 = ((uint64)m_mantissa[2] * bcdBase) + ((uint64)m_mantissa[3]);

  // Adjust to exponent
  while(exponent--)
  {
    uint64 carry    = result1 %10;
    result1 /= 10;
    result2 /= 10;
    result2 += carry * base;
  }

  // Take care of overflow
  if(result1 > (ULLONG_MAX / base2))
  {
    throw StdException(_T("BCD: Overflow in conversion to 64 bits unsigned integer number."));
  }
  result2 += (result1 * base2);

  return result2;
}

// bcd::AsString
// Description: Get as a mathematical string
//
// Engineering format: Convert back to "[sign][digit][.[digit]*][E[sign][digits]+]"
// Bookkeeping format: Convert back to "[sign]9[digit]*][.[digit]*]"
// Optionally also print the positive '+' ('-' negative sign is always printed!)
// Optionally get with fixed decimals, default = 2 decimals (most common default for bookkeeping purposes)
// Optionally get as much as needed decimals with "p_decimals = 0"
XString 
bcd::AsString(Format p_format /*=Bookkeeping*/,bool p_printPositive /*=false*/,int p_decimals /*=2*/) const
{
  XString result;
  int expo   = m_exponent;
  int prec   = bcdDigits * bcdLength;

  // Shortcut for infinity and not-a-number
  switch(m_sign)
  {
    case Sign::NaN:     return  _T("NaN");
    case Sign::INF:     return  _T("INF");
    case Sign::MIN_INF: return _T("-INF");
    case Sign::ISNULL:  return _T("NULL");
  }

  // Check format possibilities
  if(expo < -(prec/2) || expo > (prec/2))
  {
    p_format = Format::Engineering;
  }

  // Construct the mantissa string
  for(int mantpos = 0; mantpos < bcdLength; ++mantpos)
  {
    long number = m_mantissa[mantpos];
    long base   = bcdBase / 10;
    for(int digitpos = bcdDigits; digitpos > 0; --digitpos)
    {
      long num = number / base;
      number   = number % base;
      TCHAR c  = (TCHAR)num + '0';
      base    /= 10;

      result += c;
    }
  }
  // Stripping trailing zeros.
  result = result.TrimRight('0');

  if(p_format == Format::Engineering)
  {
    XString left = result.Left(1);
    result = left + XString(_T(".")) + result.Mid(1) + XString(_T("E"));
    result += LongToString(expo);
  }
  else // Bookkeeping
  {
    if(m_exponent < 0)
    {
      XString left(_T("0."));
      for(int ind = -1; ind > m_exponent; --ind)
      {
        left += _T("0");
      }
      result = left + result;
    }
    if(m_exponent >= 0)
    {
      int pos = 1 + m_exponent;
      XString before = result.Left(pos);
      XString behind = result.Mid(pos);
      while(before.GetLength() < pos)
      {
        before += _T("0");
      }
      result = before;
      while(p_decimals > 0 && behind.GetLength() < p_decimals)
      {
        behind += '0';
      }
      if(!behind.IsEmpty())
      {
        result += XString(_T(".")) + behind;
      }
    }
  }

  // Take care of the sign
  // result = ((m_sign == Positive ) ? "+" : "-") + result;
  if(m_sign == Sign::Positive)
  {
    if(p_printPositive)
    {
      result = _T("+") + result;
    }
  }
  else
  {
    result = _T("-") + result;
  }

  // Ready
  return result;
}

// Display strings are always in Format::Bookkeeping
// as most users find mathematical exponential notation hard to read.
XString 
bcd::AsDisplayString(int p_decimals /*=2*/) const
{
  // Shortcut for infinity and not-a-number
  switch(m_sign)
  {
    case Sign::NaN:     return  _T("NaN");
    case Sign::INF:     return  _T("INF");
    case Sign::MIN_INF: return _T("-INF");
    case Sign::ISNULL:  return _T("NULL");
  }

  // Initialize locale strings
  InitValutaString();

  // Not in the bookkeeping range
  if(m_exponent > 12 || m_exponent < -2)
  {
    return AsString(bcd::Format::Engineering,false,p_decimals);
  }
  bcd number(*this);
  number.Round(p_decimals);

  XString str = number.AsString(Format::Bookkeeping,false,p_decimals);
  int pos = str.Find('.');
  if(pos >= 0)
  {
    str.Replace(_T("."),g_locale_decimalSep);
  }

  // Apply thousand separators in first part of the number
  if((pos > 0) || (pos == -1 && str.GetLength() > 3))
  {
    XString result = pos > 0 ? str.Mid(pos) : XString();
    str = pos > 0 ? str.Left(pos) : str;
    pos = result.GetLength();

    while(str.GetLength() > 3)
    {
      result = XString(g_locale_thousandSep) + str.Right(3) + result;
      str = str.Left(str.GetLength() - 3);
    }
    str += result;
  }
  
  // Extra zero's for the decimals?
  if((pos <= 0 && p_decimals > 0) || (0 < pos && pos <= p_decimals))
  {
    // Round on this number of decimals
    int decimals(p_decimals);
    if(pos < 0)
    {
      str += g_locale_decimalSep;
    }
    else
    {
      decimals = p_decimals - pos + 1;
    }
    for(int index = 0;index < decimals; ++index)
    {
      str += _T("0");
    }
  }
  return str;
}

// Get as an ODBC SQL NUMERIC
void
bcd::AsNumeric(SQL_NUMERIC_STRUCT* p_numeric) const
{
  // Init the value array
  memset(p_numeric->val,0,SQL_MAX_NUMERIC_LEN);

  // Check if we have a result
  if(!IsValid() || IsNULL())
  {
    return;
  }

  // Special case for 0.0 or smaller than can be contained (1.0E-38)
  if(IsZero() || m_exponent < -SQLNUM_MAX_PREC)
  {
    return;
  }

  // Check for overflow. Cannot be greater than 9.999999999E+37
  if(m_exponent >= SQLNUM_MAX_PREC)
  {
    throw StdException(_T("BCD: Overflow in converting bcd to SQL NUMERIC/DECIMAL"));
  }

  SQLCHAR precision = 0;
  SQLCHAR scale = 0;
  CalculatePrecisionAndScale(precision,scale);

  // Setting the sign, precision and scale
  p_numeric->sign      = (m_sign == Sign::Positive) ? 1 : 0;
  p_numeric->precision = precision;
  p_numeric->scale     = scale;

  // Converting the value array
  bcd one(1);
  bcd radix(256);
  bcd accu(*this);
  int index = 0;

  // Here is the big trick: use the exponent to scale up the number
  // Adjusting m_exponent to positive scaled integer result
  accu.m_exponent += (short)scale;
  accu.m_sign      = Sign::Positive;

  while(true)
  {
    // Getting the next val array value, relying on the bcd::modulo
    bcd val = accu.Mod(radix);
    p_numeric->val[index++] = (SQLCHAR) val.AsLong();

    // Adjust the intermediate accu
    accu -= val;
    accu  = accu.Div(radix);

    // Breaking criterion: nothing left
    if(accu < one)
    {
      break;
    }

    // Breaking criterion on overflow
    // Check as last, number could fit exactly!
    if(index >= SQL_MAX_NUMERIC_LEN)
    {
      break;
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//
// END OF "GET AS SOMETHING DIFFERENT"
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// GETTERS of BCD
//
//////////////////////////////////////////////////////////////////////////

// bcd::IsNull
// Description: Gets the fact that bcd is exactly 0.0
bool  
bcd::IsZero() const
{
  // Shortcut test
  if(m_sign == Sign::Negative || m_exponent != 0)
  {
    return false;
  }
  // The normalized mantissa's first part must be a number
  // otherwise the total of the bcd is NULL!!
  // So: No need to scan the whole mantissa!!
  if(m_mantissa[0])
  {
    // Also works for (-)INF, NaN and NULL
    return false;
  }
  return true;
}

// Is bcd a database NULL
bool
bcd::IsNULL() const
{
  return m_sign == Sign::ISNULL;
}

// bcd::IsNearZero
// Description: Nearly zero or zero
bool
bcd::IsNearZero()
{
  // NULL, NaN or (-)INF is never 'near zero'
  if(m_sign > Sign::Negative)
  {
    return false;
  }
  bcd epsilon = Epsilon(2);
  return AbsoluteValue() < epsilon;
}

// Not an (-)INF or a NaN
bool
bcd::IsValid() const
{
  return m_sign <= Sign::Negative;
}

// bcd::GetSign
// Description: Gets the sign
// Technical:   Returns -1 (negative), 0 or 1 (Positive)
// Beware;      NaN, (-)INF also returns a 0
int   
bcd::GetSign() const
{
  // Negative number returns -1
  if(m_sign == Sign::Negative)
  {
    return -1;
  }
  // Positive number returns 1
  if(m_mantissa[0] || m_exponent)
  {
    return 1;
  }
  // Number is NULL. Sign = 0, or (-)INF or NaN
  return 0;
}

// Gets Signed status Positive, Negative, -INF, INF, NaN
bcd::Sign
bcd::GetStatus() const
{
  return m_sign;
}

// bcd::GetLength
// Description: Total length (before and after decimal point)
// Technical:   Returns the actual length (not the precision)
int   
bcd::GetLength() const
{
  int length  = 0;
  int counter = 0;

  // Length of the display string
  switch(m_sign)
  {
    case Sign::NaN:     return 3;
    case Sign::INF:     return 3;
    case Sign::MIN_INF: return 4;
    case Sign::ISNULL:  return 4;
  }

  // Quick optimization
  if(IsZero())
  {
    // Zero (0) has length of 1
    return 1;
  }

  // Walk the mantissa
  for(int pos = 0;pos < bcdLength; ++pos)
  {
    long number = m_mantissa[pos];
    long base   = bcdBase / 10;

    for(int dig = bcdDigits; dig > 0; --dig)
    {
      // Actual position
      ++counter;

      // See if there is a digit here
      long num = number / base;
      number  %= base;
      base    /= 10;
      if(num)
      {
        length = counter;
      }
    }
  }
  return length;
}

// bcd::GetPrecision
// Description: Total precision (length after the decimal point)
int   
bcd::GetPrecision() const
{
  // Quick optimization
  if(IsZero() || !IsValid() || IsNULL())
  {
    return 0;
  }

  // Default max precision
  int precision = bcdPrecision;

  for(int posLength = bcdLength - 1; posLength >= 0; --posLength)
  {
    int mant = m_mantissa[posLength];
    for(int posDigits =  bcdDigits - 1;posDigits >= 0; --posDigits)
    {
      int last = mant % 10;
      mant /= 10;       // Next pos
      if(last == 0)
      {
        --precision;
      }
      else
      {
        // Break on first non-zero 
        // by terminating both loops
        posDigits = -1;
        posLength = -1;
      }
    }
  }
  // Precision is 1 based (1 position for the decimal point implied)
  precision -= 1;
  // Precision is inverse to the exponent (higher exponent, means less behind the decimal point)
  precision -= m_exponent;
  // Precision cannot be less than zero
  if(precision < 0)
  {
    precision = 0;
  }
  // Return the result
  return precision;
}

// bcd::GetMaxSize
// Description: Get the max size of a bcd
int 
bcd::GetMaxSize(int /* precision /*= 0*/)
{
  // int size = bcdDigits * bcdLength;
  return bcdPrecision;
}

// bcd::GetFitsInLong
// Gets the fact that it fits in a long
bool  
bcd::GetFitsInLong() const
{
  // Infinity does not fit in a long :-)
  if(!IsValid() || IsNULL())
  {
    return false;
  }

  try
  {
    AsLong();
  }
  catch(StdException& ex)
  {
    ReThrowSafeException(ex);
    return false;
  }
  return true;
}

// bcd::GetFitsInInt64
// Description: Gets the fact that it fits in an int64
//
bool  
bcd::GetFitsInInt64() const
{
  // Infinity does not fit in an int64 :-)
  if(!IsValid() || IsNULL())
  {
    return false;
  }

  try
  {
    AsInt64();
  }
  catch(StdException& ex)
  {
    ReThrowSafeException(ex);
    return false;
  }
  return true;
}

// bcd::GetHasDecimals
// Description: Decimal part (behind the decimal point) is not "000" (zeros)
bool  
bcd::GetHasDecimals() const
{
  // Shortcut for ZERO
  if(IsZero() || !IsValid() || IsNULL())
  {
    return false;
  }
  // Shortcut: some decimals must exist
  if(m_exponent < 0)
  {
    return true;
  }

  // Compare the length with the exponent.
  // If exponent smaller, the rest is behind the decimal point
  int length = GetLength();
  if(m_exponent < (length - 1))
  {
    return true;
  }
  return false;
}

// bd::GetExponent
// Description: Gets the 10-based exponent
int   
bcd::GetExponent() const
{
  // Infinity has no exponent
  if(!IsValid() || IsNULL())
  {
    return 0;
  }
  return m_exponent;
}

// bcd::GetMantissa
// Description: Gets the mantissa
bcd   
bcd::GetMantissa() const
{
  if(!IsValid() || IsNULL())
  {
    return SetInfinity(_T("BCD: Infinity cannot give a mantissa."));
  }

  bcd number(*this);

  number.m_sign     = Sign::Positive;
  number.m_exponent = 0;
  return number;
}

//////////////////////////////////////////////////////////////////////////
//
// END OF GETTERS OF BCD
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// INTERNALS OF BCD
//
//////////////////////////////////////////////////////////////////////////

// Take the absolute value of a long
// This method is taken outside the <math> library or other macro's.
// It was needed because some versions of std linked wrongly
long 
bcd::long_abs(const long p_value) const
{
  if(p_value < 0)
  {
    return -p_value;
  }
  return p_value;
}

// bcd::SetValueInt
// Description: Sets one integer in this bcd number
void  
bcd::SetValueInt(const int p_value)
{
  Zero();

  // Shortcut if value is zero
  if(p_value == 0)
  {
    return;
  }
  // Take care of sign
  m_sign = (p_value < 0) ? Sign::Negative : Sign::Positive;
  // Place in mantissa
  m_mantissa[0] = long_abs(p_value);
  // And normalize
  Normalize(bcdDigits - 1);
}

// bcd::SetValueLong
// Description: Set the bcd to the value of two longs
//              Numbers can be set as bcd(12345,4567) -> +1.23454567E+4 = 12,345.4567
//              Works up to 8 positions before and after the decimal point
// Parameters:  const long value     // value before the decimal point
//              const long restValue // Optional value behind the decimal point
//
void
bcd::SetValueLong(const long p_value, const long p_restValue)
{
  Zero();

  if(p_value == 0 && p_restValue == 0)
  {
    // Nothing more to do. We are zero
    return;
  }
  // Get the sign
  if (p_value == 0)
  {
    m_sign = (p_restValue < 0) ? Sign::Negative : Sign::Positive;
  }
  else
  {
    m_sign = (p_value < 0) ? Sign::Negative : Sign::Positive;
  }
  // Fill in mantissa. restValue first
  int norm = 0;

  if(p_restValue)
  {
    m_mantissa[0] = long_abs(p_restValue % bcdBase);
    norm = -1;

    if(p_restValue / bcdBase)
    {
      ShiftRight();
      m_mantissa[0] = long_abs(p_restValue / bcdBase);
      norm -= bcdDigits;
    }
    Normalize(norm);
    norm = 0;
    if(p_value)
    {
      ShiftRight();
    }
  }

  if(p_value % bcdBase)
  {
    m_mantissa[0] = long_abs(p_value % bcdBase);
    norm = bcdDigits - 1;
  }
  if(p_value / bcdBase)
  {
    ShiftRight();
    m_mantissa[0] = long_abs(p_value / bcdBase);
    norm = 2 * bcdDigits - 1;
  }
  Normalize(norm);
}

// bcd::SetValueint64
// Description: Set the bcd to the value of two longs
//              Numbers can be set as bcd(12345,4567) -> +1.23454567E+4 = 12,345.4567
//              Works up to all 19 positions before and after the decimal point
// Parameters:  const long value     // value before the decimal point
//              const long restValue // Optional value behind the decimal point
// Be advised:  The restValues behave as digits left shifted to the decimal marker
//              5      becomes 0.5
//              15     becomes 0.15
//              2376   becomes 0.2376
void  
bcd::SetValueInt64(const int64 p_value, const int64 p_restValue)
{
  Zero();

  int64 dblBcdDigits = (int64)bcdBase * (int64)bcdBase;

  if(p_value == 0L && p_restValue == 0L)
  {
    // Nothing more to do. We are zero
    return;
  }
  // Get the sign
  if(p_value == 0L)
  {
    m_sign = (p_restValue < 0L) ? Sign::Negative : Sign::Positive;
  }
  else
  {
    m_sign = (p_value < 0L) ? Sign::Negative : Sign::Positive;
  }
  // Fill in mantissa
  int norm = 0;

  // Part after the decimal point first
  if(p_restValue % bcdBase)
  {
    m_mantissa[0] = long_abs(p_restValue % bcdBase);
    norm = bcdDigits - 1;
  }
  if(p_restValue / bcdBase)
  {
    ShiftRight();
    m_mantissa[0] = long_abs((long)((p_restValue / bcdBase) % bcdBase));
    norm = 2 * bcdDigits - 1;
  }
  if(p_restValue / dblBcdDigits)
  {
    ShiftRight();
    m_mantissa[0] = long_abs((long)(p_restValue / dblBcdDigits));
    norm = 3 * bcdDigits - 1;
  }

  if(p_restValue)
  {
    // Normalize the rest value to be left shifted
    Normalize(norm);
    norm = 0; // reset !!
  }

  // Part before the decimal point
  if(p_value % bcdBase)
  {
    ShiftRight();
    m_mantissa[0] = long_abs((long)(p_value % bcdBase));
    norm = bcdDigits - 1;
  }
  if(p_value / bcdBase)
  {
    ShiftRight();
    m_mantissa[0] = long_abs((long)((p_value / bcdBase) % bcdBase));
    norm = 2 * bcdDigits - 1;
  }
  if(p_value / dblBcdDigits)
  {
    ShiftRight();
    m_mantissa[0] = long_abs((long)(p_value / dblBcdDigits));
    norm = 3 * bcdDigits -1;
  }

  // Normalize the whole result
  Normalize(norm);
}

void    
bcd::SetValueUInt64(const uint64 p_value,const int64 p_restValue)
{
  uint64 value(p_value);
  bool extra = false;
  if(p_value > LONGLONG_MAX)
  {
    extra  = true;
    value -= LONGLONG_MAX;
    value -= 1;
  }
  SetValueInt64(value,p_restValue);
  if(extra)
  {
    *this += bcd(LONGLONG_MAX);
    *this += bcd(1);
  }
  Normalize();
}

// bcd::SetValueDouble
// Description: Sets the value from a double
void  
bcd::SetValueDouble(const double p_value)
{
  // Make empty
  Zero();

  // Save in between value
  double between = p_value;
  double notused;

  // Take care of sign
  if(p_value < 0.0)
  {
    m_sign  = Sign::Negative;
    between = -between;
  }
  // Take care of exponent
  m_exponent = (short)::log10(between);

  // Normalize
  between *= ::pow(10.0,(int)-m_exponent);

  // Set the double mantissa into the m_mantissa
  // A double has 16 digits
  for(int ind = 0;ind < (16 / bcdDigits); ++ind)
  {
    long base = bcdBase / 10;
    for(int pos = 0; pos < bcdDigits; ++pos)
    {
      long res = (long) ::fmod(between,10.0);
      between  = ::modf(between,&notused) * 10;

      m_mantissa[ind] += res * base;
      base /= 10;
    }
  }
  Normalize();
}

// bcd::SetValueString
// Description: Set the value of the bcd from a string
// Technical:   Scans [sign][digit][.[digit]*][E[sign][digits]+]
//              part =       1        2          3    
void
bcd::SetValueString(LPCTSTR p_string,bool /*p_fromDB*/)
{
  // Zero out this number
  Zero();

  bool spacing   = true;
  bool signing   = true;
  int  mantpos   = 0;
  int  digitpos  = bcdDigits;
  int  base      = bcdBase / 10;
  int  part      = 1;
  int  exp_extra = 0;
  Sign exp_sign  = Sign::Positive;
  
  // For normalized numbers without a first part
  m_exponent = -1;

  // Check special cases
  if(_tcscmp(p_string,_T("INF")) == 0)
  {
    m_sign = Sign::INF;
    return;
  }
  if(_tcscmp(p_string,_T("-INF")) == 0)
  {
    m_sign = Sign::MIN_INF;
    return;
  }
  if(_tcscmp(p_string,_T("NaN")) == 0)
  {
    m_sign = Sign::NaN;
    return;
  }
  if(_tcscmp(p_string,_T("NULL")) == 0)
  {
    m_sign = Sign::ISNULL;
    return;
  }

  // Scan the entire string
  for(LPCTSTR pos = p_string; *pos; ++pos)
  {
    // Get a char at the next position
    _TUCHAR c = *pos;

    // Skip whitespace at the beginning
    if(spacing)
    {
      if(isspace(c))
      {
        continue;
      }
      spacing = false;
    }
    // See if a sign is found
    if(signing && part == 1)
    {
      if(c == '-')
      {
        m_sign  = Sign::Negative;
        signing = false;
        continue;
      }
      if(c == '+')
      {
        m_sign  = Sign::Positive;
        signing = false;
        continue;
      }
    }
    // Get type of next character
    switch(c)
    {
      case '.': part = 2; 
                break;
      case 'e': // Fall through
      case 'E': part = 3; 
                break;
      case '-': exp_sign = Sign::Negative;
                break;
      case '+': exp_sign = Sign::Positive;
                break;
      default:  // Now must be a digit. No other chars allowed
                if(isdigit(c) == false)
                {
                  Zero();
                  m_sign = Sign::NaN;
                  if(g_throwing)
                  {
                    throw StdException(_T("BCD: Conversion from string. Bad format in decimal number"));
                  }
                  return;
                }
                break;
    }
    // Decimal point found. Just continue
    if(part == 2 && c == '.')
    {
      continue;
    }
    // Exponent just found. Just continue;
    if(part == 3 && (c == 'e' || c == 'E'))
    {
      continue;
    }
    // Exponent sign just found. Just continue
    if(c == '-' || c == '+')
    {
      continue;
    }
    // Get next char pos
    int number = c - '0';
    // Building the mantissa
    if(part == 1 || part == 2)
    {
      // Adjusting exponent
      if(part == 1)
      {
        // Before the decimal point, the exponent grows
        ++m_exponent;
      }
      // Set in the mantissa
      if(mantpos < bcdLength)
      {
        m_mantissa[mantpos] += number * base;
        base /= 10;

        // Check for next position in the mantissa
        if(--digitpos == 0)
        {
          // Reset for the next long in the mantissa
          base     = bcdBase / 10;
          digitpos = bcdDigits;

          // Next position
          ++mantpos;
        }
      }
      // else skip the latter part of the mantissa
    }
    // Building the exponent
    if(part == 3)
    {
      exp_extra *= 10;
      exp_extra += number;
    }
  }
  // Ready with the string

  // Adjust the exponent from saved variables
  if(exp_extra)
  {
    if(exp_sign == Sign::Negative)
    {
      exp_extra = -exp_extra;
    }
    m_exponent += (short) exp_extra;
  }
  // Normalize for "0.xxx" case
  Normalize();
}

// Sets the value from a SQL NUMERIC
void  
bcd::SetValueNumeric(const SQL_NUMERIC_STRUCT* p_numeric)
{
  int maxval = SQL_MAX_NUMERIC_LEN - 1;

  // Start at zero
  Zero();

  // Find the last value in the numeric
  int ind = maxval;
  for(;ind >= 0; --ind)
  {
    if(p_numeric->val[ind])
    {
      maxval = ind;
      break;
    }
  }

  // Special case: NUMERIC = zero
  if(ind < 0)
  {
    return;
  }

  // Compute the value array to the bcd-mantissa
  bcd radix(1);
  for(ind = 0;ind <= maxval; ++ind)
  {
    bcd val = radix * bcd(p_numeric->val[ind]);
    *this   = Add(val);
    radix  *= 256;  // Value array is in 256 radix
  }

  // Compute the exponent from the precision and scale
  m_exponent -= p_numeric->scale;

  // Adjust the sign
  m_sign      = (p_numeric->sign == 1) ? Sign::Positive : Sign::Negative;
}

// bcd::Normalize
// Description: Normalize the exponent. 
//              Always up to first position with implied decimal point
void
bcd::Normalize(int p_startExponent /*=0*/)
{
  // Set starting exponent
  if(p_startExponent)
  {
    m_exponent = (short)p_startExponent;
  }
  // Check for zero first
  bool zero = true;
  for(int ind = 0; ind < bcdLength; ++ind)
  {
    if(m_mantissa[ind])
    {
      zero = false;
    }
  }
  // Zero mantissa found
  if(zero)
  {
    m_sign     = Sign::Positive;
    m_exponent = 0;
    return;
  }
  // See to it that the mantissa is normalized
  int shift = 0;
  while(((m_mantissa[0] * 10) / bcdBase) == 0)
  {
    ++shift;
    Mult10();
  }
  // Calculate exponent from number of shifts
  m_exponent -= (short) shift;
}

// bcd::Mult10
// Description: Multiply the mantissa by 10
// Technical:   Optimize by doing shifts
//              Pure internal operation for manipulating the mantissa
//
void
bcd::Mult10(int p_times /* = 1 */)
{
  // If the number of times is bigger than bcdDigits
  // Optimize by doing shifts instead of MULT
  if(p_times / bcdDigits)
  {
    int shifts = p_times / bcdDigits;
    p_times   %= bcdDigits;
    while(shifts--)
    {
      ShiftLeft();
    }
  }
  while(p_times--)
  {
    long carry   = 0;

    // Multiply all positions by 10
    for(int ind = bcdLength -1; ind >= 0; --ind)
    {
      long between    = m_mantissa[ind] * 10 + carry;
      m_mantissa[ind] = between % bcdBase;
      carry           = between / bcdBase;
    }
  }
}

// bcd::Div10
// Description: Divide the mantissa by 10
// Technical:   Optimize by doing shifts
//              Pure internal operation for manipulating the mantissa
void
bcd::Div10(int p_times /*=1*/)
{
  // if the number of times is bigger than bcdDigits
  // optimize by doing shifts instead of divs
  if(p_times / bcdDigits)
  {
    int shifts = p_times / bcdDigits;
    p_times   %= bcdDigits;
    while(shifts--)
    {
      ShiftRight();
    }
  }
  while(p_times--)
  {
    long carry   = 0;

    for(int ind = 0; ind < bcdLength; ++ind)
    {
      long between    = m_mantissa[ind] + (carry * bcdBase);
      carry           = between % 10;
      m_mantissa[ind] = between / 10;
    }
  }
}

// bcd::ShiftRight
// Description: Shift the mantissa members one position right
void
bcd::ShiftRight()
{
  for(int ind = bcdLength - 1; ind > 0; --ind)
  {
    m_mantissa[ind] = m_mantissa[ind - 1];
  }
  m_mantissa[0] = 0;
}

// bcd::ShiftLeft
// Description: Shift the mantissa members one position left
void
bcd::ShiftLeft()
{
  for(int ind = 0; ind < bcdLength - 1; ++ind)
  {
    m_mantissa[ind] = m_mantissa[ind + 1];
  }
  m_mantissa[bcdLength - 1] = 0;
}

// bcd::LongNaarString
XString
bcd::LongToString(long p_value) const
{
  TCHAR buffer[20];
  _itot_s(p_value,buffer,20,10);
  return XString(buffer);
}

// bcd::StringNaarLong
// Description: Convert a string to a single long value
long
bcd::StringToLong(LPCTSTR p_string) const
{
  return _ttoi(p_string);
}

// bcd::SplitMantissa
// Description: Split the mantissa for floor/ceiling operations
bcd
bcd::SplitMantissa() const
{
  bcd result = *this;

  // Splitting position is 1 more than the exponent
  // because of the implied first position
  int position = m_exponent + 1;

  for(int mantpos = 0;mantpos < bcdLength; ++mantpos)
  {
    if(position <= 0)
    {
      // Result after this position will be zero
      result.m_mantissa[mantpos] = 0;
    }
    else if(position >= bcdDigits)
    {
      // Keep these numbers in the mantissa
      position -= bcdDigits;
    }
    else
    {
      // Here we will split the bcdDigits somewhere in the middle
      long base   = bcdBase / 10;
      long number = result.m_mantissa[mantpos];

      for(int pos = bcdDigits; pos > 0; --pos)
      {
        // Find next number
        long num = number / base;
        number  %= base;

        // zero out ?
        if(position <= 0)
        {
          result.m_mantissa[mantpos] -= (num * base);
        }
        // go to the next position
        --position;
        base /= 10;
      }
    }
  }
  return result;
}

// bcd::CompareMantissa
// Description: Compare two mantissa
// Technical:   returns the following values
//              -1  p_value is bigger
//              0   mantissa are equal
//              1   this is is bigger
int   
bcd::CompareMantissa(const bcd& p_value) const
{
  // Now compare the mantissa
  for(int ind = 0;ind < bcdLength; ++ind)
  {
    // Find the first position not equal to the other
    if(m_mantissa[ind] != p_value.m_mantissa[ind])
    {
      // Result by comparing the mantissa positions
      return (m_mantissa[ind] > p_value.m_mantissa[ind]) ? 1 : -1;
    }
  }
  // Mantissa are equal
  return 0;
}

#ifdef _DEBUG
// Debug print of the mantissa
XString
bcd::DebugPrint(PTCHAR p_name)
{
  XString debug;

  // Print the debug name
  debug.Format(_T("%-14s "),p_name);

  // Print the sign
  debug.AppendFormat(_T("%c "),m_sign == Sign::Positive ? '+' : '-');

  // Print the exponent
  debug.AppendFormat(_T("E%+d "),m_exponent);

  // Print the mantissa in special format
  for(int ind = 0;ind < bcdLength; ++ind)
  {
    // Text "%08ld" dependent on bcdDigits
    debug.AppendFormat(_T(" %08ld"),m_mantissa[ind]);
  }
  debug += _T("\n");

  return debug;
}
#endif

// bcd::Epsilon
// Description: Stopping criterion for iterations
// Technical:   Translates fraction to lowest decimal position
//               10 -> 0.0000000000000000000000000000000000000010
//                5 -> 0.0000000000000000000000000000000000000005
bcd&
bcd::Epsilon(long p_fraction) const
{
  // Calculate stop criterion epsilon
  static bcd epsilon;
  epsilon.m_mantissa[0] = p_fraction * bcdBase / 10;
  epsilon.m_exponent    = 2 - bcdPrecision;
  return epsilon;
}

// Calculate the precision and scale for a SQL_NUMERIC
// Highly optimized version as we do this a lot when
// streaming bcd numbers to the database
void
bcd::CalculatePrecisionAndScale(SQLCHAR& p_precision,SQLCHAR& p_scale) const
{
  // Default max values
  p_precision = bcdDigits * bcdLength;
  p_scale     = 0;

  // Quick check on zero
  if(IsZero())
  {
    p_precision = 1;
    return;
  }

  int index;
  // Find the first non-zero mantissa digit
  for(index = bcdLength - 1;index >= 0; --index)
  {
    if(m_mantissa[index] == 0)
    {
      p_precision -= bcdDigits;
    }
    else break;
  }
  if(index < 0)
  {
    return;
  }
  // Find the number of digits in this mantissa
  // Change this optimalization when changing bcdDigits or bcdLength !!
  if(m_mantissa[index] % 10000)
  {
    // Lower half filled
    if(m_mantissa[index] % 100)
    {
      // 7 or 8 digits
      p_precision -= (m_mantissa[index] % 10) ? 0 : 1;
    }
    else
    {
      // 5 or 6 digits
      p_precision -= (m_mantissa[index] % 1000) ? 2 : 3;
    }
  }
  else
  {
    // Lower half is empty
    if(m_mantissa[index] % 1000000)
    {
      // 3 or 4 digits
      p_precision -= (m_mantissa[index] % 100000) ? 4 : 5;
    }
    else
    {
      // just two digits
      p_precision -= (m_mantissa[index] % 10000000) ? 6 : 7;
    }
  }
  // Final check on maximum precision
  // Cannot exceed SQLNUM_MAX_PREC as otherwise 
  // we **will** crash on certain RDBMS (MS SQL-Server)
  if(p_precision > SQLNUM_MAX_PREC)
  {
    p_precision = SQLNUM_MAX_PREC;
  }

  // Now adjust the scale to accommodate precision and exponent
  // m_exponent always below (-)SQLNUM_MAX_PREC, so this is safe
  if(m_exponent < 0)
  {
    p_scale      = p_precision - (SQLCHAR) m_exponent - 1;
    p_precision -= (SQLCHAR) m_exponent;
  }
  else
  {
    if((p_precision - 1) > (SQLCHAR) m_exponent)
    {
      p_scale = p_precision - (SQLCHAR) m_exponent - 1;
    }
    if(p_precision <= m_exponent)
    {
      p_precision = (SQLCHAR) m_exponent + 1;
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//
// END OF INTERNALS OF BCD
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// BASIC OPERATIONS OF BCD
//
//////////////////////////////////////////////////////////////////////////

// Addition operation
bcd 
bcd::Add(const bcd& p_number) const 
{
  // Check if we can add
  if(!IsValid() || !p_number.IsValid())
  {
    return SetInfinity(_T("Cannot add to INFINITY"));
  }
  // NULL always yield a NULL
  if(IsNULL() || p_number.IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  // See if we must do addition or subtraction
  // Probably we need to swap the arguments....
  // (+x) + (+y) -> Addition,    result positive, Do not swap
  // (+x) + (-y) -> Subtraction, result pos/neg,  Possibly swap
  // (-x) + (+y) -> Subtraction, result pos/neg,  Possibly swap
  // (-x) + (-y) -> Addition,    result negative, Do not swap
  Sign     signResult   = Sign::Positive;
  Operator operatorKind = Operator::Addition;
  bcd      arg1(*this);
  bcd      arg2(p_number);
  PositionArguments(arg1, arg2, signResult, operatorKind);

  if (operatorKind == Operator::Addition)
  {
    arg1 = PositiveAddition(arg1, arg2);
  }
  else
  {
    if(arg1 > arg2)
    {
      arg1 = PositiveSubtraction(arg1, arg2);
    }
    else
    {
      arg1 = PositiveSubtraction(arg2, arg1);
    }
  }
  arg1.m_sign = signResult;

  return arg1;
}

// Subtraction operation
bcd 
bcd::Sub(const bcd& p_number) const 
{
  // Check if we can subtract
  if(!IsValid() || !p_number.IsValid())
  {
    return SetInfinity(_T("Cannot subtract with INFINITY"));
  }
  // NULL always yield a NULL
  if(IsNULL() || p_number.IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  // x-y is equal to  x+(-y)
  return *this + (-p_number);
}

// Multiplication
bcd 
bcd::Mul(const bcd& p_number) const 
{
  // Check if we can multiply
  if(!IsValid() || !p_number.IsValid())
  {
    return SetInfinity(_T("Cannot multiply with INFINITY"));
  }
  // NULL always yield a NULL
  if(IsNULL() || p_number.IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  // Multiplication without signs
  bcd result = PositiveMultiplication(*this,p_number);

  // Take care of the sign
  result.m_sign = result.IsZero() ? Sign::Positive : CalculateSign(*this, p_number);

  return result;
}

// Division
bcd 
bcd::Div(const bcd& p_number) const 
{
  // Check if we can divide
  if(!IsValid() || !p_number.IsValid())
  {
    return SetInfinity(_T("Cannot divide with INFINITY"));
  }
  // NULL always yield a NULL
  if(IsNULL() || p_number.IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  // If divisor is zero -> ERROR
  if(p_number.IsZero())
  {
    return SetInfinity(_T("BCD: Division by zero."));
  }
  // Shortcut: result is zero if this is zero
  if(IsZero())
  {
    return *this;
  }
  // Division without signs
  bcd arg1(*this);
  bcd arg2(p_number);
  bcd result = PositiveDivision(arg1,arg2);

  // Take care of the sign
  result.m_sign = result.IsZero() ? Sign::Positive : CalculateSign(*this, p_number);

  return result;
}

// Modulo
bcd 
bcd::Mod(const bcd& p_number) const 
{
  // Check if we can do a modulo
  if(!IsValid() || !p_number.IsValid())
  {
    return SetInfinity(_T("Cannot do a modulo with INFINITY"));
  }
  // NULL always yield a NULL
  if(IsNULL() || p_number.IsNULL())
  {
    return bcd(Sign::ISNULL);
  }
  bcd count = ((*this) / p_number).Floor();
  bcd mod((*this) - (count * p_number));

  if (m_sign == Sign::Negative)
  {
    mod = -mod;
  }
  return mod;
}

// Position the arguments for a positive addition or subtraction
// Only called from within Add()
void
bcd::PositionArguments(bcd&       arg1,
                       bcd&       arg2,
                       Sign&      signResult,
                       Operator&  operatorKind) const
{
  // Get the resulting kind of operator and sign
  // if (-x) + y then turnaround, so x + (-y), becomes x - y
  if (arg1.m_sign == Sign::Positive)
  {
    if(arg2.m_sign == Sign::Positive)
    {
      // Both are positive
      signResult   = Sign::Positive;
      operatorKind = Operator::Addition;
    }
    else
    {
      // Arg2 is negative
      // Now the rest is positive
      arg1.m_sign  = Sign::Positive;
      arg2.m_sign  = Sign::Positive;
      operatorKind = Operator::Subtraction;
      // Sign depends on the size
      signResult   = (arg1 >= arg2) ? Sign::Positive : Sign::Negative;
    }
  }
  else // arg1.m_sign == Negative
  {
    if(arg2.m_sign == Sign::Negative)
    {
      // Both are negative
      arg1.m_sign  = Sign::Positive;
      arg2.m_sign  = Sign::Positive;
      signResult   = Sign::Negative;
      operatorKind = Operator::Addition;
    }
    else
    {
      // arg2 is positive
      // Now the rest is positive
      arg1.m_sign = Sign::Positive;
      arg2.m_sign = Sign::Positive;
      operatorKind = Operator::Subtraction;
      // Sign depends on the size
      signResult  = (arg2 >= arg1) ? Sign::Positive : Sign::Negative;
    }
  }
}

bcd::Sign
bcd::CalculateSign(const bcd& p_arg1, const bcd& p_arg2) const
{
  // Find the sign for multiplication / division
  // (+x) * (+y) -> positive
  // (-x) * (+y) -> negative
  // (-x) * (-y) -> positive
  // (+x) * (-y) -> negative
  if (p_arg1.IsZero() || p_arg2.IsZero())
  {
    return Sign::Positive;
  }
  if(p_arg1.m_sign != p_arg2.m_sign)
  {
    return Sign::Negative;
  }
  return Sign::Positive;
}

// Addition of two mantissa (no signs/exponents)
bcd
bcd::PositiveAddition(bcd& arg1,bcd& arg2) const
{
  // Take care of the exponents
  if(arg1.m_exponent != arg2.m_exponent)
  {
    // If numbers differ more than this, an addition is useless
    int border = bcdDigits * bcdLength;

    if(arg1.m_exponent > arg2.m_exponent)
    {
      int shift = arg1.m_exponent - arg2.m_exponent;
      if(shift > border)
      {
        // Adding arg2 will not result in a difference
        return arg1;
      }
      // Shift arg2 to the right;
      arg2.Div10(shift);
    }
    else
    {
      int shift = arg2.m_exponent - arg1.m_exponent;
      if(shift > border)
      {
        // Adding arg1 will not result in a difference
        return arg2;
      }
      // Shift arg1 to the right
      arg1.Div10(shift);
      arg1.m_exponent += (short) shift;
    }
  }
  // Do the addition of the mantissa
  int64 carry = 0L;
  for(int ind = bcdLength - 1;ind >= 0; --ind)
  {
    int64 reg = ((int64)arg1.m_mantissa[ind]) + ((int64)arg2.m_mantissa[ind]) + carry;
    carry = reg / bcdBase;
    arg1.m_mantissa[ind] = reg % bcdBase;
  }
  // Take care of carry
  if(carry)
  {
    arg1.Div10();
    arg2.Div10();
    arg1.m_exponent++;
    arg1.m_mantissa[0] += (long)(carry * (bcdBase / 10));
  }
  return arg1;
}

// Subtraction of two mantissa (no signs/exponents)
// Precondition arg1 > arg2
bcd
bcd::PositiveSubtraction(bcd& arg1,bcd& arg2) const
{
  // Take care of the exponents
  if(arg1.m_exponent != arg2.m_exponent)
  {
    // If numbers differ more than this, an addition is useless
    int border = bcdDigits * bcdLength;

    int shift = arg1.m_exponent - arg2.m_exponent;
    if(shift > border)
    {
      // Adding arg2 will not result in a difference
      return arg1;
    }
    if (shift > 0)
    {
      // Shift arg2 to the right;
      arg2.Div10(shift);
    }
    else
    {
      arg2.Mult10(-shift);
    }
  }
  // Do the subtraction of the mantissa
  for(int ind = bcdLength - 1;ind >= 0; --ind)
  {
    if(arg1.m_mantissa[ind] >= arg2.m_mantissa[ind])
    {
      arg1.m_mantissa[ind] -= arg2.m_mantissa[ind];
    }
    else
    {
      int64 reg = ((int64)bcdBase + arg1.m_mantissa[ind]) - arg2.m_mantissa[ind];
      arg1.m_mantissa[ind] = (long) reg;
      // Take care of carry
      if(ind > 0)
      {
        arg1.m_mantissa[ind - 1] -= 1;
      }
    }
  }
  // Normalize the mantissa/exponent
  arg1.Normalize();

  // Return the result
  return arg1;
}

// bcd::PositiveMultiplication
// Description: Multiplication of two mantissa (no signs)
// Technical:   1) addition of the exponents
//              2) multiplication of the mantissa
//              3) take-in carry and normalize
bcd
bcd::PositiveMultiplication(const bcd& p_arg1,const bcd& p_arg2) const
{
  bcd result;
  int64 res[2 * bcdLength] = { 0 };

  // Multiplication of the mantissa
  for(int i = bcdLength - 1; i >= 0; --i)
  {
    for(int j = bcdLength - 1; j >= 0; --j)
    {
      int64 between = (int64)p_arg1.m_mantissa[i] * (int64)p_arg2.m_mantissa[j];
      res[i + j + 1] += between % bcdBase; // result
      res[i + j    ] += between / bcdBase; // carry
    }
  }

  // Normalize resulting mantissa to bcdBase
  int64 carry   = 0;
  for(int ind = (2 * bcdLength) - 1;ind >= 0; --ind)
  {
    res[ind] += carry;
    carry     = res[ind] / bcdBase;
    res[ind] %= bcdBase;
  }

  // Possibly perform rounding of res[bcdLength] -> res[bcdLength-1]

  // Put the resulting mantissa's in the result
  for(int ind = 0; ind < bcdLength; ++ind)
  {
    result.m_mantissa[ind] = (long)res[ind];
  }

  // Take care of exponents
  // Multiplication is the addition of the exponents
  // Add 1 for the implied 1 position of the second argument
  result.m_exponent = p_arg1.m_exponent + p_arg2.m_exponent + 1;

  // Normalize to implied dec.point position
  result.Normalize();

  return result;
}

// bcd::PositiveDivision
// Description: Division of two mantissa (no signs)
// Technical:   Do traditional 'long division' of two bcd's
bcd
bcd::PositiveDivision(bcd& p_arg1,bcd& p_arg2) const
{
  bcd   result;
  short result_mantissa[bcdPrecision + 1] = { 0 };
  bcd   subtrahend;

  // Division of the mantissa
  long dividend  = 0;
  long divisor   = 0; 
  long quotient  = 0;
  int  guess     = 2;

  // Grade down arg2 one position
  p_arg2.Div10();

  for(int ind = 1;ind <= bcdPrecision; ++ind)
  {
//     printf("Iteration: %d\n",ind);
//     p_arg1.DebugPrint("argument1");
//     p_arg2.DebugPrint("argument2");

    // Check for intermediate of zero. Arg1 != zero, so it must end!!
    bool zero = true;
    for(int x = 0;x < bcdLength; ++x)
    {
      if(p_arg1.m_mantissa[x]) 
      {
        zero = false;
        break;
      }
    }
    // arg1 has become zero. Nothing more to do
    if(zero)
    {
      break;
    }
    // If not at implied position: Mantissa can shift 1 position up
    if((p_arg1.m_mantissa[0] * 10 / bcdBase) == 0)
    {
      p_arg1.Mult10();

      // If still not at implied position, we may do an extra shifts 
      // inserting zero's in the result if there are more
      if((p_arg1.m_mantissa[0] * 10 / bcdBase) == 0)
      {
        do
        {
          result_mantissa[ind++] = 0;
          p_arg1.Mult10();
          if(ind > bcdPrecision)
          {
            // Escape from double loop (for/while)
            goto end;
          }
        }
        while((p_arg1.m_mantissa[0] * 10 / bcdBase) == 0);
      }
    }

//     // Resulting mantissa's 
//     p_arg1.DebugPrint("argument1-2");
//     p_arg2.DebugPrint("argument2-2");

    // get quotient
    dividend = p_arg1.m_mantissa[0];
    divisor  = p_arg2.m_mantissa[0];
    quotient = dividend / divisor;

    // Try to get a subtrahend
    // Could go wrong because the total of quotient * arg2.mantissa can be > than arg1.mantissa
    guess = 2;
    while(guess--)
    {
      // quotient * p_arg2 -> subtrahend
      int64 carry  = 0;
      for(int pos = bcdLength - 1; pos >= 0; --pos)
      {
        int64 number = (int64)quotient * (int64)p_arg2.m_mantissa[pos] + carry;
        subtrahend.m_mantissa[pos] = number % bcdBase;
        carry = number / bcdBase;
      }
//    // Effective subtrahend
//    subtrahend.DebugPrint("subtrahend");

      // if(p_arg1 < subtrahend)
      if(p_arg1.CompareMantissa(subtrahend) == -1)
      {
        --quotient;
      }
      else
      {
        // Effective quotient found for this 'ind' step
//      printf("%02d quotient: %d\n",ind,quotient);

        // Ready guessing: quotient is OK
        guess = 0;
        result_mantissa[ind] = (short)quotient;
        // arg1 = arg1 - subtrahend
        for(int pos = bcdLength - 1;pos >= 0; --pos)
        {
          if(p_arg1.m_mantissa[pos] >= subtrahend.m_mantissa[pos])
          {
            p_arg1.m_mantissa[pos] -= subtrahend.m_mantissa[pos];
          }
          else
          {
            int64 number = ((int64)bcdBase + p_arg1.m_mantissa[pos]) - subtrahend.m_mantissa[pos];
            p_arg1.m_mantissa[pos] = (long)number;
            if(pos > 0)
            {
              // Do the borrow
              p_arg1.m_mantissa[pos - 1] -= 1;
            }
          }
        }
      }
//    // Effective difference (minuend - subtrahend)
//    p_arg1.DebugPrint("argument1-3");
    }
  }
end:
  // Normalize result
  short carry = 0;
  for(int i = bcdPrecision; i > 0; --i)
  {
    result_mantissa[i] += carry;
    carry               = result_mantissa[i] / 10;
    result_mantissa[i] %= 10;
  }
  result_mantissa[0] = carry;

  // Get starting position of the answer
  int respos = (carry == 0) ? 1 : 0;

  // Set the result_mantissa in the result number
  for(int mantpos = 0;mantpos < bcdLength; ++mantpos)
  {
    long number = 0;
    long base   = bcdBase / 10;

    for(int ind = 0; ind < bcdDigits; ++ind)
    {
      number += result_mantissa[respos++] * base;
      base   /= 10;
    }
    result.m_mantissa[mantpos] = number;
  }

  // Subtraction of the exponents
  // Subtract 1 for initial Div10 of the second argument
  result.m_exponent = p_arg1.m_exponent - p_arg2.m_exponent - 1;

  // If we had a carry, the exponent will be 1 higher
  if(carry)
  {
    result.m_exponent++;
  }

  // Return the result
  return result;
}

// On overflow we set negative or positive infinity
bcd
bcd::SetInfinity(XString p_reason /*= ""*/) const
{
  if(g_throwing)
  {
    throw StdException(p_reason);
  }
  // NaN AND previous infinity is set to positive infinity !!
  bcd inf;
  inf.m_sign = (m_sign == Sign::Negative) ? Sign::MIN_INF : Sign::INF;
  return inf;
}

//////////////////////////////////////////////////////////////////////////
//
// END OF BASIC OPERATIONS OF BCD
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// OVERLOADED MATH FUNCTIONS (Everybody has come to expect)
//
//////////////////////////////////////////////////////////////////////////

// Overloaded math precision floating point functions equivalent with the std C functions
// Overloaded to work with the BCD number class, always yielding a bcd number.

bcd modf(const bcd& p_number, bcd* p_intpart)
{
  *p_intpart = p_number.Floor();
  return p_number.Fraction();
}

bcd fmod(const bcd& p_number,const bcd& p_divisor)
{
  return p_number % p_divisor;
}

bcd floor(const bcd& p_number)
{
  return p_number.Floor();
}

bcd ceil(const bcd& p_number)
{
  return p_number.Ceiling();
}

bcd fabs(const bcd& p_number)
{
  return p_number.AbsoluteValue();
}

bcd sqrt(const bcd& p_number)
{
  return p_number.SquareRoot();
}

bcd log10(const bcd& p_number)
{
  return p_number.Log10();
}

bcd log(const bcd& p_number)
{
  return p_number.Log();
}

bcd exp(const bcd& p_number)
{
  return p_number.Exp();
}

bcd pow(const bcd& p_number,const bcd& p_power)
{
  return p_number.Power(p_power);
}

bcd frexp(const bcd& p_number,int* p_exponent)
{
  *p_exponent = p_number.GetExponent();
  return p_number.GetMantissa();
}

bcd ldexp(const bcd& p_number,int p_power)
{
  if(p_power == 0)
  {
    return p_number;
  }
  if(p_power > 0 && p_power <= 31)
  {
    return p_number * bcd((long) (((unsigned)1) << p_power));
  }
  return p_number * pow(bcd(2L),bcd((long)p_power));
}

// Overloaded trigonometric functions on a bcd number

bcd atan (const bcd& p_number) 
{ 
  return p_number.ArcTangent(); 
}

bcd atan2(const bcd& p_y,const bcd& p_x)
{
  return p_y.ArcTangent2Points(p_x);
}

bcd asin(const bcd& p_number)
{
  return p_number.ArcSine();
}

bcd acos(const bcd& p_number)
{
  return p_number.ArcCosine();
}

bcd sin(const bcd& p_number)
{
  return p_number.Sine();
}

bcd cos(const bcd& p_number)
{
  return p_number.Cosine();
}

bcd tan(const bcd& p_number)
{
  return p_number.Tangent();
}

//////////////////////////////////////////////////////////////////////////
//
// END OF OVERLOADED MATH FUNCTIONS
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// FILE STREAM FUNCTIONS
// Used for binary storage in a binary-mode file stream!
//
//////////////////////////////////////////////////////////////////////////

bool
bcd::WriteToFile (FILE* p_fp)
{
  // Write out the sign
  if(putc((char)m_sign,p_fp)      == EOF) return false;
  // Write out the exponent (little endian)
  if(putc(m_exponent >> 8,  p_fp) == EOF) return false;
  if(putc(m_exponent & 0xFF,p_fp) == EOF) return false;
  // Write out the mantissa (little endian)
  for(unsigned int ind = 0;ind < bcdLength; ++ind)
  {
    ulong num = (ulong) m_mantissa[ind];
    if(putc((num & 0xFF000000) >> 24,p_fp) == EOF) return false;
    if(putc((num & 0x00FF0000) >> 16,p_fp) == EOF) return false;
    if(putc((num & 0x0000FF00) >>  8,p_fp) == EOF) return false;
    if(putc((num & 0x000000FF)      ,p_fp) == EOF) return false;
  }
  return true;
}

bool
bcd::ReadFromFile(FILE* p_fp)
{
  int ch = 0;

  // Read in the sign
  m_sign = (Sign) getc(p_fp);
  // Read in the exponent
  ch = getc(p_fp);
  m_exponent = (short) (ch << 8);
  ch = getc(p_fp);
  m_exponent += (short) ch;

  // Read in the mantissa
  for(unsigned int ind = 0; ind < bcdLength; ++ind)
  {
    ulong num = 0L;
    ch = getc(p_fp); num += ((ulong)ch) << 24;
    ch = getc(p_fp); num += ((ulong)ch) << 16;
    ch = getc(p_fp); num += ((ulong)ch) <<  8;
    ch = getc(p_fp); num +=  (ulong)ch;
    m_mantissa[ind] = num;
  }

  // BCD is invalid in case of file error
  if(ferror(p_fp))
  {
    Zero();
    return false;
  }
  return true;
}

//////////////////////////////////////////////////////////////////////////
//
// END OF FILE STREAM FUNCTIONS
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// END OF BCD IMPLEMENTATION
//
//////////////////////////////////////////////////////////////////////////
