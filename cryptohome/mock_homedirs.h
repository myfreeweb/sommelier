// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOCK_HOMEDIRS_H_
#define MOCK_HOMEDIRS_H_

#include "homedirs.h"

#include <gmock/gmock.h>

#include "credentials.h"

namespace cryptohome {

class MockHomeDirs : public HomeDirs {
 public:
  MockHomeDirs() {}
  virtual ~MockHomeDirs() {}

  MOCK_METHOD0(Init, bool());
  MOCK_METHOD0(FreeDiskSpace, bool());
  MOCK_METHOD1(AreCredentialsValid, bool(const Credentials&));
  MOCK_METHOD1(Remove, bool(const std::string&));
};

}  // namespace cryptohome

#endif  /* !MOCK_HOMEDIRS_H_ */
