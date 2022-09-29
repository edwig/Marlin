/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Namespace.h
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

// Compares two namespaces
// Returns strcmp values
int CompareNamespaces(XString p_namespace1,XString p_namespace2);

// Can be used to split SOAPAction from the HTTP protocol
// or from the Soap envelope <Action>  node
bool SplitNamespaceAndAction(XString p_soapAction,XString& p_namespace,XString& p_action,bool p_nmsp_ends_in_slash = false);

// Concatenate namespace and action to a soapaction entry
// Can be used in HTTP and in SOAP messages
XString CreateSoapAction(XString p_namespace,XString p_action);

// Split namespace from an identifier
XString SplitNamespace(XString& p_name);
