// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LOGIN_MANAGER_SYSTEM_UTILS_IMPL_H_
#define LOGIN_MANAGER_SYSTEM_UTILS_IMPL_H_

#include "login_manager/system_utils.h"

#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include <string>
#include <vector>

#include <base/files/scoped_temp_dir.h>
#include <base/strings/stringprintf.h>
#include <base/time/time.h>
#include <chromeos/dbus/service_constants.h>

namespace base {
class FilePath;
}

namespace login_manager {

class SystemUtilsImpl : public SystemUtils {
 public:
  SystemUtilsImpl();
  ~SystemUtilsImpl() override;

  int kill(pid_t pid, uid_t owner, int signal) override;
  time_t time(time_t* t) override;
  pid_t fork() override;
  bool GetAppOutput(const std::vector<std::string>& argv,
                    std::string* output) override;
  DevModeState GetDevModeState() override;
  VmState GetVmState() override;
  bool ProcessGroupIsGone(pid_t child_spec, base::TimeDelta timeout) override;
  bool ProcessIsGone(pid_t child_spec, base::TimeDelta timeout) override;

  bool EnsureAndReturnSafeFileSize(const base::FilePath& file,
                                   int32_t* file_size_32) override;
  bool Exists(const base::FilePath& file) override;
  bool DirectoryExists(const base::FilePath& dir) override;
  bool IsDirectoryEmpty(const base::FilePath& dir) override;
  bool CreateReadOnlyFileInTempDir(base::FilePath* temp_file) override;
  bool CreateTemporaryDirIn(const base::FilePath& parent_dir,
                            base::FilePath* out_dir) override;
  bool CreateDir(const base::FilePath& dir) override;
  bool GetUniqueFilenameInWriteOnlyTempDir(
      base::FilePath* temp_file_path) override;
  bool RemoveDirTree(const base::FilePath& dir) override;
  bool RemoveFile(const base::FilePath& filename) override;
  bool RenameDir(const base::FilePath& source,
                 const base::FilePath& target) override;
  bool AtomicFileWrite(const base::FilePath& filename,
                       const std::string& data) override;
  int64_t AmountOfFreeDiskSpace(const base::FilePath& path) override;
  bool GetGroupInfo(const std::string& group_name, gid_t* out_gid) override;
  bool ChangeOwner(const base::FilePath& filename,
                   pid_t pid,
                   gid_t gid) override;
  bool SetPosixFilePermissions(const base::FilePath& filename,
                               mode_t mode) override;
  ScopedPlatformHandle CreateServerHandle(
      const NamedPlatformHandle& named_handle) override;

  void set_base_dir_for_testing(const base::FilePath& base_dir) {
    CHECK(!base_dir.empty());
    CHECK(base_dir_for_testing_.empty());
    base_dir_for_testing_ = base_dir;
  }

  // Returns the given path "chrooted" inside |base_dir_for_testing_| if set.
  // Ex: /run/foo -> /tmp/.org.Chromium.whatever/run/foo
  base::FilePath PutInsideBaseDirForTesting(const base::FilePath& path);

 private:
  // Provides the real implementation of PutInsideBaseDirForTesting.
  base::FilePath PutInsideBaseDir(const base::FilePath& path);

  // If this file exists on the next boot, the stateful partition will be wiped.
  static const char kResetFile[];

  DevModeState dev_mode_state_;
  VmState vm_state_;
  base::ScopedTempDir temp_dir_;
  base::FilePath base_dir_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(SystemUtilsImpl);
};

}  // namespace login_manager

#endif  // LOGIN_MANAGER_SYSTEM_UTILS_IMPL_H_
