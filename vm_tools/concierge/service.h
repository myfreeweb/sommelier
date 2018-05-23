// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VM_TOOLS_CONCIERGE_SERVICE_H_
#define VM_TOOLS_CONCIERGE_SERVICE_H_

#include <map>
#include <memory>
#include <string>
#include <utility>

#include <base/callback.h>
#include <base/files/scoped_file.h>
#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>
#include <base/message_loop/message_loop.h>
#include <base/sequence_checker.h>
#include <base/synchronization/lock.h>
#include <base/synchronization/waitable_event.h>
#include <base/threading/thread.h>
#include <dbus/bus.h>
#include <dbus/exported_object.h>
#include <dbus/message.h>
#include <grpc++/grpc++.h>

#include "vm_tools/concierge/container_listener_impl.h"
#include "vm_tools/concierge/mac_address_generator.h"
#include "vm_tools/concierge/startup_listener_impl.h"
#include "vm_tools/concierge/subnet_pool.h"
#include "vm_tools/concierge/virtual_machine.h"
#include "vm_tools/concierge/vsock_cid_pool.h"

namespace vm_tools {
namespace apps {
class ApplicationList;
}  // namespace apps
}  // namespace vm_tools

namespace vm_tools {
namespace concierge {

// VM Launcher Service responsible for responding to DBus method calls for
// starting, stopping, and otherwise managing VMs.
class Service final : public base::MessageLoopForIO::Watcher {
 public:
  // Creates a new Service instance.  |quit_closure| is posted to the TaskRunner
  // for the current thread when this process receives a SIGTERM.
  static std::unique_ptr<Service> Create(base::Closure quit_closure);
  ~Service() override;

  // base::MessageLoopForIO::Watcher overrides.
  void OnFileCanReadWithoutBlocking(int fd) override;
  void OnFileCanWriteWithoutBlocking(int fd) override;

  // Notifies the service that a container with |container_token| and IP of
  // |container_ip| has completed startup. Sets |result| to true if this maps to
  // a subnet inside a currently running VM and |container_token| matches a
  // security token for that VM; false otherwise. Signals |event| when done.
  void ContainerStartupCompleted(const std::string& container_token,
                                 const uint32_t container_ip,
                                 bool* result,
                                 base::WaitableEvent* event);

  // Notifies the service that a container with |container_name| and VSOCK
  // |cid| has failed startup.
  void ContainerStartupFailed(const std::string& container_name,
                              const uint32_t cid);

  // Notifies the service that a container with |container_token| and IP of
  // |container_ip| is shutting down. Sets |result| to true if this maps to
  // a subnet inside a currently running VM and |container_token| matches a
  // security token for that VM; false otherwise. Signals |event| when done.
  void ContainerShutdown(const std::string& container_token,
                         const uint32_t container_ip,
                         bool* result,
                         base::WaitableEvent* event);

  // This will send a D-Bus message to Chrome to inform it of the current
  // installed application list for a container. It will use |container_ip| to
  // resolve the request to a VM and then |container_token| to resolve it to a
  // container. |app_list| should be populated with the list of installed
  // applications but the vm & container names should be left blank; it must
  // remain valid for the lifetime of this call. |result| is set to true on
  // success, false otherwise. Signals |event| when done.
  void UpdateApplicationList(const std::string& container_token,
                             const uint32_t container_ip,
                             vm_tools::apps::ApplicationList* app_list,
                             bool* result,
                             base::WaitableEvent* event);

  // Sends a D-Bus message to Chrome to tell it to open the |url| in a new tab.
  // |container_ip| should be the IP address of the container the request is
  // coming form. |result| is set to true on success, false otherwise. Signals
  // |event| when done.
  void OpenUrl(const std::string& url,
               uint32_t container_ip,
               bool* result,
               base::WaitableEvent* event);

 private:
  explicit Service(base::Closure quit_closure);

  // Initializes the service by connecting to the system DBus daemon, exporting
  // its methods, and taking ownership of it's name.
  bool Init();

  // Handles the termination of a child process.
  void HandleChildExit();

  // Handles a SIGTERM.
  void HandleSigterm();

  // Handles a request to start a VM.  |method_call| must have a StartVmRequest
  // protobuf serialized as an array of bytes.
  std::unique_ptr<dbus::Response> StartVm(dbus::MethodCall* method_call);

  // Handles a request to stop a VM.  |method_call| must have a StopVmRequest
  // protobuf serialized as an array of bytes.
  std::unique_ptr<dbus::Response> StopVm(dbus::MethodCall* method_call);

  // Handles a request to stop all running VMs.
  std::unique_ptr<dbus::Response> StopAllVms(dbus::MethodCall* method_call);

  // Handles a request to get VM info.
  std::unique_ptr<dbus::Response> GetVmInfo(dbus::MethodCall* method_call);

  // Handles a request to create a disk image.
  std::unique_ptr<dbus::Response> CreateDiskImage(
      dbus::MethodCall* method_call);

  // Handles a request to destroy a disk image.
  std::unique_ptr<dbus::Response> DestroyDiskImage(
      dbus::MethodCall* method_call);

  // Handles a request to list existing disk images.
  std::unique_ptr<dbus::Response> ListVmDisks(dbus::MethodCall* method_call);

  // Handles a request to start a container in a VM.
  std::unique_ptr<dbus::Response> StartContainer(dbus::MethodCall* method_call);

  // Handles a request to launch an application in a container.
  std::unique_ptr<dbus::Response> LaunchContainerApplication(
      dbus::MethodCall* method_call);

  // Handles a request to get application icons in a container.
  std::unique_ptr<dbus::Response> GetContainerAppIcon(
      dbus::MethodCall* method_call);

  // Handles a request to get the SSH keys for a container.
  std::unique_ptr<dbus::Response> GetContainerSshKeys(
      dbus::MethodCall* method_call);

  // Handles a request to launch vshd in a container.
  std::unique_ptr<dbus::Response> LaunchVshd(dbus::MethodCall* method_call);

  // Helper for starting termina VMs, e.g. starting lxd.
  bool StartTermina(VirtualMachine* vm, std::string* failure_reason);

  // Registers |hostname| and |ip| with the hostname resolver service so that
  // the container is reachable from a known hostname.
  void RegisterHostname(const std::string& hostname, const std::string& ip);

  // Unregisters all the hostnames that were registered for this |vm| with
  // |vm_name| with the hostname resolver service.
  void UnregisterVmHostnames(VirtualMachine* vm,
                             const std::string& owner_id,
                             const std::string& vm_name);

  // Unregisters |hostname| with the hostname resolver service.
  void UnregisterHostname(const std::string& hostname);

  // Callback for when the crosdns D-Bus service goes online (or is online
  // already) so we can then register the NameOnwerChanged callback.
  void OnCrosDnsServiceAvailable(bool service_is_available);

  // Callback for when the crosdns D-Bus service restarts so we can
  // re-register any of our hostnames that are active.
  void OnCrosDnsNameOwnerChanged(const std::string& old_owner,
                                 const std::string& new_owner);

  using VmMap = std::map<std::pair<std::string, std::string>,
                         std::unique_ptr<VirtualMachine>>;

  // Returns an iterator to vm with key (|owner_id|, |vm_name|). If no such
  // element exists, tries the former with |owner_id| equal to empty string.
  VmMap::iterator FindVm(std::string owner_id, std::string vm_name);

  // Gets the VirtualMachine that corresponds to a container at |container_ip|
  // and returns an iterator. Returns |vms_.end()| if no sutch mapping exists.
  // VM. Returns false if no such mapping exists.
  VmMap::const_iterator GetVirtualMachineForContainerIp(uint32_t container_ip);

  // Resource allocators for VMs.
  MacAddressGenerator mac_address_generator_;
  SubnetPool subnet_pool_;
  VsockCidPool vsock_cid_pool_;

  // File descriptor for the SIGCHLD events.
  base::ScopedFD signal_fd_;
  base::MessageLoopForIO::FileDescriptorWatcher watcher_;

  // Active VMs keyed by (owner_id, vm_name).
  VmMap vms_;

  // Owner of the first started vm with name kDefaultVmName
  std::string principal_owner_id_;

  // Connection to the system bus.
  scoped_refptr<dbus::Bus> bus_;
  dbus::ExportedObject* exported_object_;             // Owned by |bus_|.
  dbus::ObjectProxy* vm_applications_service_proxy_;  // Owned by |bus_|.
  dbus::ObjectProxy* url_handler_service_proxy_;      // Owned by |bus_|.
  dbus::ObjectProxy* crosdns_service_proxy_;          // Owned by |bus_|.

  // The StartupListener service.
  std::unique_ptr<StartupListenerImpl> startup_listener_;

  // The ContainerListener service.
  std::unique_ptr<ContainerListenerImpl> container_listener_;

  // Thread on which the StartupListener service lives.
  base::Thread grpc_thread_vm_{"gRPC VM Server Thread"};

  // Thread on which the ContainerListener service lives.
  base::Thread grpc_thread_container_{"gRPC Container Server Thread"};

  // The server where the StartupListener service lives.
  std::shared_ptr<grpc::Server> grpc_server_vm_;

  // The server where the ContainerListener service lives.
  std::shared_ptr<grpc::Server> grpc_server_container_;

  // Closure that's posted to the current thread's TaskRunner when the service
  // receives a SIGTERM.
  base::Closure quit_closure_;

  // Ensure calls are made on the right thread.
  base::SequenceChecker sequence_checker_;

  // Map of hostnames/IPs we have registered so we can re-register them if the
  // resolver service restarts.
  std::map<std::string, std::string> hostname_mappings_;

  base::WeakPtrFactory<Service> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(Service);
};

}  // namespace concierge
}  // namespace vm_tools

#endif  // VM_TOOLS_CONCIERGE_SERVICE_H_
