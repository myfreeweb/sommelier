// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UPDATE_ENGINE_UPDATE_MANAGER_SHILL_PROVIDER_H_
#define UPDATE_ENGINE_UPDATE_MANAGER_SHILL_PROVIDER_H_

#include <base/time/time.h>

#include "update_engine/update_manager/provider.h"
#include "update_engine/update_manager/variable.h"

namespace chromeos_update_manager {

enum class ConnectionType {
  kEthernet,
  kWifi,
  kWimax,
  kBluetooth,
  kCellular,
  kUnknown
};

enum class ConnectionTethering {
  kNotDetected,
  kSuspected,
  kConfirmed,
  kUnknown,
};

// Provider for networking related information.
class ShillProvider : public Provider {
 public:
  virtual ~ShillProvider() {}

  // A variable returning whether we currently have network connectivity.
  virtual Variable<bool>* var_is_connected() = 0;

  // A variable returning the current network connection type. Unknown if not
  // connected.
  virtual Variable<ConnectionType>* var_conn_type() = 0;

  // A variable returning the tethering mode of a network connection. Unknown if
  // not connected.
  virtual Variable<ConnectionTethering>* var_conn_tethering() = 0;

  // A variable returning the time when network connection last changed.
  // Initialized to current time.
  virtual Variable<base::Time>* var_conn_last_changed() = 0;

 protected:
  ShillProvider() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ShillProvider);
};

}  // namespace chromeos_update_manager

#endif  // UPDATE_ENGINE_UPDATE_MANAGER_SHILL_PROVIDER_H_
