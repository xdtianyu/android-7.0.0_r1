// Copyright 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef WEBSERVER_WEBSERVD_CONFIG_H_
#define WEBSERVER_WEBSERVD_CONFIG_H_

#include <string>
#include <vector>

#include <base/files/file_path.h>
#include <brillo/errors/error.h>
#include <brillo/secure_blob.h>

namespace webservd {

extern const char kDefaultLogDirectory[];

// This class contains global server configuration.
struct Config final {
 public:
  // Configuration of one specific protocol handler.
  struct ProtocolHandler final {
    ~ProtocolHandler();
    // Protocol Handler Name.
    std::string name;
    // Port to use.
    uint16_t port{0};
    // Specifies whether the handler is for HTTPS (true) or HTTP (false).
    bool use_tls{false};
    // Interface name to use if the protocol handler should work only on
    // particular network interface. If empty, the TCP socket will be open
    // on the specified port for all network interfaces.
    std::string interface_name;
    // For HTTPS handlers, these specify the certificates/private keys used
    // during TLS handshake and communication session. For HTTP protocol
    // handlers these fields are not used and are empty.
    brillo::SecureBlob private_key;
    brillo::Blob certificate;
    brillo::Blob certificate_fingerprint;

    // Custom socket created for protocol handlers that are bound to specific
    // network interfaces only. SO_BINDTODEVICE option on a socket does exactly
    // what is required but it needs root access. So we create those sockets
    // before we drop privileges.
    int socket_fd{-1};
  };

  // List of all registered protocol handlers for the web server.
  std::vector<ProtocolHandler> protocol_handlers;

  // Specifies whether additional debugging information should be included.
  // When set, this turns out additional diagnostic logging in libmicrohttpd as
  // well as includes additional information in error responses delivered to
  // HTTP clients.
  bool use_debug{false};

  // Specifies whether IPv6 is enabled and should be used by the server.
  bool use_ipv6{true};

  // Output directory for web server's request log in Common Log Format
  // (see http://www.w3.org/Daemon/User/Config/Logging.html).
  // The files in this directory contain only the "official" request logs, not
  // general logging messages from the webserver, which still go to the standard
  // system log.
  std::string log_directory{kDefaultLogDirectory};

  // Default request timeout (in seconds).
  int default_request_timeout_seconds{60};
};

// Initializes the config with default preset settings (two handlers, one for
// HTTP on port 80 and one for HTTPS on port 443).
void LoadDefaultConfig(Config* config);

// Loads server configuration form specified file. The file is expected
// to exist and contain a valid configuration in JSON format.
// Returns false on error (whether opening/reading the file or parsing JSON
// content).
bool LoadConfigFromFile(const base::FilePath& json_file_path, Config* config);

// Loads the configuration from a string containing JSON data.
// In case of parsing or configuration validation errors, returns false and
// specifies the reason for the failure in |error| object.
bool LoadConfigFromString(const std::string& config_json,
                          Config* config,
                          brillo::ErrorPtr* error);

}  // namespace webservd

#endif  // WEBSERVER_WEBSERVD_CONFIG_H_
