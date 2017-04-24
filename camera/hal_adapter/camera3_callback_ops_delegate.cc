/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "hal_adapter/camera3_callback_ops_delegate.h"

#include <utility>

#include <base/bind.h>
#include <base/bind_helpers.h>

#include "arc/common.h"
#include "arc/future.h"
#include "hal_adapter/camera_device_adapter.h"

namespace arc {

Camera3CallbackOpsDelegate::Camera3CallbackOpsDelegate(
    CameraDeviceAdapter* camera_device_adapter,
    mojo::InterfacePtrInfo<mojom::Camera3CallbackOps> callback_ops_ptr_info,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : internal::MojoChannel<mojom::Camera3CallbackOps>(
          std::move(callback_ops_ptr_info),
          task_runner),
      camera_device_adapter_(camera_device_adapter) {
  camera3_callback_ops_t::process_capture_result = ProcessCaptureResult;
  camera3_callback_ops_t::notify = Notify;
}

void Camera3CallbackOpsDelegate::ProcessCaptureResult(
    const camera3_callback_ops_t* ops,
    const camera3_capture_result_t* result) {
  VLOGF_ENTER();
  Camera3CallbackOpsDelegate* delegate =
      const_cast<Camera3CallbackOpsDelegate*>(
          static_cast<const Camera3CallbackOpsDelegate*>(ops));

  mojom::Camera3CaptureResultPtr result_ptr =
      delegate->camera_device_adapter_->ProcessCaptureResult(result);
  delegate->task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&Camera3CallbackOpsDelegate::ProcessCaptureResultOnThread,
                 base::AsWeakPtr(delegate), base::Passed(&result_ptr)));
}

void Camera3CallbackOpsDelegate::Notify(const camera3_callback_ops_t* ops,
                                        const camera3_notify_msg_t* msg) {
  VLOGF_ENTER();
  Camera3CallbackOpsDelegate* delegate =
      const_cast<Camera3CallbackOpsDelegate*>(
          static_cast<const Camera3CallbackOpsDelegate*>(ops));

  mojom::Camera3NotifyMsgPtr msg_ptr =
      delegate->camera_device_adapter_->Notify(msg);
  delegate->task_runner_->PostTask(
      FROM_HERE, base::Bind(&Camera3CallbackOpsDelegate::NotifyOnThread,
                            base::AsWeakPtr(delegate), base::Passed(&msg_ptr)));
}

void Camera3CallbackOpsDelegate::ProcessCaptureResultOnThread(
    mojom::Camera3CaptureResultPtr result) {
  VLOGF_ENTER();
  DCHECK(task_runner_->BelongsToCurrentThread());
  interface_ptr_->ProcessCaptureResult(std::move(result));
}

void Camera3CallbackOpsDelegate::NotifyOnThread(
    mojom::Camera3NotifyMsgPtr msg) {
  VLOGF_ENTER();
  DCHECK(task_runner_->BelongsToCurrentThread());
  interface_ptr_->Notify(std::move(msg));
}

}  // end of namespace arc
