/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestCrypto.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
#include "Base64.h"
#include <conio.h>
#include <wincrypt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

XString CreateNonce()
{
  // Init random generator
  clock_t now = clock();
  srand((unsigned int)now);

  XString nonce;
  for (int index = 0; index < 20; ++index)
  {
    long key = (rand() % 255) + 1;
    nonce += (unsigned char)key;
  }
  return nonce;
}


int TestHashing()
{
  Crypto  crypt;
  XString hash;
  XString password("P@$$w03d4m3"); // Password for me
  XString buffer("This is a somewhat longer string with an intentional message");
  int     errors = 0;

  // This is what we're testing
  xprintf("TESTING HASHING PROVIDERS AND METHODS OF MS-Cryptographic-providers\n");
   
  xprintf("===================================================================\n");
  xprintf("DIGEST Password: %s\n",password.GetString());
  xprintf("DIGEST Buffer  : %s\n",buffer.GetString());
  xprintf("-------------------------------------------------------------------\n");

  // TEST 1
  hash.Empty();
  crypt.SetHashMethod(CALG_SHA1);
  hash = crypt.Digest(buffer, password);
  XString expected1("9M9TaD0gWVy4qwVSeN5DEzMYQFE=");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_SHA1\n");
  xprintf("DIGEST Expected: %s\n",expected1.GetString());
  xprintf("DIGEST hashval : %s\n",hash.GetString());
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Cryptographic hashing provider SHA1            : %s\n",(hash == expected1) ? "OK" : "ERROR");
  errors = (hash == expected1) ? 0 : 1;

  // TEST 2
  hash.Empty();
  crypt.SetHashMethod(CALG_HMAC);
  hash = crypt.Digest(buffer,password);
  XString expected2("9M9TaD0gWVy4qwVSeN5DEzMYQFE=");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_HMAC\n");
  xprintf("DIGEST Expected: %s\n", expected2.GetString());
  xprintf("DIGEST hashval : %s\n", hash.GetString());
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Cryptographic hashing provider HMAC            : %s\n",(hash == expected2) ? "OK" : "ERROR");
  errors += (hash == expected2) ? 0 : 1;

  // TEST 3
  hash.Empty();
  crypt.SetHashMethod(CALG_DSS_SIGN);
  hash = crypt.Digest(buffer,password);
  XString expected3("onrhIV34nZGqoThg6380Xvk1rT0=");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_DSS_SIGN\n");
  xprintf("DIGEST Expected: %s\n", expected3.GetString());
  xprintf("DIGEST hashval : %s\n", hash.GetString());
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Cryptographic hashing provider DSS_SIGN        : %s\n",(hash == expected3) ? "OK" : "ERROR");
  errors += (hash == expected3) ? 0 : 1;

  // TEST 4
  hash.Empty();
  crypt.SetHashMethod(CALG_RSA_SIGN);
  hash = crypt.Digest(buffer, password);
  XString expected4("PtUUjwUxC2BrYogfGWBqIB4lUsGQK76PxBolbL8nAdk=");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_RSA_SIGN\n");
  xprintf("DIGEST Expected: %s\n", expected4.GetString());
  xprintf("DIGEST hashval : %s\n", hash.GetString());
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Cryptographic hashing provider RSA_SIGN        : %s\n",(hash == expected4) ? "OK" : "ERROR");
  errors += (hash == expected4) ? 0 : 1;

  // TEST 5
  hash.Empty();
  crypt.SetHashMethod(CALG_MD2);
  hash = crypt.Digest(buffer, password);
  XString expected5("Tbf0ESz3tTz/PqhHrJgc1g==");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_MD2\n");
  xprintf("DIGEST Expected: %s\n", expected5.GetString());
  xprintf("DIGEST hashval : %s\n", hash.GetString());
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Cryptographic hashing provider CALG_MD2        : %s\n",(hash == expected5) ? "OK" : "ERROR");
  errors += (hash == expected5) ? 0 : 1;

  // TEST 6
  hash.Empty();
  crypt.SetHashMethod(CALG_MD4);
  hash = crypt.Digest(buffer, password);
  XString expected6("po1oCGriWfvikcPUEBQ/Ow==");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_MD4\n");
  xprintf("DIGEST Expected: %s\n", expected6.GetString());
  xprintf("DIGEST hashval : %s\n", hash.GetString());
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Cryptographic hashing provider CALG_MD4        : %s\n",(hash == expected6) ? "OK" : "ERROR");
  errors += (hash == expected6) ? 0 : 1;

  // TEST 7
  hash.Empty();
  crypt.SetHashMethod(CALG_MD5);
  hash = crypt.Digest(buffer, password);
  XString expected7("03Ka00wHVBoHj1hzpontgg==");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_MD5\n");
  xprintf("DIGEST Expected: %s\n",expected7.GetString());
  xprintf("DIGEST hashval : %s\n", hash.GetString());
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Cryptograhpic hashing provider CALG_MD5        : %s\n",(hash == expected7) ? "OK" : "ERROR");
  errors += (hash == expected7) ? 0 : 1;

  // TEST 8
  hash.Empty();
  crypt.SetHashMethod(CALG_SHA_384);
  hash = crypt.Digest(buffer, password);
  XString expected8("B/W+IoBdT6Z9YNX584ixk3kaqQBtEXIYVs+O3teFc3z7sj/6oo1OlEkm6UOSUFzE");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_SHA_384\n");
  xprintf("DIGEST Expected: %s\n",expected8.GetString());
  xprintf("DIGEST hashval : %s\n", hash.GetString());
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Cryptographic hashing provider SHA-384         : %s\n",(hash == expected8) ? "OK" : "ERROR");
  errors += (hash == expected8) ? 0 : 1;

  // TEST 9
  hash.Empty();
  crypt.SetHashMethod(CALG_SHA_512);
  hash = crypt.Digest(buffer, password);
  XString expected9("12Sw4ChvtCYQ2go9h39LArObGfUUmwtFatUTxfvN5e77w9SdsjzGpymhk0jS8CgZKzhkfvWmfWnSaHKFIvs7fQ==");
  xprintf("\n");
  xprintf("DIGEST Method  : CALG_SHA_512\n");
  xprintf("DIGEST Expected: %s\n", expected9.GetString());
  xprintf("DIGEST hashval : %s\n", hash.GetString());
  xprintf("\n");
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Cryptographic hashing provider SHA-512         : %s\n",(hash == expected9) ? "OK" : "ERROR");
  errors += (hash == expected9) ? 0 : 1;

  // TEST 10
  XString nonce = CreateNonce();
  XString created("2022-04-25T10:23:55");

  // Password text = Base64(SHA1(nonce + created + password))
  XString combined = nonce + created + password;
  XString total1 = crypt.Digest(combined.GetString(),combined.GetLength(), CALG_SHA1);
  Crypto  crypt2;
  XString total2 = crypt2.Digest(combined.GetString(),combined.GetLength(), CALG_SHA1);
  errors += (total1 == total2) ? 0 : 1;

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Re-hashing with SHA1 is stable (TokenProfile)  : %s\n", (total1 == total2) ? "OK" : "ERROR");

  // TEST11
  XString nonce64("ZmJEiZTMUu7IqTVuvZPy7g==");
  nonce = Base64::Decrypt(nonce64);
  combined = nonce + "2022-04-25T11:32:22.745Z" + "MijnWachtwoord$$";
  XString total3 = crypt.Digest(combined.GetString(), combined.GetLength(), CALG_SHA1);
  XString expected10("cvVPyMJr/9W6plAwmuG2wl27akU=");
  errors += (total3 == expected10) ? 0 : 1;

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("SHA1 (TokenProfile) conforming standard        : %s\n", (total3 == expected10) ? "OK" : "ERROR");


  return errors;
}

int  TestEncryption(void)
{
  Crypto crypt;
  XString result;
  XString password("P@$$w03d4m3"); // Password for me
  XString buffer("This is a somewhat longer string with an intentional message");

  // This is what we test
  xprintf("TESTING ENCRYPTION PROVIDERS AND METHODS OF MS-Cryptographic-providers\n");
  xprintf("======================================================================\n");
  xprintf("DIGEST Password: %s\n", password.GetString());
  xprintf("DIGEST Buffer  : %s\n", buffer.GetString());
  xprintf("----------------------------------------------------------------------\n");
  xprintf("\n");

  xprintf("Provider       : PROV_RSA_AES\n");
  xprintf("Password-hash  : CALG_SHA_256\n");
  xprintf("Encryption    : CALG_AES_256\n");
  xprintf("\n");


  result = crypt.Encryption(buffer,password);

  xprintf("Expected       : iCby8h4MjErrm3rj4EwKsKVb5/Gt+EyNrDwE2ZG7pAqTgvzZwAMZ+D0045WhM2+5NqVYBrdGQmr+Xhn/ufq5Kw==\n");
  xprintf("ENCRYPTION     : %s\n",result.GetString());

  result = crypt.Decryption(result,password);

  xprintf("DECRYPTION     : %s\n",result.GetString());
  xprintf("\n");

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Test MS Cryptographic provider in AES-256 mode : %s\n",(result == buffer) ? "OK" : "ERROR");

  return (result == buffer) ? 0 : 1;
}

int TestEncryptionLongerStrings()
{
  Crypto  crypt;
  XString result;
  XString password("V3ryL0ngP@$$w03dL0ng3rTh@n16Ch@r$"); // 38 chars long!
  int     errors = 0;

  xprintf("TESTING THE MICROSOFT ENCYRPTION PROVIDER FOR ALL SORTS OF STRING\n");
  xprintf("=================================================================\n");


  for(int length = 1; length <= 3000; ++length)
  {
    // Construct original string
    XString original;
    for (int index = 0; index < length; ++index)
    {
      original += (char) ('A' + index % 26);
    }

    XString encrypted = crypt.Encryption(original, password);
    XString decrypted = crypt.Decryption(encrypted,password);

    if(original != decrypted)
    {
      ++errors;
      xprintf("Encryption failed at: %d ",length);
      xprintf("Error: %s\n",crypt.GetError().GetString());
    }
  }


  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Test MS Cryptographic provider for lengths     : %s\n", (errors == 0) ? "OK" : "ERROR");
  return errors;
}

int  TestFastEncryption(void)
{
  Crypto crypt;
  XString result;
  XString password("P@$$w03d4m3"); // Password for me
  XString buffer("This is a somewhat longer string with an intentional message");

  // This is what we test
  xprintf("TESTING ENCRYPTION PROVIDERS AND METHODS OF MS-Cryptographic-providers\n");
  xprintf("======================================================================\n");
  xprintf("DIGEST Password: %s\n",password.GetString());
  xprintf("DIGEST Buffer  : %s\n",buffer.GetString());
  xprintf("----------------------------------------------------------------------\n");
  xprintf("\n");

  xprintf("Provider       : BCrypt\n");
  xprintf("Password-hash  : RC4\n");
  xprintf("Encryption     : RC4\n");
  xprintf("\n");


  result = crypt.FastEncryption(buffer,password);

  xprintf("Expected       : F8jAdeNxzKzNzPN70RUAtlpuOuyraqlNTDmUImGPMsfdqZgVY7rvIqWjFxtHQRNi6+NprOHcpsuyIQDX\n");
  xprintf("ENCRYPTION     : %s\n",result.GetString());

  result = crypt.FastDecryption(result,password);

  xprintf("DECRYPTION     : %s\n",result.GetString());
  xprintf("\n");

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Test MS Cryptographic provider in fast RC4     : %s\n",(result == buffer) ? "OK" : "ERROR");

  return (result == buffer) ? 0 : 1;
}


int  TestCryptography(void)
{
  int errors = 0;

  errors += TestHashing();
  errors += TestEncryption();
  errors += TestEncryptionLongerStrings();
  errors += TestFastEncryption();

  return errors;
}