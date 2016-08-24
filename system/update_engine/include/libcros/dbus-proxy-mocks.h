// Automatic generation of D-Bus interface mock proxies for:
//  - org.chromium.LibCrosServiceInterface
//  - org.chromium.UpdateEngineLibcrosProxyResolvedInterface
#ifndef ____CHROMEOS_DBUS_BINDING___UPDATE_ENGINE_INCLUDE_LIBCROS_DBUS_PROXY_MOCKS_H
#define ____CHROMEOS_DBUS_BINDING___UPDATE_ENGINE_INCLUDE_LIBCROS_DBUS_PROXY_MOCKS_H
#include <string>
#include <vector>

#include <base/callback_forward.h>
#include <base/logging.h>
#include <base/macros.h>
#include <brillo/any.h>
#include <brillo/errors/error.h>
#include <brillo/variant_dictionary.h>
#include <gmock/gmock.h>

#include "libcros/dbus-proxies.h"

namespace org {
namespace chromium {

// Mock object for LibCrosServiceInterfaceProxyInterface.
class LibCrosServiceInterfaceProxyMock : public LibCrosServiceInterfaceProxyInterface {
 public:
  LibCrosServiceInterfaceProxyMock() = default;

  MOCK_METHOD5(ResolveNetworkProxy,
               bool(const std::string& /*in_source_url*/,
                    const std::string& /*in_signal_interface*/,
                    const std::string& /*in_signal_name*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD6(ResolveNetworkProxyAsync,
               void(const std::string& /*in_source_url*/,
                    const std::string& /*in_signal_interface*/,
                    const std::string& /*in_signal_name*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_CONST_METHOD0(GetObjectPath, const dbus::ObjectPath&());

 private:
  DISALLOW_COPY_AND_ASSIGN(LibCrosServiceInterfaceProxyMock);
};
}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Mock object for UpdateEngineLibcrosProxyResolvedInterfaceProxyInterface.
class UpdateEngineLibcrosProxyResolvedInterfaceProxyMock : public UpdateEngineLibcrosProxyResolvedInterfaceProxyInterface {
 public:
  UpdateEngineLibcrosProxyResolvedInterfaceProxyMock() = default;

  MOCK_METHOD2(RegisterProxyResolvedSignalHandler,
               void(const base::Callback<void(const std::string&,
                                              const std::string&,
                                              const std::string&)>& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_CONST_METHOD0(GetObjectPath, const dbus::ObjectPath&());

 private:
  DISALLOW_COPY_AND_ASSIGN(UpdateEngineLibcrosProxyResolvedInterfaceProxyMock);
};
}  // namespace chromium
}  // namespace org

#endif  // ____CHROMEOS_DBUS_BINDING___UPDATE_ENGINE_INCLUDE_LIBCROS_DBUS_PROXY_MOCKS_H
