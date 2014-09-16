// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include <gtest/gtest.h>

#include "trunks/hmac_auth_delegate.h"

namespace trunks {

TEST(HmacAuthDelegateTest, UninitializedSessionTest) {
  HmacAuthDelegate delegate(false);
  std::string dummy;
  std::string p_hash("test");
  EXPECT_EQ(delegate.GetCommandAuthorization(p_hash, &dummy), false);
  EXPECT_EQ(dummy.size(), 0);
  EXPECT_EQ(delegate.CheckResponseAuthorization(p_hash, dummy), false);
  EXPECT_EQ(delegate.EncryptCommandParameter(&dummy), false);
  EXPECT_EQ(delegate.DecryptResponseParameter(&dummy), false);
}

TEST(HmacAuthDelegateTest, SessionKeyTest) {
  HmacAuthDelegate delegate(false);
  TPM2B_NONCE nonce;
  nonce.size = kAesKeySize;
  memset(nonce.buffer, 0, nonce.size);
  TPM_HANDLE dummy_handle = HMAC_SESSION_FIRST;
  EXPECT_EQ(delegate.InitSession(dummy_handle, nonce, nonce, std::string(),
      std::string()), true);
  EXPECT_EQ(delegate.session_key_.size(), 0);

  std::string dummy_auth = std::string("authorization");
  std::string dummy_salt = std::string("salt");
  EXPECT_EQ(delegate.InitSession(dummy_handle, nonce, nonce, dummy_salt,
      dummy_auth), true);
  EXPECT_EQ(delegate.session_key_.size(), kHashDigestSize);
  // TODO(usanghi): Use TCG TPM2.0 test vectors when available.
  std::string expected_key("\x63\xdb\x03\xe1\x6c\x5a\x72\x89"
                           "\x0f\xfb\xb2\x3f\x04\xe3\x78\xba"
                           "\x58\x23\xa4\xe8\xa7\xbb\xf5\xfc"
                           "\x0d\xac\xef\x3b\x7b\x98\xf1\x91",
                           kHashDigestSize);
  EXPECT_EQ(expected_key.compare(delegate.session_key_), 0);
}

TEST(HmacAuthDelegateTest, EncryptDecryptTest) {
  HmacAuthDelegate delegate(false);
  std::string plaintext_parameter("parameter");
  std::string encrypted_parameter(plaintext_parameter);
  // Check should fail as delegate isnt initialized
  EXPECT_EQ(delegate.EncryptCommandParameter(&encrypted_parameter), false);

  // Check if AES encrypt and decrypt operations are symmetric with the same
  // nonce.
  HmacAuthDelegate encrypt_delegate(true);
  TPM_HANDLE dummy_handle = HMAC_SESSION_FIRST;
  TPM2B_NONCE nonce;
  nonce.size = kAesKeySize;
  EXPECT_EQ(encrypt_delegate.InitSession(dummy_handle, nonce, nonce,
      std::string(), std::string()), true);
  EXPECT_EQ(encrypt_delegate.EncryptCommandParameter(&encrypted_parameter),
            true);
  EXPECT_NE(plaintext_parameter.compare(encrypted_parameter), 0);
  EXPECT_EQ(encrypt_delegate.DecryptResponseParameter(&encrypted_parameter),
            true);
  EXPECT_EQ(plaintext_parameter.compare(encrypted_parameter), 0);

  // Now we check that encryption with no symmetric algorithm succeeds.
  ASSERT_EQ(plaintext_parameter.compare(encrypted_parameter), 0);
  delegate.EncryptCommandParameter(&encrypted_parameter);
  delegate.DecryptResponseParameter(&encrypted_parameter);
  EXPECT_EQ(plaintext_parameter.compare(encrypted_parameter), 0);
}

TEST(HmacAuthDelegateTest, CommandAuthTest) {
  HmacAuthDelegate delegate(false);
  TPM_HANDLE dummy_handle = HMAC_SESSION_FIRST;
  TPM2B_NONCE nonce;
  nonce.size = kAesKeySize;
  EXPECT_EQ(delegate.InitSession(dummy_handle, nonce, nonce, std::string(),
      std::string()), true);
  std::string command_hash;
  std::string authorization;
  EXPECT_EQ(delegate.GetCommandAuthorization(command_hash, &authorization),
            true);
  TPMS_AUTH_COMMAND auth_command;
  std::string auth_bytes;
  EXPECT_EQ(Parse_TPMS_AUTH_COMMAND(&authorization, &auth_command,
      &auth_bytes), TPM_RC_SUCCESS);
  EXPECT_EQ(auth_command.session_handle, dummy_handle);
  EXPECT_EQ(auth_command.nonce.size, nonce.size);
  EXPECT_EQ(auth_command.session_attributes, kContinueSession);
  EXPECT_EQ(auth_command.hmac.size, kHashDigestSize);
}

TEST(HmacAuthDelegateTest, ResponseAuthTest) {
  HmacAuthDelegate delegate(false);
  TPM_HANDLE dummy_handle = HMAC_SESSION_FIRST;
  TPM2B_NONCE nonce;
  nonce.size = kAesKeySize;
  memset(nonce.buffer, 0, kAesKeySize);
  EXPECT_EQ(delegate.InitSession(dummy_handle, nonce, nonce, std::string(),
      std::string()), true);
  TPMS_AUTH_RESPONSE auth_response;
  auth_response.session_attributes = kContinueSession;
  auth_response.nonce.size = kAesKeySize;
  memset(auth_response.nonce.buffer, 0, kAesKeySize);
  auth_response.hmac.size = kHashDigestSize;
  // TODO(usanghi): Use TCG TPM2.0 test vectors when available.
  uint8_t hmac_buffer[kHashDigestSize] =
      {0x37, 0x69, 0xaf, 0x12, 0xff, 0x4d, 0xbf, 0x44,
       0xe5, 0x16, 0xa2, 0x2d, 0x1d, 0x05, 0x12, 0xe8,
       0xbc, 0x42, 0x51, 0x6d, 0x59, 0xe8, 0xbf, 0x40,
       0x1e, 0xa3, 0x46, 0xa4, 0xd6, 0x0d, 0xcc, 0xf7};
  memcpy(auth_response.hmac.buffer, hmac_buffer, kHashDigestSize);
  std::string response_hash;
  std::string authorization;
  EXPECT_EQ(Serialize_TPMS_AUTH_RESPONSE(auth_response, &authorization),
            TPM_RC_SUCCESS);
  EXPECT_EQ(delegate.CheckResponseAuthorization(response_hash, authorization),
            true);
}

}  // namespace trunks
