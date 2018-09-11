// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "smbprovider/mount_tracker.h"

namespace smbprovider {

MountTracker::MountTracker(std::unique_ptr<base::TickClock> tick_clock)
    : tick_clock_(std::move(tick_clock)) {}

MountTracker::~MountTracker() = default;

bool MountTracker::IsAlreadyMounted(int32_t mount_id) const {
  auto mount_iter = mounts_.Find(mount_id);
  if (mount_iter == mounts_.End()) {
    return false;
  }

  // Check if |mounted_share_paths_| and |mounts_| are in sync.
  DCHECK(ExistsInMounts(mount_iter->second.mount_root));
  return true;
}

bool MountTracker::IsAlreadyMounted(const std::string& mount_root) const {
  bool exists_in_mounted_share_paths =
      mounted_share_paths_.count(mount_root) == 1;

  if (!exists_in_mounted_share_paths) {
    DCHECK(!ExistsInMounts(mount_root));

    return false;
  }

  DCHECK(ExistsInMounts(mount_root));
  return true;
}

bool MountTracker::ExistsInMounts(const std::string& mount_root) const {
  for (auto mount_iter = mounts_.Begin(); mount_iter != mounts_.End();
       ++mount_iter) {
    if (mount_iter->second.mount_root == mount_root) {
      return true;
    }
  }

  return false;
}

bool MountTracker::ExistsInSambaInterfaceMap(const int32_t mount_id) const {
  for (const auto& samba_iter : samba_interface_map_) {
    if (samba_iter.second == mount_id) {
      return true;
    }
  }

  return false;
}

bool MountTracker::AddMount(const std::string& mount_root,
                            SmbCredential credential,
                            std::unique_ptr<SambaInterface> samba_interface,
                            int32_t* mount_id) {
  DCHECK(mount_id);

  if (IsAlreadyMounted(mount_root)) {
    return false;
  }

  *mount_id = mounts_.Insert(CreateMountInfo(mount_root, std::move(credential),
                                             std::move(samba_interface)));

  AddSambaInterfaceIdToSambaInterfaceMap(*mount_id);
  mounted_share_paths_.insert(mount_root);
  return true;
}

bool MountTracker::AddMountWithId(
    const std::string& mount_root,
    SmbCredential credential,
    std::unique_ptr<SambaInterface> samba_interface,
    int32_t mount_id) {
  DCHECK_GE(mount_id, 0);

  if (IsAlreadyMounted(mount_id) || IsAlreadyMounted(mount_root)) {
    return false;
  }

  mounts_.InsertWithSpecificId(
      mount_id, CreateMountInfo(mount_root, std::move(credential),
                                std::move(samba_interface)));

  AddSambaInterfaceIdToSambaInterfaceMap(mount_id);
  mounted_share_paths_.insert(mount_root);
  return true;
}

bool MountTracker::RemoveMount(int32_t mount_id) {
  auto mount_iter = mounts_.Find(mount_id);
  if (mount_iter == mounts_.End()) {
    DCHECK(!ExistsInSambaInterfaceMap(mount_id));
    return false;
  }
  DeleteSambaInterfaceIdFromSambaInterfaceMap(mount_id);

  const bool path_removed =
      mounted_share_paths_.erase(mount_iter->second.mount_root);
  DCHECK(path_removed);

  mounts_.Remove(mount_iter->first);
  return true;
}

MountTracker::MountInfo MountTracker::CreateMountInfo(
    const std::string& mount_root,
    SmbCredential credential,
    std::unique_ptr<SambaInterface> samba_interface) {
  return MountInfo(mount_root, tick_clock_.get(), std::move(credential),
                   std::move(samba_interface));
}

void MountTracker::AddSambaInterfaceIdToSambaInterfaceMap(int32_t mount_id) {
  const SambaInterface::SambaInterfaceId samba_interface_id =
      GetSambaInterfaceIdForMountId(mount_id);
  DCHECK_EQ(0, samba_interface_map_.count(samba_interface_id));

  samba_interface_map_[samba_interface_id] = mount_id;
}

SambaInterface::SambaInterfaceId MountTracker::GetSambaInterfaceIdForMountId(
    int32_t mount_id) const {
  DCHECK(mounts_.Contains(mount_id));

  const MountTracker::MountInfo& mount_info = mounts_.At(mount_id);
  return mount_info.samba_interface->GetSambaInterfaceId();
}

void MountTracker::DeleteSambaInterfaceIdFromSambaInterfaceMap(
    int32_t mount_id) {
  const SambaInterface::SambaInterfaceId samba_interface_id =
      GetSambaInterfaceIdForMountId(mount_id);
  DCHECK_NE(0, samba_interface_map_.count(samba_interface_id));

  samba_interface_map_.erase(samba_interface_id);
}

}  // namespace smbprovider
