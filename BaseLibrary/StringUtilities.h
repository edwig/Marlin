#pragma once
#include <vector>

XString AsString(int p_number,int p_radix = 10);
XString AsString(double p_number);

int     AsInteger(XString p_string);
double  AsDouble(XString p_string);
bcd     AsBcd(XString p_string);

int     SplitString(XString p_string,std::vector<XString>& p_vector,char p_separator = ',',bool p_trim = false);
void    NormalizeLineEndings(XString& p_string);
