/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestCrypto.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2016 ir. W.E. Huisman
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
#include "stdafx.h"
#include "TestClient.h"
#include "Crypto.h"
#include <conio.h>
#include <wincrypt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int TestHashing()
{
  Crypto  crypt;
  CString hash;
  CString password("P@$$w03d4m3"); // Password for me
  CString buffer("This is a somewhat longer string with an intentional message");
  int     errors = 0;

  // Dit gaan we testen
   printf("TESTING HASHING PROVIDERS AND METHODS OF MS-Cryptographic-providers\n");
   
   printf("===================================================================\n");
  xprintf("DIGEST Password: %s\n",password);
  xprintf("DIGEST Buffer  : %s\n",buffer);
  xprintf("-------------------------------------------------------------------\n");

  // TEST 1
  hash.Empty();
  crypt.SetHashMethod(CALG_SHA1);
  hash = crypt.Digest(buffer, password);
  CString expected1("9M9TaD0gWVy4qwVSeN5DEzMYQFE=");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_SHA1\n");
  xprintf("DIGEST Expected: %s\n",expected1);
  xprintf("DIGEST hashval : %s\n",hash);
  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  printf("TEST HASH 1 - SHA1          : %s\n",(hash == expected1) ? "OK" : "ERROR");
  errors = (hash == expected1) ? 0 : 1;

  // TEST 2
  hash.Empty();
  crypt.SetHashMethod(CALG_HMAC);
  hash = crypt.Digest(buffer,password);
  CString expected2("9M9TaD0gWVy4qwVSeN5DEzMYQFE=");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_HMAC\n");
  xprintf("DIGEST Expected: %s\n", expected2);
  xprintf("DIGEST hashval : %s\n", hash);
  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  printf("TEST HASH 2 - HMAC          : %s\n",(hash == expected2) ? "OK" : "ERROR");
  errors += (hash == expected2) ? 0 : 1;

  // TEST 3
  hash.Empty();
  crypt.SetHashMethod(CALG_DSS_SIGN);
  hash = crypt.Digest(buffer,password);
  CString expected3("onrhIV34nZGqoThg6380Xvk1rT0=");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_DSS_SIGN\n");
  xprintf("DIGEST Expected: %s\n", expected3);
  xprintf("DIGEST hashval : %s\n", hash);
  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  printf("TEST HASH 3 - DSS_SIGN      : %s\n",(hash == expected3) ? "OK" : "ERROR");
  errors += (hash == expected3) ? 0 : 1;

  // TEST 4
  hash.Empty();
  crypt.SetHashMethod(CALG_RSA_SIGN);
  hash = crypt.Digest(buffer, password);
  CString expected4("PtUUjwUxC2BrYogfGWBqIB4lUsGQK76PxBolbL8nAdk=");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_RSA_SIGN\n");
  xprintf("DIGEST Expected: %s\n", expected4);
  xprintf("DIGEST hashval : %s\n", hash);
  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  printf("TEST HASH 4 - RSA_SIGN      : %s\n",(hash == expected4) ? "OK" : "ERROR");
  errors += (hash == expected4) ? 0 : 1;

  // TEST 5
  hash.Empty();
  crypt.SetHashMethod(CALG_MD2);
  hash = crypt.Digest(buffer, password);
  CString expected5("Tbf0ESz3tTz/PqhHrJgc1g==");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_MD2\n");
  xprintf("DIGEST Expected: %s\n", expected5);
  xprintf("DIGEST hashval : %s\n", hash);
  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  printf("TEST HASH 5 - CALG_MD2      : %s\n",(hash == expected5) ? "OK" : "ERROR");
  errors += (hash == expected5) ? 0 : 1;

  // TEST 6
  hash.Empty();
  crypt.SetHashMethod(CALG_MD4);
  hash = crypt.Digest(buffer, password);
  CString expected6("po1oCGriWfvikcPUEBQ/Ow==");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_MD4\n");
  xprintf("DIGEST Expected: %s\n", expected6);
  xprintf("DIGEST hashval : %s\n", hash);
  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  printf("TEST HASH 6 - CALG_MD4      : %s\n",(hash == expected6) ? "OK" : "ERROR");
  errors += (hash == expected6) ? 0 : 1;

  // TEST 7
  hash.Empty();
  crypt.SetHashMethod(CALG_MD5);
  hash = crypt.Digest(buffer, password);
  CString expected7("03Ka00wHVBoHj1hzpontgg==");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_MD5\n");
  xprintf("DIGEST Expected: %s\n",expected7);
  xprintf("DIGEST hashval : %s\n", hash);
  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  printf("TEST HASH 7 - CALG_MD5      : %s\n",(hash == expected7) ? "OK" : "ERROR");
  errors += (hash == expected7) ? 0 : 1;

  // TEST 8
  hash.Empty();
  crypt.SetHashMethod(CALG_SHA_384);
  hash = crypt.Digest(buffer, password);
  CString expected8("B/W+IoBdT6Z9YNX584ixk3kaqQBtEXIYVs+O3teFc3z7sj/6oo1OlEkm6UOSUFzE");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_SHA_384\n");
  xprintf("DIGEST Expected: %s\n",expected8);
  xprintf("DIGEST hashval : %s\n", hash);
  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  printf("TEST HASH 8 - SHA-384       : %s\n",(hash == expected8) ? "OK" : "ERROR");
  errors += (hash == expected8) ? 0 : 1;

  // TEST 9
  hash.Empty();
  crypt.SetHashMethod(CALG_SHA_512);
  hash = crypt.Digest(buffer, password);
  CString expected9("12Sw4ChvtCYQ2go9h39LArObGfUUmwtFatUTxfvN5e77w9SdsjzGpymhk0jS8CgZKzhkfvWmfWnSaHKFIvs7fQ==");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_SHA_512\n");
  xprintf("DIGEST Expected: %s\n", expected9);
  xprintf("DIGEST hashval : %s\n", hash);
  xprintf("\n");
  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  printf("TEST HASH 9 - SHA-512       : %s\n",(hash == expected9) ? "OK" : "ERROR");
  errors += (hash == expected9) ? 0 : 1;

  return errors;
}

int  TestEncryption(void)
{
  Crypto crypt;
  CString result;
  CString password("P@$$w03d4m3"); // Password for me
  CString buffer("This is a somewhat longer string with an intentional message");

  // Dit gaan we testen
   printf("TESTING ENCRYPTION PROVIDERS AND METHODS OF MS-Cryptographic-providers\n");
   printf("======================================================================\n");
  xprintf("DIGEST Password: %s\n", password);
  xprintf("DIGEST Buffer  : %s\n", buffer);
  xprintf("----------------------------------------------------------------------\n");
  xprintf("\n");

  xprintf("Provider       : PROV_RSA_AES\n");
  xprintf("Password-hash  : CALG_SHA_256\n");
  xprintf("Encryptie      : CALG_AES_256\n");
  xprintf("\n");


  result = crypt.Encryptie(buffer,password);

  xprintf("Expected       : iCby8h4MjErrm3rj4EwKsKVb5/Gt+EyNrDwE2ZG7pAqTgvzZwAMZ+D0045WhM2+5NqVYBrdGQmr+Xhn/ufq5Kw==\n");
  xprintf("ENCRYPTION     : %s\n",result);

  result = crypt.Decryptie(result,password);

  xprintf("DECRYPTION     : %s\n",result);
  xprintf("\n");

  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  printf("TEST ENCRYPTION AES-256     : %s\n",(result == buffer) ? "OK" : "ERROR");

  return (result == buffer) ? 0 : 1;
}

int  TestCryptography(void)
{
  int errors = 0;

  errors += TestHashing();
  errors += TestEncryption();

  return errors;
}