// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cros-disks/device-event-moderator.h"

#include <base/logging.h>

#include "cros-disks/device-event-dispatcher-interface.h"
#include "cros-disks/device-event-source-interface.h"

using std::string;

namespace cros_disks {

DeviceEventModerator::DeviceEventModerator(
    DeviceEventDispatcherInterface* event_dispatcher,
    DeviceEventSourceInterface* event_source)
    : event_dispatcher_(event_dispatcher),
      event_source_(event_source),
      is_event_queued_(true) {
  CHECK(event_dispatcher_) << "Invalid event dispatcher object";
  CHECK(event_source_) << "Invalid event source object";
}

DeviceEventModerator::~DeviceEventModerator() {
}

void DeviceEventModerator::DispatchQueuedDeviceEvents() {
  const DeviceEvent* event;
  while ((event = event_queue_.Head()) != NULL) {
    LOG(INFO) << "Dispatch queued event: type=" << event->event_type
              << " device='" << event->device_path << "'";
    event_dispatcher_->DispatchDeviceEvent(*event);
    event_queue_.Remove();
  }
}

void DeviceEventModerator::OnScreenIsLocked() {
  LOG(INFO) << "Screen is locked. Device events are now queued.";
  is_event_queued_ = true;
}

void DeviceEventModerator::OnScreenIsUnlocked() {
  LOG(INFO) << "Screen is locked. Queued device events are now dispatched.";
  DispatchQueuedDeviceEvents();
  is_event_queued_ = false;
}

void DeviceEventModerator::OnSessionStarted() {
  LOG(INFO) << "Session started. Queued device events are now dispatched.";
  DispatchQueuedDeviceEvents();
  is_event_queued_ = false;
}

void DeviceEventModerator::OnSessionStopped() {
  LOG(INFO) << "Session stopped. Device events are now queued.";
  is_event_queued_ = true;
}

void DeviceEventModerator::ProcessDeviceEvents() {
  DeviceEventList events;
  if (event_source_->GetDeviceEvents(&events)) {
    if (is_event_queued_) {
      for (DeviceEventList::const_iterator event_iter = events.begin();
           event_iter != events.end(); ++event_iter) {
        event_queue_.Add(*event_iter);
      }
    } else {
      for (DeviceEventList::const_iterator event_iter = events.begin();
           event_iter != events.end(); ++event_iter) {
        event_dispatcher_->DispatchDeviceEvent(*event_iter);
      }
    }
  }
}

}  // namespace cros_disks
