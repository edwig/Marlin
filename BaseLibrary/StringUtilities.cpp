#include "pch.h"
#include "StringUtilities.h"

XString AsString(int p_number,int p_radix /*=10*/)
{
  XString string;
  char* buffer = string.GetBufferSetLength(12);
  _itoa_s(p_number,buffer,12,p_radix);
  string.ReleaseBuffer();
  
  return string;
}

XString AsString(double p_number)
{
  XString string;
  string.Format("%f",p_number);
  return string;
}

int AsInteger(XString p_string)
{
  return atoi(p_string.GetBuffer());
}

double  AsDouble(XString p_string)
{
  return atof(p_string.GetBuffer());
}

bcd AsBcd(XString p_string)
{
  bcd num(p_string);
  return num;
}

int SplitString(XString p_string,std::vector<XString>& p_vector,char p_separator /*= ','*/,bool p_trim /*=false*/)
{
  p_vector.clear();
  int pos = p_string.Find(p_separator);
  while(pos >= 0)
  {
    XString first = p_string.Left(pos);
    p_string = p_string.Mid(pos + 1);
    if(p_trim)
    {
      first = first.Trim();
    }
    p_vector.push_back(first);
    // Find next string
    pos = p_string.Find(p_separator);
  }
  if(p_trim)
  {
    p_string = p_string.Trim();
  }
  if(p_string.GetLength())
  {
    p_vector.push_back(p_string);
  }
  return (int)p_vector.size();
}
