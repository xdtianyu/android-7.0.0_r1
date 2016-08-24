//
// Copyright (C) 2011 The Android Open Source Project
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
//

#ifndef SHILL_ASYNC_CONNECTION_H_
#define SHILL_ASYNC_CONNECTION_H_

#include <memory>
#include <string>

#include <base/callback.h>

#include "shill/refptr_types.h"

namespace shill {

class EventDispatcher;
class IOHandler;
class IPAddress;
class Sockets;

// The AsyncConnection class implements an asynchronous
// outgoing TCP connection.  When passed an IPAddress and
// port, and it will notify the caller when the connection
// is made.  It can also be passed an interface name to
// bind the local side of the connection.
class AsyncConnection {
 public:
  // If non-empty |interface_name| specifies an local interface from which
  // to originate the connection.
  AsyncConnection(const std::string& interface_name,
                  EventDispatcher* dispatcher,
                  Sockets* sockets,
                  const base::Callback<void(bool, int)>& callback);
  virtual ~AsyncConnection();

  // Open a connection given an IP address and port (in host order).
  // When the connection completes, |callback| will be called with the
  // a boolean (indicating success if true) and an fd of the opened socket
  // (in the success case).  If successful, ownership of this open fd is
  // passed to the caller on execution of the callback.
  //
  // This function (Start) returns true if the connection is in progress,
  // or if the connection has immediately succeeded (the callback will be
  // called in this case).  On success the callback may be called before
  // Start() returns to its caller.  On failure to start the connection,
  // this function returns false, but does not execute the callback.
  //
  // Calling Start() on an AsyncConnection that is already Start()ed is
  // an error.
  virtual bool Start(const IPAddress& address, int port);

  // Stop the open connection, closing any fds that are still owned.
  // Calling Stop() on an unstarted or Stop()ped AsyncConnection is
  // a no-op.
  virtual void Stop();

  std::string error() const { return error_; }

 private:
  friend class AsyncConnectionTest;

  void OnConnectCompletion(int fd);

  // Initiate a socket connection to given IP address and port (in host order).
  int ConnectTo(const IPAddress& address, int port);

  std::string interface_name_;
  EventDispatcher* dispatcher_;
  Sockets* sockets_;
  base::Callback<void(bool, int)> callback_;
  std::string error_;
  int fd_;
  base::Callback<void(int)> connect_completion_callback_;
  std::unique_ptr<IOHandler> connect_completion_handler_;

  DISALLOW_COPY_AND_ASSIGN(AsyncConnection);
};

}  // namespace shill

#endif  // SHILL_ASYNC_CONNECTION_H_
