// Automatic generation of D-Bus interface mock proxies for:
//  - org.chromium.debugd
#ifndef ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_VAR_CACHE_PORTAGE_CHROMEOS_BASE_DEBUGD_CLIENT_OUT_DEFAULT_GEN_INCLUDE_DEBUGD_DBUS_PROXY_MOCKS_H
#define ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_VAR_CACHE_PORTAGE_CHROMEOS_BASE_DEBUGD_CLIENT_OUT_DEFAULT_GEN_INCLUDE_DEBUGD_DBUS_PROXY_MOCKS_H
#include <string>
#include <vector>

#include <base/callback_forward.h>
#include <base/logging.h>
#include <base/macros.h>
#include <brillo/any.h>
#include <brillo/errors/error.h>
#include <brillo/variant_dictionary.h>
#include <gmock/gmock.h>

#include "debugd/dbus-proxies.h"

namespace org {
namespace chromium {

// Mock object for debugdProxyInterface.
class debugdProxyMock : public debugdProxyInterface {
 public:
  debugdProxyMock() = default;

  MOCK_METHOD6(PingStart,
               bool(const dbus::FileDescriptor& /*in_outfd*/,
                    const std::string& /*in_destination*/,
                    const brillo::VariantDictionary& /*in_options*/,
                    std::string* /*out_handle*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD6(PingStartAsync,
               void(const dbus::FileDescriptor& /*in_outfd*/,
                    const std::string& /*in_destination*/,
                    const brillo::VariantDictionary& /*in_options*/,
                    const base::Callback<void(const std::string& /*handle*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(PingStop,
               bool(const std::string& /*in_handle*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(PingStopAsync,
               void(const std::string& /*in_handle*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(SystraceStart,
               bool(const std::string& /*in_categories*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(SystraceStartAsync,
               void(const std::string& /*in_categories*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(SystraceStop,
               bool(const dbus::FileDescriptor& /*in_outfd*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(SystraceStopAsync,
               void(const dbus::FileDescriptor& /*in_outfd*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(SystraceStatus,
               bool(std::string* /*out_status*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(SystraceStatusAsync,
               void(const base::Callback<void(const std::string& /*status*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD6(TracePathStart,
               bool(const dbus::FileDescriptor& /*in_outfd*/,
                    const std::string& /*in_destination*/,
                    const brillo::VariantDictionary& /*in_options*/,
                    std::string* /*out_handle*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD6(TracePathStartAsync,
               void(const dbus::FileDescriptor& /*in_outfd*/,
                    const std::string& /*in_destination*/,
                    const brillo::VariantDictionary& /*in_options*/,
                    const base::Callback<void(const std::string& /*handle*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(TracePathStop,
               bool(const std::string& /*in_handle*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(TracePathStopAsync,
               void(const std::string& /*in_handle*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(GetRoutes,
               bool(const brillo::VariantDictionary& /*in_options*/,
                    std::vector<std::string>* /*out_result*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(GetRoutesAsync,
               void(const brillo::VariantDictionary& /*in_options*/,
                    const base::Callback<void(const std::vector<std::string>& /*result*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetModemStatus,
               bool(std::string* /*out_status*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetModemStatusAsync,
               void(const base::Callback<void(const std::string& /*status*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(RunModemCommand,
               bool(const std::string& /*in_command*/,
                    std::string* /*out_status*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(RunModemCommandAsync,
               void(const std::string& /*in_command*/,
                    const base::Callback<void(const std::string& /*status*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetNetworkStatus,
               bool(std::string* /*out_status*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetNetworkStatusAsync,
               void(const base::Callback<void(const std::string& /*status*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetWiMaxStatus,
               bool(std::string* /*out_status*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetWiMaxStatusAsync,
               void(const base::Callback<void(const std::string& /*status*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD7(GetPerfOutput,
               bool(uint32_t /*in_duration_sec*/,
                    const std::vector<std::string>& /*in_perf_args*/,
                    int32_t* /*out_status*/,
                    std::vector<uint8_t>* /*out_perf_data*/,
                    std::vector<uint8_t>* /*out_perf_stat*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(GetPerfOutputAsync,
               void(uint32_t /*in_duration_sec*/,
                    const std::vector<std::string>& /*in_perf_args*/,
                    const base::Callback<void(int32_t /*status*/, const std::vector<uint8_t>& /*perf_data*/, const std::vector<uint8_t>& /*perf_stat*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD6(GetRandomPerfOutput,
               bool(uint32_t /*in_duration_sec*/,
                    int32_t* /*out_status*/,
                    std::vector<uint8_t>* /*out_perf_data*/,
                    std::vector<uint8_t>* /*out_perf_stat*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(GetRandomPerfOutputAsync,
               void(uint32_t /*in_duration_sec*/,
                    const base::Callback<void(int32_t /*status*/, const std::vector<uint8_t>& /*perf_data*/, const std::vector<uint8_t>& /*perf_stat*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(GetRichPerfData,
               bool(uint32_t /*in_duration_sec*/,
                    std::vector<uint8_t>* /*out_status*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(GetRichPerfDataAsync,
               void(uint32_t /*in_duration_sec*/,
                    const base::Callback<void(const std::vector<uint8_t>& /*status*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetDebugLogs,
               bool(const dbus::FileDescriptor& /*in_outfd*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(GetDebugLogsAsync,
               void(const dbus::FileDescriptor& /*in_outfd*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(DumpDebugLogs,
               bool(bool /*in_is_compressed*/,
                    const dbus::FileDescriptor& /*in_outfd*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(DumpDebugLogsAsync,
               void(bool /*in_is_compressed*/,
                    const dbus::FileDescriptor& /*in_outfd*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(SetDebugMode,
               bool(const std::string& /*in_subsystem*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(SetDebugModeAsync,
               void(const std::string& /*in_subsystem*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(GetLog,
               bool(const std::string& /*in_log*/,
                    std::string* /*out_contents*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(GetLogAsync,
               void(const std::string& /*in_log*/,
                    const base::Callback<void(const std::string& /*contents*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetAllLogs,
               bool(std::map<std::string, std::string>* /*out_logs*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetAllLogsAsync,
               void(const base::Callback<void(const std::map<std::string, std::string>& /*logs*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetFeedbackLogs,
               bool(std::map<std::string, std::string>* /*out_logs*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetFeedbackLogsAsync,
               void(const base::Callback<void(const std::map<std::string, std::string>& /*logs*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetUserLogFiles,
               bool(std::map<std::string, std::string>* /*out_user_log_files*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetUserLogFilesAsync,
               void(const base::Callback<void(const std::map<std::string, std::string>& /*user_log_files*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetExample,
               bool(std::string* /*out_result*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetExampleAsync,
               void(const base::Callback<void(const std::string& /*result*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetInterfaces,
               bool(std::string* /*out_result*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetInterfacesAsync,
               void(const base::Callback<void(const std::string& /*result*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(TestICMP,
               bool(const std::string& /*in_host*/,
                    std::string* /*out_result*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(TestICMPAsync,
               void(const std::string& /*in_host*/,
                    const base::Callback<void(const std::string& /*result*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(TestICMPWithOptions,
               bool(const std::string& /*in_host*/,
                    const std::map<std::string, std::string>& /*in_options*/,
                    std::string* /*out_result*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(TestICMPWithOptionsAsync,
               void(const std::string& /*in_host*/,
                    const std::map<std::string, std::string>& /*in_options*/,
                    const base::Callback<void(const std::string& /*result*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(BatteryFirmware,
               bool(const std::string& /*in_option*/,
                    std::string* /*out_result*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(BatteryFirmwareAsync,
               void(const std::string& /*in_option*/,
                    const base::Callback<void(const std::string& /*result*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(Smartctl,
               bool(const std::string& /*in_option*/,
                    std::string* /*out_result*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(SmartctlAsync,
               void(const std::string& /*in_option*/,
                    const base::Callback<void(const std::string& /*result*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(MemtesterStart,
               bool(const dbus::FileDescriptor& /*in_outfd*/,
                    uint32_t /*in_memory*/,
                    std::string* /*out_status*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(MemtesterStartAsync,
               void(const dbus::FileDescriptor& /*in_outfd*/,
                    uint32_t /*in_memory*/,
                    const base::Callback<void(const std::string& /*status*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(MemtesterStop,
               bool(const std::string& /*in_handle*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(MemtesterStopAsync,
               void(const std::string& /*in_handle*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(BadblocksStart,
               bool(const dbus::FileDescriptor& /*in_outfd*/,
                    std::string* /*out_status*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(BadblocksStartAsync,
               void(const dbus::FileDescriptor& /*in_outfd*/,
                    const base::Callback<void(const std::string& /*status*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(BadblocksStop,
               bool(const std::string& /*in_handle*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(BadblocksStopAsync,
               void(const std::string& /*in_handle*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD6(PacketCaptureStart,
               bool(const dbus::FileDescriptor& /*in_statfd*/,
                    const dbus::FileDescriptor& /*in_outfd*/,
                    const brillo::VariantDictionary& /*in_options*/,
                    std::string* /*out_handle*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD6(PacketCaptureStartAsync,
               void(const dbus::FileDescriptor& /*in_statfd*/,
                    const dbus::FileDescriptor& /*in_outfd*/,
                    const brillo::VariantDictionary& /*in_options*/,
                    const base::Callback<void(const std::string& /*handle*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(PacketCaptureStop,
               bool(const std::string& /*in_handle*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(PacketCaptureStopAsync,
               void(const std::string& /*in_handle*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(LogKernelTaskStates,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(LogKernelTaskStatesAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(UploadCrashes,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(UploadCrashesAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(RemoveRootfsVerification,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(RemoveRootfsVerificationAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(EnableBootFromUsb,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(EnableBootFromUsbAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(ConfigureSshServer,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(ConfigureSshServerAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(SetUserPassword,
               bool(const std::string& /*in_username*/,
                    const std::string& /*in_password*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(SetUserPasswordAsync,
               void(const std::string& /*in_username*/,
                    const std::string& /*in_password*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(EnableChromeRemoteDebugging,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(EnableChromeRemoteDebuggingAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(EnableChromeDevFeatures,
               bool(const std::string& /*in_root_password*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(EnableChromeDevFeaturesAsync,
               void(const std::string& /*in_root_password*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(QueryDevFeatures,
               bool(int32_t* /*out_features*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(QueryDevFeaturesAsync,
               void(const base::Callback<void(int32_t /*features*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(EnableDevCoredumpUpload,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(EnableDevCoredumpUploadAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(DisableDevCoredumpUpload,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(DisableDevCoredumpUploadAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));

 private:
  DISALLOW_COPY_AND_ASSIGN(debugdProxyMock);
};
}  // namespace chromium
}  // namespace org

#endif  // ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_VAR_CACHE_PORTAGE_CHROMEOS_BASE_DEBUGD_CLIENT_OUT_DEFAULT_GEN_INCLUDE_DEBUGD_DBUS_PROXY_MOCKS_H
