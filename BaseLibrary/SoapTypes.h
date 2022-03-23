/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SOAPTypes.h
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
#pragma once

// The general SOAP Version
enum class SoapVersion
{
  SOAP_POS  = 1   // Plain Old Soap
 ,SOAP_10   = 1   // Soap version 1.0 (simple fault,no envelope + SOAPAction HTTP header)
 ,SOAP_11   = 2   // Soap version 1.1 (simple fault,   envelope + SOAPAction HTTP header)
 ,SOAP_12   = 3   // Soap version 1.2 (extended fault, envelope + Action in header)
};

// Encryption levels
// For SOAPMessage / HTTPSite alike
enum class XMLEncryption
{
   XENC_NoInit = -1
  ,XENC_Plain  = 0  // Plain XML text
  ,XENC_Signing     // Sign the body in the header
  ,XENC_Body        // Encrypt the whole body
  ,XENC_Message     // Encrypt the whole message
};
