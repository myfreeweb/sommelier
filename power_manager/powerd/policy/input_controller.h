// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef POWER_MANAGER_POWERD_POLICY_INPUT_CONTROLLER_H_
#define POWER_MANAGER_POWERD_POLICY_INPUT_CONTROLLER_H_
#pragma once

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/file_path.h"
#include "base/time.h"
#include "power_manager/common/power_constants.h"
#include "power_manager/common/signal_callback.h"
#include "power_manager/powerd/system/input_observer.h"

typedef int gboolean;
typedef unsigned int guint;

namespace power_manager {

class BacklightController;
class DBusSenderInterface;
class PrefsInterface;

namespace system {
class Input;
}  // namespace system

namespace policy {

class StateController;

// InputController responds to input events (e.g. lid open/close, power button,
// etc.).
class InputController : public system::InputObserver {
 public:
  // Interface for delegates responsible for performing actions on behalf of
  // InputController.
  class Delegate {
   public:
    virtual ~Delegate() {}

    // Handles the lid being closed.
    virtual void HandleLidClosed() = 0;

    // Handles the lid being opened.
    virtual void HandleLidOpened() = 0;

    // Sends metrics in response to the power button being pressed or released.
    virtual void SendPowerButtonMetric(bool down,
                                       base::TimeTicks timestamp) = 0;
  };

  InputController(system::Input* input,
                  Delegate* delegate,
                  BacklightController* backlight_controller,
                  StateController* state_controller,
                  DBusSenderInterface* dbus_sender,
                  const base::FilePath& run_dir);
  ~InputController();

  void Init(PrefsInterface* prefs);

  // system::InputObserver implementation:
  virtual void OnLidEvent(LidState state) OVERRIDE;
  virtual void OnPowerButtonEvent(ButtonState state) OVERRIDE;

 private:
  // Reports user activity to |state_controller_| if the second virtual
  // terminal is currently active (which typically means that the user is
  // doing something on the console in dev mode, so Chrome won't be
  // reporting user activity to keep power management from kicking in).
  SIGNAL_CALLBACK_0(InputController, gboolean, CheckActiveVT);

  system::Input* input_;  // not owned
  Delegate* delegate_;  // not owned
  BacklightController* backlight_controller_;  // not owned
  StateController* state_controller_;  // not owned
  DBusSenderInterface* dbus_sender_;  // not owned

  LidState lid_state_;

  // Should |input_| be queried for the state of the lid?
  bool use_input_for_lid_;

  // GLib source ID for a timeout that calls CheckActiveVT() periodically.
  guint check_active_vt_timeout_id_;

  DISALLOW_COPY_AND_ASSIGN(InputController);
};

}  // namespace policy
}  // namespace power_manager

#endif  // POWER_MANAGER_POWERD_POLICY_INPUT_CONTROLLER_H_
