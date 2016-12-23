// Copyright 2016 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AUTHPOLICY_SAMBA_INTERFACE_INTERNAL_H_
#define AUTHPOLICY_SAMBA_INTERFACE_INTERNAL_H_

#include <string>
#include <vector>

#include <base/files/file_path.h>
#include <dbus/authpolicy/dbus-constants.h>

namespace authpolicy {
namespace internal {

// Parses user_name@workgroup.some.domain into its components and normalizes
// (uppercases) the part behind the @. |out_realm| is WORKGROUP.SOME.DOMAIN,
// |out_workgroup| is WORKGROUP, |out_normalized_user_principal_name| is
// user_name@WORKGROUP.SOME.DOMAIN.
bool ParseUserPrincipalName(const std::string& user_principal_name,
                            std::string* out_user_name,
                            std::string* out_realm,
                            std::string* out_workgroup,
                            std::string* out_normalized_user_principal_name,
                            ErrorType* out_error);

// Parses the given |in_str| consisting of individual lines for
//   ... \n
//   |token| <token_separator> |out_result| \n
//   ... \n
// and returns the first non-empty |out_result|. Whitespace is trimmed.
bool FindToken(const std::string& in_str, char token_separator,
               const std::string& token, std::string* out_result);

// Parses a GPO version string, which consists of a number and the same number
// as base-16 hex number, e.g. '31 (0x0000001f)'.
bool ParseGpoVersion(const std::string& str, unsigned int* out_num);

}  // namespace internal
}  // namespace authpolicy

#endif  // AUTHPOLICY_SAMBA_INTERFACE_INTERNAL_H_
