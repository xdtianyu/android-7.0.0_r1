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

#ifndef WEBSERVER_LIBWEBSERV_PROTOCOL_HANDLER_H_
#define WEBSERVER_LIBWEBSERV_PROTOCOL_HANDLER_H_

#include <memory>
#include <set>
#include <string>

#include <base/callback_forward.h>
#include <base/macros.h>
#include <brillo/secure_blob.h>

#include <libwebserv/export.h>
#include <libwebserv/request_handler_interface.h>

namespace libwebserv {

// Wrapper around a protocol handler (e.g. HTTP or HTTPs).
// ProtocolHandler allows consumers to add request handlers on a given protocol.
// When the ProtocolHandler is connected, allows users to read port and protocol
// information.
class LIBWEBSERV_EXPORT ProtocolHandler {
 public:
  ProtocolHandler() = default;
  virtual ~ProtocolHandler() = default;

  // Returns true if the protocol handler object is backed by a ProtocolHandler
  // on the remote web server and is capable of processing incoming requests.
  virtual bool IsConnected() const = 0;

  // Handler's name identifier (as provided in "name" setting of config file).
  // Standard/default handler names are "http" and "https".
  virtual std::string GetName() const = 0;

  // Returns the ports the handler is bound to. There could be multiple.
  // If the handler is not connected to the server, this will return an empty
  // set.
  virtual std::set<uint16_t> GetPorts() const = 0;

  // Returns the transport protocol that is served by this handler.
  // Can be either "http" or "https".
  // If the handler is not connected to the server, this will return an empty
  // set.
  virtual std::set<std::string> GetProtocols() const = 0;

  // Returns a SHA-256 fingerprint of HTTPS certificate used. Returns an empty
  // byte buffer if this handler does not serve the HTTPS protocol.
  // If the handler is not connected to the server, this will return an empty
  // array.
  virtual brillo::Blob GetCertificateFingerprint() const = 0;

  // Adds a request handler for given |url|. If the |url| ends with a '/', this
  // makes the handler respond to any URL beneath this path.
  // Note that it is not possible to add a specific handler just for the root
  // path "/". Doing so means "respond to any URL".
  // |method| is optional request method verb, such as "GET" or "POST".
  // If |method| is empty, the handler responds to any request verb.
  // If there are more than one handler for a given request, the most specific
  // match is chosen. For example, if there are the following handlers provided:
  //    - A["/foo/",  ""]
  //    - B["/foo/bar", "GET"]
  //    - C["/foo/bar", ""]
  // Here is what handlers are called when making certain requests:
  //    - GET("/foo/bar")   => B[]
  //    - POST("/foo/bar")  => C[]
  //    - PUT("/foo/bar")   => C[]
  //    - GET("/foo/baz")   => A[]
  //    - GET("/foo")       => 404 Not Found
  // This functions returns a handler ID which can be used later to remove
  // the handler.
  //
  // The handler registration information is stored inside ProtocolHandler and
  // is used to register the handlers with the web server daemon when it becomes
  // available. This also happens when the web server goes away and then comes
  // back (e.g. restarted). So, there is no need to re-register the handlers
  // once the web server process is restarted.
  virtual int AddHandler(const std::string& url,
                         const std::string& method,
                         std::unique_ptr<RequestHandlerInterface> handler) = 0;

  // Similar to AddHandler() above but the handler is just a callback function.
  virtual int AddHandlerCallback(
      const std::string& url,
      const std::string& method,
      const base::Callback<RequestHandlerInterface::HandlerSignature>&
          handler_callback) = 0;

  // Removes the handler with the specified |handler_id|.
  // Returns false if the handler with the given ID is not found.
  virtual bool RemoveHandler(int handler_id) = 0;

  static const char kHttp[];
  static const char kHttps[];

 private:
  DISALLOW_COPY_AND_ASSIGN(ProtocolHandler);
};

}  // namespace libwebserv

#endif  // WEBSERVER_LIBWEBSERV_PROTOCOL_HANDLER_H_
