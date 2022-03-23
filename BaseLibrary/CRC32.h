/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: CRC32.h
//
// BaseLibrary: Indispensable general objects and functions
// 
// This function is not written by me, but is copied from the Microsoft open specification
// which can be found in the documentation for MS-Office file formats at:
// 
// https://docs.microsoft.com/en-us/openspecs/office_file_formats/ms-pst/39c35207-130f-4d83-96f8-2b311a285a8f
//
#pragma once

DWORD ComputeCRC32(DWORD dwCRC,LPCVOID pv,UINT cbLength);

