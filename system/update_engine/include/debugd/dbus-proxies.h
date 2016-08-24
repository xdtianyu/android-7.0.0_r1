// Automatic generation of D-Bus interfaces:
//  - org.chromium.debugd
#ifndef ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_VAR_CACHE_PORTAGE_CHROMEOS_BASE_DEBUGD_CLIENT_OUT_DEFAULT_GEN_INCLUDE_DEBUGD_DBUS_PROXIES_H
#define ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_VAR_CACHE_PORTAGE_CHROMEOS_BASE_DEBUGD_CLIENT_OUT_DEFAULT_GEN_INCLUDE_DEBUGD_DBUS_PROXIES_H
#include <memory>
#include <string>
#include <vector>

#include <base/bind.h>
#include <base/callback.h>
#include <base/logging.h>
#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <brillo/any.h>
#include <brillo/dbus/dbus_method_invoker.h>
#include <brillo/dbus/dbus_property.h>
#include <brillo/dbus/dbus_signal_handler.h>
#include <brillo/errors/error.h>
#include <brillo/variant_dictionary.h>
#include <dbus/bus.h>
#include <dbus/message.h>
#include <dbus/object_manager.h>
#include <dbus/object_path.h>
#include <dbus/object_proxy.h>

namespace org {
namespace chromium {

// Abstract interface proxy for org::chromium::debugd.
class debugdProxyInterface {
 public:
  virtual ~debugdProxyInterface() = default;

  // Starts pinging the specified hostname with the specified options, with
  // output directed to the given output file descriptor. The returned opaque
  // string functions as a handle for this particular ping. Multiple pings
  // can be running at once.
  virtual bool PingStart(
      const dbus::FileDescriptor& in_outfd,
      const std::string& in_destination,
      const brillo::VariantDictionary& in_options,
      std::string* out_handle,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Starts pinging the specified hostname with the specified options, with
  // output directed to the given output file descriptor. The returned opaque
  // string functions as a handle for this particular ping. Multiple pings
  // can be running at once.
  virtual void PingStartAsync(
      const dbus::FileDescriptor& in_outfd,
      const std::string& in_destination,
      const brillo::VariantDictionary& in_options,
      const base::Callback<void(const std::string& /*handle*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Stops a running ping.
  virtual bool PingStop(
      const std::string& in_handle,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Stops a running ping.
  virtual void PingStopAsync(
      const std::string& in_handle,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Start system/kernel tracing.  If tracing is already enabled it is
  // stopped first and any collected events are discarded.  The kernel
  // must have been configured to support tracing.
  virtual bool SystraceStart(
      const std::string& in_categories,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Start system/kernel tracing.  If tracing is already enabled it is
  // stopped first and any collected events are discarded.  The kernel
  // must have been configured to support tracing.
  virtual void SystraceStartAsync(
      const std::string& in_categories,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Stop system/kernel tracing and write the collected event data.
  virtual bool SystraceStop(
      const dbus::FileDescriptor& in_outfd,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Stop system/kernel tracing and write the collected event data.
  virtual void SystraceStopAsync(
      const dbus::FileDescriptor& in_outfd,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Return current status for system/kernel tracing including whether it
  // is enabled, the tracing clock, and the set of events enabled.
  virtual bool SystraceStatus(
      std::string* out_status,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Return current status for system/kernel tracing including whether it
  // is enabled, the tracing clock, and the set of events enabled.
  virtual void SystraceStatusAsync(
      const base::Callback<void(const std::string& /*status*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool TracePathStart(
      const dbus::FileDescriptor& in_outfd,
      const std::string& in_destination,
      const brillo::VariantDictionary& in_options,
      std::string* out_handle,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void TracePathStartAsync(
      const dbus::FileDescriptor& in_outfd,
      const std::string& in_destination,
      const brillo::VariantDictionary& in_options,
      const base::Callback<void(const std::string& /*handle*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Stops a running tracepath.
  virtual bool TracePathStop(
      const std::string& in_handle,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Stops a running tracepath.
  virtual void TracePathStopAsync(
      const std::string& in_handle,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns the routing table.
  virtual bool GetRoutes(
      const brillo::VariantDictionary& in_options,
      std::vector<std::string>* out_result,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns the routing table.
  virtual void GetRoutesAsync(
      const brillo::VariantDictionary& in_options,
      const base::Callback<void(const std::vector<std::string>& /*result*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns modem information as a JSON string. See the design document for
  // a rationale.
  virtual bool GetModemStatus(
      std::string* out_status,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns modem information as a JSON string. See the design document for
  // a rationale.
  virtual void GetModemStatusAsync(
      const base::Callback<void(const std::string& /*status*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Runs the specified command through the modem serial interface and
  // returns the output.
  virtual bool RunModemCommand(
      const std::string& in_command,
      std::string* out_status,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Runs the specified command through the modem serial interface and
  // returns the output.
  virtual void RunModemCommandAsync(
      const std::string& in_command,
      const base::Callback<void(const std::string& /*status*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns network information as a JSON string. See the design document
  // for a rationale.
  virtual bool GetNetworkStatus(
      std::string* out_status,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns network information as a JSON string. See the design document
  // for a rationale.
  virtual void GetNetworkStatusAsync(
      const base::Callback<void(const std::string& /*status*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns WiMAX information as a JSON string. See the design document for
  // a rationale.
  virtual bool GetWiMaxStatus(
      std::string* out_status,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns WiMAX information as a JSON string. See the design document for
  // a rationale.
  virtual void GetWiMaxStatusAsync(
      const base::Callback<void(const std::string& /*status*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Runs system-wide perf profiling. The profile parameters are selected by
  // perf_args.
  virtual bool GetPerfOutput(
      uint32_t in_duration_sec,
      const std::vector<std::string>& in_perf_args,
      int32_t* out_status,
      std::vector<uint8_t>* out_perf_data,
      std::vector<uint8_t>* out_perf_stat,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Runs system-wide perf profiling. The profile parameters are selected by
  // perf_args.
  virtual void GetPerfOutputAsync(
      uint32_t in_duration_sec,
      const std::vector<std::string>& in_perf_args,
      const base::Callback<void(int32_t /*status*/, const std::vector<uint8_t>& /*perf_data*/, const std::vector<uint8_t>& /*perf_stat*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Runs system-wide perf profiling. It can can profile events other than
  // cycles (example: iTLB-misses), and can collect branch profiles. It can
  // also return raw counter values. The exact profile or counters to be
  // collected is chosen at random and depends on what CPU is used by the
  // system (certain CPUs do not support certain profiling modes).
  virtual bool GetRandomPerfOutput(
      uint32_t in_duration_sec,
      int32_t* out_status,
      std::vector<uint8_t>* out_perf_data,
      std::vector<uint8_t>* out_perf_stat,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Runs system-wide perf profiling. It can can profile events other than
  // cycles (example: iTLB-misses), and can collect branch profiles. It can
  // also return raw counter values. The exact profile or counters to be
  // collected is chosen at random and depends on what CPU is used by the
  // system (certain CPUs do not support certain profiling modes).
  virtual void GetRandomPerfOutputAsync(
      uint32_t in_duration_sec,
      const base::Callback<void(int32_t /*status*/, const std::vector<uint8_t>& /*perf_data*/, const std::vector<uint8_t>& /*perf_stat*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns perf event data. Does systemwide profiling. It can profile
  // events other than cycles (example: iTLB-misses), and can collect branch
  // profiles. The exact profile to be collected is chosen at random
  // and depends on what CPU is used by the system (certain CPUs do not
  // support certain profiling modes).
  virtual bool GetRichPerfData(
      uint32_t in_duration_sec,
      std::vector<uint8_t>* out_status,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns perf event data. Does systemwide profiling. It can profile
  // events other than cycles (example: iTLB-misses), and can collect branch
  // profiles. The exact profile to be collected is chosen at random
  // and depends on what CPU is used by the system (certain CPUs do not
  // support certain profiling modes).
  virtual void GetRichPerfDataAsync(
      uint32_t in_duration_sec,
      const base::Callback<void(const std::vector<uint8_t>& /*status*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // DEPRECATED: Use DumpDebugLogs instead.
  // Packages up system logs into a .tar.gz and returns it over the supplied
  // file descriptor.
  virtual bool GetDebugLogs(
      const dbus::FileDescriptor& in_outfd,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // DEPRECATED: Use DumpDebugLogs instead.
  // Packages up system logs into a .tar.gz and returns it over the supplied
  // file descriptor.
  virtual void GetDebugLogsAsync(
      const dbus::FileDescriptor& in_outfd,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Packages up system logs into a .tar(.gz) and returns it over the
  // supplied file descriptor.
  virtual bool DumpDebugLogs(
      bool in_is_compressed,
      const dbus::FileDescriptor& in_outfd,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Packages up system logs into a .tar(.gz) and returns it over the
  // supplied file descriptor.
  virtual void DumpDebugLogsAsync(
      bool in_is_compressed,
      const dbus::FileDescriptor& in_outfd,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Enables or disables debug mode for a specified subsystem.
  virtual bool SetDebugMode(
      const std::string& in_subsystem,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Enables or disables debug mode for a specified subsystem.
  virtual void SetDebugModeAsync(
      const std::string& in_subsystem,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Fetches the contents of a single system log, identified by name. See
  // /src/log_tool.cc for a list of valid names.
  virtual bool GetLog(
      const std::string& in_log,
      std::string* out_contents,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Fetches the contents of a single system log, identified by name. See
  // /src/log_tool.cc for a list of valid names.
  virtual void GetLogAsync(
      const std::string& in_log,
      const base::Callback<void(const std::string& /*contents*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns all the system logs.
  virtual bool GetAllLogs(
      std::map<std::string, std::string>* out_logs,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns all the system logs.
  virtual void GetAllLogsAsync(
      const base::Callback<void(const std::map<std::string, std::string>& /*logs*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns system logs for feedback reports.
  virtual bool GetFeedbackLogs(
      std::map<std::string, std::string>* out_logs,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns system logs for feedback reports.
  virtual void GetFeedbackLogsAsync(
      const base::Callback<void(const std::map<std::string, std::string>& /*logs*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns list of User log file names that Chrome itself must collect.
  // These logfiles are relative to the user's profile path and must be
  // collected separately for each user.
  virtual bool GetUserLogFiles(
      std::map<std::string, std::string>* out_user_log_files,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns list of User log file names that Chrome itself must collect.
  // These logfiles are relative to the user's profile path and must be
  // collected separately for each user.
  virtual void GetUserLogFilesAsync(
      const base::Callback<void(const std::map<std::string, std::string>& /*user_log_files*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Example method. See /doc/hacking.md.
  virtual bool GetExample(
      std::string* out_result,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Example method. See /doc/hacking.md.
  virtual void GetExampleAsync(
      const base::Callback<void(const std::string& /*result*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns information about network interfaces as a JSON string.
  virtual bool GetInterfaces(
      std::string* out_result,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Returns information about network interfaces as a JSON string.
  virtual void GetInterfacesAsync(
      const base::Callback<void(const std::string& /*result*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Tests ICMP connectivity to a specified host.
  virtual bool TestICMP(
      const std::string& in_host,
      std::string* out_result,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Tests ICMP connectivity to a specified host.
  virtual void TestICMPAsync(
      const std::string& in_host,
      const base::Callback<void(const std::string& /*result*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Tests ICMP connectivity to a specified host (with options).
  virtual bool TestICMPWithOptions(
      const std::string& in_host,
      const std::map<std::string, std::string>& in_options,
      std::string* out_result,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Tests ICMP connectivity to a specified host (with options).
  virtual void TestICMPWithOptionsAsync(
      const std::string& in_host,
      const std::map<std::string, std::string>& in_options,
      const base::Callback<void(const std::string& /*result*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Runs BatteryFirmware utility.
  virtual bool BatteryFirmware(
      const std::string& in_option,
      std::string* out_result,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Runs BatteryFirmware utility.
  virtual void BatteryFirmwareAsync(
      const std::string& in_option,
      const base::Callback<void(const std::string& /*result*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Runs Smartctl utility.
  virtual bool Smartctl(
      const std::string& in_option,
      std::string* out_result,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Runs Smartctl utility.
  virtual void SmartctlAsync(
      const std::string& in_option,
      const base::Callback<void(const std::string& /*result*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Starts running memtester.
  virtual bool MemtesterStart(
      const dbus::FileDescriptor& in_outfd,
      uint32_t in_memory,
      std::string* out_status,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Starts running memtester.
  virtual void MemtesterStartAsync(
      const dbus::FileDescriptor& in_outfd,
      uint32_t in_memory,
      const base::Callback<void(const std::string& /*status*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Stops running memtester.
  virtual bool MemtesterStop(
      const std::string& in_handle,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Stops running memtester.
  virtual void MemtesterStopAsync(
      const std::string& in_handle,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Starts running badblocks test.
  virtual bool BadblocksStart(
      const dbus::FileDescriptor& in_outfd,
      std::string* out_status,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Starts running badblocks test.
  virtual void BadblocksStartAsync(
      const dbus::FileDescriptor& in_outfd,
      const base::Callback<void(const std::string& /*status*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Stops running badblocks.
  virtual bool BadblocksStop(
      const std::string& in_handle,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Stops running badblocks.
  virtual void BadblocksStopAsync(
      const std::string& in_handle,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Starts a packet capture with the specified options, with diagnostic
  // status directed to the "statfd" file descriptor and packet capture
  // data sent to the "outfd" file descriptor.  The returned opaque string
  // functions as a handle for this particular packet capture.  Multiple
  // captures can be running at once.  Captures can be initiated on
  // Ethernet-like devices or WiFi devices in "client mode" (showing only
  // Ethernet frames) by specifying the "device" parameter (see below).
  // By specifying a channel, the script will find or create a "monitor
  // mode" interface if one is available and produce an "over the air"
  // packet capture.  The name of the output packet capture file is sent
  // to the output file descriptor.
  virtual bool PacketCaptureStart(
      const dbus::FileDescriptor& in_statfd,
      const dbus::FileDescriptor& in_outfd,
      const brillo::VariantDictionary& in_options,
      std::string* out_handle,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Starts a packet capture with the specified options, with diagnostic
  // status directed to the "statfd" file descriptor and packet capture
  // data sent to the "outfd" file descriptor.  The returned opaque string
  // functions as a handle for this particular packet capture.  Multiple
  // captures can be running at once.  Captures can be initiated on
  // Ethernet-like devices or WiFi devices in "client mode" (showing only
  // Ethernet frames) by specifying the "device" parameter (see below).
  // By specifying a channel, the script will find or create a "monitor
  // mode" interface if one is available and produce an "over the air"
  // packet capture.  The name of the output packet capture file is sent
  // to the output file descriptor.
  virtual void PacketCaptureStartAsync(
      const dbus::FileDescriptor& in_statfd,
      const dbus::FileDescriptor& in_outfd,
      const brillo::VariantDictionary& in_options,
      const base::Callback<void(const std::string& /*handle*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Stops a running packet capture.
  virtual bool PacketCaptureStop(
      const std::string& in_handle,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Stops a running packet capture.
  virtual void PacketCaptureStopAsync(
      const std::string& in_handle,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Triggers show-task-states(T) SysRq.
  // See https://www.kernel.org/doc/Documentation/sysrq.txt.
  virtual bool LogKernelTaskStates(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Triggers show-task-states(T) SysRq.
  // See https://www.kernel.org/doc/Documentation/sysrq.txt.
  virtual void LogKernelTaskStatesAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Triggers uploading of system crashes (the crash_sender program).
  virtual bool UploadCrashes(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Triggers uploading of system crashes (the crash_sender program).
  virtual void UploadCrashesAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Removes rootfs verification. Requires a system reboot before it will
  // take effect. Restricted to pre-owner dev mode.
  virtual bool RemoveRootfsVerification(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Removes rootfs verification. Requires a system reboot before it will
  // take effect. Restricted to pre-owner dev mode.
  virtual void RemoveRootfsVerificationAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Enables OS booting from a USB image. Restricted to pre-owner dev mode.
  virtual bool EnableBootFromUsb(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Enables OS booting from a USB image. Restricted to pre-owner dev mode.
  virtual void EnableBootFromUsbAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Sets up sshd to provide an SSH server immediately and on future reboots.
  // Also installs the test SSH keys to allow access by cros tools. Requires
  // that rootfs verification has been removed. Restricted to pre-owner dev
  // mode.
  virtual bool ConfigureSshServer(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Sets up sshd to provide an SSH server immediately and on future reboots.
  // Also installs the test SSH keys to allow access by cros tools. Requires
  // that rootfs verification has been removed. Restricted to pre-owner dev
  // mode.
  virtual void ConfigureSshServerAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Sets both the system and dev mode password for the indicated account.
  // Restricted to pre-owner dev mode.
  virtual bool SetUserPassword(
      const std::string& in_username,
      const std::string& in_password,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Sets both the system and dev mode password for the indicated account.
  // Restricted to pre-owner dev mode.
  virtual void SetUserPasswordAsync(
      const std::string& in_username,
      const std::string& in_password,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Sets up Chrome for remote debugging. It will take effect after a reboot
  // and using port 9222.
  // Requires that rootfs verification has been removed. Restricted to
  // pre-owner dev mode.
  virtual bool EnableChromeRemoteDebugging(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Sets up Chrome for remote debugging. It will take effect after a reboot
  // and using port 9222.
  // Requires that rootfs verification has been removed. Restricted to
  // pre-owner dev mode.
  virtual void EnableChromeRemoteDebuggingAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Convenience function to enable a predefined set of tools from the Chrome
  // UI. Equivalent to calling these functions in order:
  //   1. EnableBootFromUsb()
  //   2. ConfigureSshServer()
  //   3. SetUserPassword("root", root_password)
  // Requires that rootfs verification has been removed. If any sub-function
  // fails, this function will exit with an error without attempting any
  // further configuration or rollback. Restricted to pre-owner dev mode.
  virtual bool EnableChromeDevFeatures(
      const std::string& in_root_password,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Convenience function to enable a predefined set of tools from the Chrome
  // UI. Equivalent to calling these functions in order:
  //   1. EnableBootFromUsb()
  //   2. ConfigureSshServer()
  //   3. SetUserPassword("root", root_password)
  // Requires that rootfs verification has been removed. If any sub-function
  // fails, this function will exit with an error without attempting any
  // further configuration or rollback. Restricted to pre-owner dev mode.
  virtual void EnableChromeDevFeaturesAsync(
      const std::string& in_root_password,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Queries which dev features have been enabled. Each dev feature will be
  // indicated by a bit flag in the return value. Flags are defined in the
  // DevFeatureFlag enumeration. If the dev tools are unavailable (system is
  // not in dev mode/pre-login state), the DEV_FEATURES_DISABLED flag will be
  // set and the rest of the bits will always be set to 0.
  virtual bool QueryDevFeatures(
      int32_t* out_features,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Queries which dev features have been enabled. Each dev feature will be
  // indicated by a bit flag in the return value. Flags are defined in the
  // DevFeatureFlag enumeration. If the dev tools are unavailable (system is
  // not in dev mode/pre-login state), the DEV_FEATURES_DISABLED flag will be
  // set and the rest of the bits will always be set to 0.
  virtual void QueryDevFeaturesAsync(
      const base::Callback<void(int32_t /*features*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Allow uploading of device coredump files.
  virtual bool EnableDevCoredumpUpload(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Allow uploading of device coredump files.
  virtual void EnableDevCoredumpUploadAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Disallow uploading of device coredump files.
  virtual bool DisableDevCoredumpUpload(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Disallow uploading of device coredump files.
  virtual void DisableDevCoredumpUploadAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Interface proxy for org::chromium::debugd.
class debugdProxy final : public debugdProxyInterface {
 public:
  debugdProxy(const scoped_refptr<dbus::Bus>& bus) :
      bus_{bus},
      dbus_object_proxy_{
          bus_->GetObjectProxy(service_name_, object_path_)} {
  }

  ~debugdProxy() override {
    bus_->RemoveObjectProxy(
        service_name_, object_path_, base::Bind(&base::DoNothing));
  }

  void ReleaseObjectProxy(const base::Closure& callback) {
    bus_->RemoveObjectProxy(service_name_, object_path_, callback);
  }

  const dbus::ObjectPath& GetObjectPath() const {
    return object_path_;
  }

  dbus::ObjectProxy* GetObjectProxy() const { return dbus_object_proxy_; }

  // Starts pinging the specified hostname with the specified options, with
  // output directed to the given output file descriptor. The returned opaque
  // string functions as a handle for this particular ping. Multiple pings
  // can be running at once.
  bool PingStart(
      const dbus::FileDescriptor& in_outfd,
      const std::string& in_destination,
      const brillo::VariantDictionary& in_options,
      std::string* out_handle,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "PingStart",
        error,
        in_outfd,
        in_destination,
        in_options);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_handle);
  }

  // Starts pinging the specified hostname with the specified options, with
  // output directed to the given output file descriptor. The returned opaque
  // string functions as a handle for this particular ping. Multiple pings
  // can be running at once.
  void PingStartAsync(
      const dbus::FileDescriptor& in_outfd,
      const std::string& in_destination,
      const brillo::VariantDictionary& in_options,
      const base::Callback<void(const std::string& /*handle*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "PingStart",
        success_callback,
        error_callback,
        in_outfd,
        in_destination,
        in_options);
  }

  // Stops a running ping.
  bool PingStop(
      const std::string& in_handle,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "PingStop",
        error,
        in_handle);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Stops a running ping.
  void PingStopAsync(
      const std::string& in_handle,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "PingStop",
        success_callback,
        error_callback,
        in_handle);
  }

  // Start system/kernel tracing.  If tracing is already enabled it is
  // stopped first and any collected events are discarded.  The kernel
  // must have been configured to support tracing.
  bool SystraceStart(
      const std::string& in_categories,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "SystraceStart",
        error,
        in_categories);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Start system/kernel tracing.  If tracing is already enabled it is
  // stopped first and any collected events are discarded.  The kernel
  // must have been configured to support tracing.
  void SystraceStartAsync(
      const std::string& in_categories,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "SystraceStart",
        success_callback,
        error_callback,
        in_categories);
  }

  // Stop system/kernel tracing and write the collected event data.
  bool SystraceStop(
      const dbus::FileDescriptor& in_outfd,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "SystraceStop",
        error,
        in_outfd);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Stop system/kernel tracing and write the collected event data.
  void SystraceStopAsync(
      const dbus::FileDescriptor& in_outfd,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "SystraceStop",
        success_callback,
        error_callback,
        in_outfd);
  }

  // Return current status for system/kernel tracing including whether it
  // is enabled, the tracing clock, and the set of events enabled.
  bool SystraceStatus(
      std::string* out_status,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "SystraceStatus",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_status);
  }

  // Return current status for system/kernel tracing including whether it
  // is enabled, the tracing clock, and the set of events enabled.
  void SystraceStatusAsync(
      const base::Callback<void(const std::string& /*status*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "SystraceStatus",
        success_callback,
        error_callback);
  }

  bool TracePathStart(
      const dbus::FileDescriptor& in_outfd,
      const std::string& in_destination,
      const brillo::VariantDictionary& in_options,
      std::string* out_handle,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "TracePathStart",
        error,
        in_outfd,
        in_destination,
        in_options);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_handle);
  }

  void TracePathStartAsync(
      const dbus::FileDescriptor& in_outfd,
      const std::string& in_destination,
      const brillo::VariantDictionary& in_options,
      const base::Callback<void(const std::string& /*handle*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "TracePathStart",
        success_callback,
        error_callback,
        in_outfd,
        in_destination,
        in_options);
  }

  // Stops a running tracepath.
  bool TracePathStop(
      const std::string& in_handle,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "TracePathStop",
        error,
        in_handle);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Stops a running tracepath.
  void TracePathStopAsync(
      const std::string& in_handle,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "TracePathStop",
        success_callback,
        error_callback,
        in_handle);
  }

  // Returns the routing table.
  bool GetRoutes(
      const brillo::VariantDictionary& in_options,
      std::vector<std::string>* out_result,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetRoutes",
        error,
        in_options);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_result);
  }

  // Returns the routing table.
  void GetRoutesAsync(
      const brillo::VariantDictionary& in_options,
      const base::Callback<void(const std::vector<std::string>& /*result*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetRoutes",
        success_callback,
        error_callback,
        in_options);
  }

  // Returns modem information as a JSON string. See the design document for
  // a rationale.
  bool GetModemStatus(
      std::string* out_status,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetModemStatus",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_status);
  }

  // Returns modem information as a JSON string. See the design document for
  // a rationale.
  void GetModemStatusAsync(
      const base::Callback<void(const std::string& /*status*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetModemStatus",
        success_callback,
        error_callback);
  }

  // Runs the specified command through the modem serial interface and
  // returns the output.
  bool RunModemCommand(
      const std::string& in_command,
      std::string* out_status,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "RunModemCommand",
        error,
        in_command);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_status);
  }

  // Runs the specified command through the modem serial interface and
  // returns the output.
  void RunModemCommandAsync(
      const std::string& in_command,
      const base::Callback<void(const std::string& /*status*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "RunModemCommand",
        success_callback,
        error_callback,
        in_command);
  }

  // Returns network information as a JSON string. See the design document
  // for a rationale.
  bool GetNetworkStatus(
      std::string* out_status,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetNetworkStatus",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_status);
  }

  // Returns network information as a JSON string. See the design document
  // for a rationale.
  void GetNetworkStatusAsync(
      const base::Callback<void(const std::string& /*status*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetNetworkStatus",
        success_callback,
        error_callback);
  }

  // Returns WiMAX information as a JSON string. See the design document for
  // a rationale.
  bool GetWiMaxStatus(
      std::string* out_status,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetWiMaxStatus",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_status);
  }

  // Returns WiMAX information as a JSON string. See the design document for
  // a rationale.
  void GetWiMaxStatusAsync(
      const base::Callback<void(const std::string& /*status*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetWiMaxStatus",
        success_callback,
        error_callback);
  }

  // Runs system-wide perf profiling. The profile parameters are selected by
  // perf_args.
  bool GetPerfOutput(
      uint32_t in_duration_sec,
      const std::vector<std::string>& in_perf_args,
      int32_t* out_status,
      std::vector<uint8_t>* out_perf_data,
      std::vector<uint8_t>* out_perf_stat,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetPerfOutput",
        error,
        in_duration_sec,
        in_perf_args);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_status, out_perf_data, out_perf_stat);
  }

  // Runs system-wide perf profiling. The profile parameters are selected by
  // perf_args.
  void GetPerfOutputAsync(
      uint32_t in_duration_sec,
      const std::vector<std::string>& in_perf_args,
      const base::Callback<void(int32_t /*status*/, const std::vector<uint8_t>& /*perf_data*/, const std::vector<uint8_t>& /*perf_stat*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetPerfOutput",
        success_callback,
        error_callback,
        in_duration_sec,
        in_perf_args);
  }

  // Runs system-wide perf profiling. It can can profile events other than
  // cycles (example: iTLB-misses), and can collect branch profiles. It can
  // also return raw counter values. The exact profile or counters to be
  // collected is chosen at random and depends on what CPU is used by the
  // system (certain CPUs do not support certain profiling modes).
  bool GetRandomPerfOutput(
      uint32_t in_duration_sec,
      int32_t* out_status,
      std::vector<uint8_t>* out_perf_data,
      std::vector<uint8_t>* out_perf_stat,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetRandomPerfOutput",
        error,
        in_duration_sec);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_status, out_perf_data, out_perf_stat);
  }

  // Runs system-wide perf profiling. It can can profile events other than
  // cycles (example: iTLB-misses), and can collect branch profiles. It can
  // also return raw counter values. The exact profile or counters to be
  // collected is chosen at random and depends on what CPU is used by the
  // system (certain CPUs do not support certain profiling modes).
  void GetRandomPerfOutputAsync(
      uint32_t in_duration_sec,
      const base::Callback<void(int32_t /*status*/, const std::vector<uint8_t>& /*perf_data*/, const std::vector<uint8_t>& /*perf_stat*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetRandomPerfOutput",
        success_callback,
        error_callback,
        in_duration_sec);
  }

  // Returns perf event data. Does systemwide profiling. It can profile
  // events other than cycles (example: iTLB-misses), and can collect branch
  // profiles. The exact profile to be collected is chosen at random
  // and depends on what CPU is used by the system (certain CPUs do not
  // support certain profiling modes).
  bool GetRichPerfData(
      uint32_t in_duration_sec,
      std::vector<uint8_t>* out_status,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetRichPerfData",
        error,
        in_duration_sec);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_status);
  }

  // Returns perf event data. Does systemwide profiling. It can profile
  // events other than cycles (example: iTLB-misses), and can collect branch
  // profiles. The exact profile to be collected is chosen at random
  // and depends on what CPU is used by the system (certain CPUs do not
  // support certain profiling modes).
  void GetRichPerfDataAsync(
      uint32_t in_duration_sec,
      const base::Callback<void(const std::vector<uint8_t>& /*status*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetRichPerfData",
        success_callback,
        error_callback,
        in_duration_sec);
  }

  // DEPRECATED: Use DumpDebugLogs instead.
  // Packages up system logs into a .tar.gz and returns it over the supplied
  // file descriptor.
  bool GetDebugLogs(
      const dbus::FileDescriptor& in_outfd,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetDebugLogs",
        error,
        in_outfd);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // DEPRECATED: Use DumpDebugLogs instead.
  // Packages up system logs into a .tar.gz and returns it over the supplied
  // file descriptor.
  void GetDebugLogsAsync(
      const dbus::FileDescriptor& in_outfd,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetDebugLogs",
        success_callback,
        error_callback,
        in_outfd);
  }

  // Packages up system logs into a .tar(.gz) and returns it over the
  // supplied file descriptor.
  bool DumpDebugLogs(
      bool in_is_compressed,
      const dbus::FileDescriptor& in_outfd,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "DumpDebugLogs",
        error,
        in_is_compressed,
        in_outfd);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Packages up system logs into a .tar(.gz) and returns it over the
  // supplied file descriptor.
  void DumpDebugLogsAsync(
      bool in_is_compressed,
      const dbus::FileDescriptor& in_outfd,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "DumpDebugLogs",
        success_callback,
        error_callback,
        in_is_compressed,
        in_outfd);
  }

  // Enables or disables debug mode for a specified subsystem.
  bool SetDebugMode(
      const std::string& in_subsystem,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "SetDebugMode",
        error,
        in_subsystem);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Enables or disables debug mode for a specified subsystem.
  void SetDebugModeAsync(
      const std::string& in_subsystem,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "SetDebugMode",
        success_callback,
        error_callback,
        in_subsystem);
  }

  // Fetches the contents of a single system log, identified by name. See
  // /src/log_tool.cc for a list of valid names.
  bool GetLog(
      const std::string& in_log,
      std::string* out_contents,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetLog",
        error,
        in_log);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_contents);
  }

  // Fetches the contents of a single system log, identified by name. See
  // /src/log_tool.cc for a list of valid names.
  void GetLogAsync(
      const std::string& in_log,
      const base::Callback<void(const std::string& /*contents*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetLog",
        success_callback,
        error_callback,
        in_log);
  }

  // Returns all the system logs.
  bool GetAllLogs(
      std::map<std::string, std::string>* out_logs,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetAllLogs",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_logs);
  }

  // Returns all the system logs.
  void GetAllLogsAsync(
      const base::Callback<void(const std::map<std::string, std::string>& /*logs*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetAllLogs",
        success_callback,
        error_callback);
  }

  // Returns system logs for feedback reports.
  bool GetFeedbackLogs(
      std::map<std::string, std::string>* out_logs,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetFeedbackLogs",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_logs);
  }

  // Returns system logs for feedback reports.
  void GetFeedbackLogsAsync(
      const base::Callback<void(const std::map<std::string, std::string>& /*logs*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetFeedbackLogs",
        success_callback,
        error_callback);
  }

  // Returns list of User log file names that Chrome itself must collect.
  // These logfiles are relative to the user's profile path and must be
  // collected separately for each user.
  bool GetUserLogFiles(
      std::map<std::string, std::string>* out_user_log_files,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetUserLogFiles",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_user_log_files);
  }

  // Returns list of User log file names that Chrome itself must collect.
  // These logfiles are relative to the user's profile path and must be
  // collected separately for each user.
  void GetUserLogFilesAsync(
      const base::Callback<void(const std::map<std::string, std::string>& /*user_log_files*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetUserLogFiles",
        success_callback,
        error_callback);
  }

  // Example method. See /doc/hacking.md.
  bool GetExample(
      std::string* out_result,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetExample",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_result);
  }

  // Example method. See /doc/hacking.md.
  void GetExampleAsync(
      const base::Callback<void(const std::string& /*result*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetExample",
        success_callback,
        error_callback);
  }

  // Returns information about network interfaces as a JSON string.
  bool GetInterfaces(
      std::string* out_result,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetInterfaces",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_result);
  }

  // Returns information about network interfaces as a JSON string.
  void GetInterfacesAsync(
      const base::Callback<void(const std::string& /*result*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "GetInterfaces",
        success_callback,
        error_callback);
  }

  // Tests ICMP connectivity to a specified host.
  bool TestICMP(
      const std::string& in_host,
      std::string* out_result,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "TestICMP",
        error,
        in_host);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_result);
  }

  // Tests ICMP connectivity to a specified host.
  void TestICMPAsync(
      const std::string& in_host,
      const base::Callback<void(const std::string& /*result*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "TestICMP",
        success_callback,
        error_callback,
        in_host);
  }

  // Tests ICMP connectivity to a specified host (with options).
  bool TestICMPWithOptions(
      const std::string& in_host,
      const std::map<std::string, std::string>& in_options,
      std::string* out_result,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "TestICMPWithOptions",
        error,
        in_host,
        in_options);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_result);
  }

  // Tests ICMP connectivity to a specified host (with options).
  void TestICMPWithOptionsAsync(
      const std::string& in_host,
      const std::map<std::string, std::string>& in_options,
      const base::Callback<void(const std::string& /*result*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "TestICMPWithOptions",
        success_callback,
        error_callback,
        in_host,
        in_options);
  }

  // Runs BatteryFirmware utility.
  bool BatteryFirmware(
      const std::string& in_option,
      std::string* out_result,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "BatteryFirmware",
        error,
        in_option);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_result);
  }

  // Runs BatteryFirmware utility.
  void BatteryFirmwareAsync(
      const std::string& in_option,
      const base::Callback<void(const std::string& /*result*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "BatteryFirmware",
        success_callback,
        error_callback,
        in_option);
  }

  // Runs Smartctl utility.
  bool Smartctl(
      const std::string& in_option,
      std::string* out_result,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "Smartctl",
        error,
        in_option);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_result);
  }

  // Runs Smartctl utility.
  void SmartctlAsync(
      const std::string& in_option,
      const base::Callback<void(const std::string& /*result*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "Smartctl",
        success_callback,
        error_callback,
        in_option);
  }

  // Starts running memtester.
  bool MemtesterStart(
      const dbus::FileDescriptor& in_outfd,
      uint32_t in_memory,
      std::string* out_status,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "MemtesterStart",
        error,
        in_outfd,
        in_memory);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_status);
  }

  // Starts running memtester.
  void MemtesterStartAsync(
      const dbus::FileDescriptor& in_outfd,
      uint32_t in_memory,
      const base::Callback<void(const std::string& /*status*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "MemtesterStart",
        success_callback,
        error_callback,
        in_outfd,
        in_memory);
  }

  // Stops running memtester.
  bool MemtesterStop(
      const std::string& in_handle,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "MemtesterStop",
        error,
        in_handle);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Stops running memtester.
  void MemtesterStopAsync(
      const std::string& in_handle,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "MemtesterStop",
        success_callback,
        error_callback,
        in_handle);
  }

  // Starts running badblocks test.
  bool BadblocksStart(
      const dbus::FileDescriptor& in_outfd,
      std::string* out_status,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "BadblocksStart",
        error,
        in_outfd);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_status);
  }

  // Starts running badblocks test.
  void BadblocksStartAsync(
      const dbus::FileDescriptor& in_outfd,
      const base::Callback<void(const std::string& /*status*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "BadblocksStart",
        success_callback,
        error_callback,
        in_outfd);
  }

  // Stops running badblocks.
  bool BadblocksStop(
      const std::string& in_handle,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "BadblocksStop",
        error,
        in_handle);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Stops running badblocks.
  void BadblocksStopAsync(
      const std::string& in_handle,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "BadblocksStop",
        success_callback,
        error_callback,
        in_handle);
  }

  // Starts a packet capture with the specified options, with diagnostic
  // status directed to the "statfd" file descriptor and packet capture
  // data sent to the "outfd" file descriptor.  The returned opaque string
  // functions as a handle for this particular packet capture.  Multiple
  // captures can be running at once.  Captures can be initiated on
  // Ethernet-like devices or WiFi devices in "client mode" (showing only
  // Ethernet frames) by specifying the "device" parameter (see below).
  // By specifying a channel, the script will find or create a "monitor
  // mode" interface if one is available and produce an "over the air"
  // packet capture.  The name of the output packet capture file is sent
  // to the output file descriptor.
  bool PacketCaptureStart(
      const dbus::FileDescriptor& in_statfd,
      const dbus::FileDescriptor& in_outfd,
      const brillo::VariantDictionary& in_options,
      std::string* out_handle,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "PacketCaptureStart",
        error,
        in_statfd,
        in_outfd,
        in_options);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_handle);
  }

  // Starts a packet capture with the specified options, with diagnostic
  // status directed to the "statfd" file descriptor and packet capture
  // data sent to the "outfd" file descriptor.  The returned opaque string
  // functions as a handle for this particular packet capture.  Multiple
  // captures can be running at once.  Captures can be initiated on
  // Ethernet-like devices or WiFi devices in "client mode" (showing only
  // Ethernet frames) by specifying the "device" parameter (see below).
  // By specifying a channel, the script will find or create a "monitor
  // mode" interface if one is available and produce an "over the air"
  // packet capture.  The name of the output packet capture file is sent
  // to the output file descriptor.
  void PacketCaptureStartAsync(
      const dbus::FileDescriptor& in_statfd,
      const dbus::FileDescriptor& in_outfd,
      const brillo::VariantDictionary& in_options,
      const base::Callback<void(const std::string& /*handle*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "PacketCaptureStart",
        success_callback,
        error_callback,
        in_statfd,
        in_outfd,
        in_options);
  }

  // Stops a running packet capture.
  bool PacketCaptureStop(
      const std::string& in_handle,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "PacketCaptureStop",
        error,
        in_handle);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Stops a running packet capture.
  void PacketCaptureStopAsync(
      const std::string& in_handle,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "PacketCaptureStop",
        success_callback,
        error_callback,
        in_handle);
  }

  // Triggers show-task-states(T) SysRq.
  // See https://www.kernel.org/doc/Documentation/sysrq.txt.
  bool LogKernelTaskStates(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "LogKernelTaskStates",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Triggers show-task-states(T) SysRq.
  // See https://www.kernel.org/doc/Documentation/sysrq.txt.
  void LogKernelTaskStatesAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "LogKernelTaskStates",
        success_callback,
        error_callback);
  }

  // Triggers uploading of system crashes (the crash_sender program).
  bool UploadCrashes(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "UploadCrashes",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Triggers uploading of system crashes (the crash_sender program).
  void UploadCrashesAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "UploadCrashes",
        success_callback,
        error_callback);
  }

  // Removes rootfs verification. Requires a system reboot before it will
  // take effect. Restricted to pre-owner dev mode.
  bool RemoveRootfsVerification(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "RemoveRootfsVerification",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Removes rootfs verification. Requires a system reboot before it will
  // take effect. Restricted to pre-owner dev mode.
  void RemoveRootfsVerificationAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "RemoveRootfsVerification",
        success_callback,
        error_callback);
  }

  // Enables OS booting from a USB image. Restricted to pre-owner dev mode.
  bool EnableBootFromUsb(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "EnableBootFromUsb",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Enables OS booting from a USB image. Restricted to pre-owner dev mode.
  void EnableBootFromUsbAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "EnableBootFromUsb",
        success_callback,
        error_callback);
  }

  // Sets up sshd to provide an SSH server immediately and on future reboots.
  // Also installs the test SSH keys to allow access by cros tools. Requires
  // that rootfs verification has been removed. Restricted to pre-owner dev
  // mode.
  bool ConfigureSshServer(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "ConfigureSshServer",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Sets up sshd to provide an SSH server immediately and on future reboots.
  // Also installs the test SSH keys to allow access by cros tools. Requires
  // that rootfs verification has been removed. Restricted to pre-owner dev
  // mode.
  void ConfigureSshServerAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "ConfigureSshServer",
        success_callback,
        error_callback);
  }

  // Sets both the system and dev mode password for the indicated account.
  // Restricted to pre-owner dev mode.
  bool SetUserPassword(
      const std::string& in_username,
      const std::string& in_password,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "SetUserPassword",
        error,
        in_username,
        in_password);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Sets both the system and dev mode password for the indicated account.
  // Restricted to pre-owner dev mode.
  void SetUserPasswordAsync(
      const std::string& in_username,
      const std::string& in_password,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "SetUserPassword",
        success_callback,
        error_callback,
        in_username,
        in_password);
  }

  // Sets up Chrome for remote debugging. It will take effect after a reboot
  // and using port 9222.
  // Requires that rootfs verification has been removed. Restricted to
  // pre-owner dev mode.
  bool EnableChromeRemoteDebugging(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "EnableChromeRemoteDebugging",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Sets up Chrome for remote debugging. It will take effect after a reboot
  // and using port 9222.
  // Requires that rootfs verification has been removed. Restricted to
  // pre-owner dev mode.
  void EnableChromeRemoteDebuggingAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "EnableChromeRemoteDebugging",
        success_callback,
        error_callback);
  }

  // Convenience function to enable a predefined set of tools from the Chrome
  // UI. Equivalent to calling these functions in order:
  //   1. EnableBootFromUsb()
  //   2. ConfigureSshServer()
  //   3. SetUserPassword("root", root_password)
  // Requires that rootfs verification has been removed. If any sub-function
  // fails, this function will exit with an error without attempting any
  // further configuration or rollback. Restricted to pre-owner dev mode.
  bool EnableChromeDevFeatures(
      const std::string& in_root_password,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "EnableChromeDevFeatures",
        error,
        in_root_password);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Convenience function to enable a predefined set of tools from the Chrome
  // UI. Equivalent to calling these functions in order:
  //   1. EnableBootFromUsb()
  //   2. ConfigureSshServer()
  //   3. SetUserPassword("root", root_password)
  // Requires that rootfs verification has been removed. If any sub-function
  // fails, this function will exit with an error without attempting any
  // further configuration or rollback. Restricted to pre-owner dev mode.
  void EnableChromeDevFeaturesAsync(
      const std::string& in_root_password,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "EnableChromeDevFeatures",
        success_callback,
        error_callback,
        in_root_password);
  }

  // Queries which dev features have been enabled. Each dev feature will be
  // indicated by a bit flag in the return value. Flags are defined in the
  // DevFeatureFlag enumeration. If the dev tools are unavailable (system is
  // not in dev mode/pre-login state), the DEV_FEATURES_DISABLED flag will be
  // set and the rest of the bits will always be set to 0.
  bool QueryDevFeatures(
      int32_t* out_features,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "QueryDevFeatures",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_features);
  }

  // Queries which dev features have been enabled. Each dev feature will be
  // indicated by a bit flag in the return value. Flags are defined in the
  // DevFeatureFlag enumeration. If the dev tools are unavailable (system is
  // not in dev mode/pre-login state), the DEV_FEATURES_DISABLED flag will be
  // set and the rest of the bits will always be set to 0.
  void QueryDevFeaturesAsync(
      const base::Callback<void(int32_t /*features*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "QueryDevFeatures",
        success_callback,
        error_callback);
  }

  // Allow uploading of device coredump files.
  bool EnableDevCoredumpUpload(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "EnableDevCoredumpUpload",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Allow uploading of device coredump files.
  void EnableDevCoredumpUploadAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "EnableDevCoredumpUpload",
        success_callback,
        error_callback);
  }

  // Disallow uploading of device coredump files.
  bool DisableDevCoredumpUpload(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "DisableDevCoredumpUpload",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Disallow uploading of device coredump files.
  void DisableDevCoredumpUploadAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.debugd",
        "DisableDevCoredumpUpload",
        success_callback,
        error_callback);
  }

 private:
  scoped_refptr<dbus::Bus> bus_;
  const std::string service_name_{"org.chromium.debugd"};
  const dbus::ObjectPath object_path_{"/org/chromium/debugd"};
  dbus::ObjectProxy* dbus_object_proxy_;

  DISALLOW_COPY_AND_ASSIGN(debugdProxy);
};

}  // namespace chromium
}  // namespace org

#endif  // ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_VAR_CACHE_PORTAGE_CHROMEOS_BASE_DEBUGD_CLIENT_OUT_DEFAULT_GEN_INCLUDE_DEBUGD_DBUS_PROXIES_H
