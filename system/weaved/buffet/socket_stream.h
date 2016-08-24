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

#ifndef BUFFET_SOCKET_STREAM_H_
#define BUFFET_SOCKET_STREAM_H_

#include <string>

#include <base/callback.h>
#include <base/macros.h>
#include <brillo/streams/stream.h>
#include <weave/provider/network.h>
#include <weave/stream.h>

namespace buffet {

class SocketStream : public weave::Stream {
 public:
  explicit SocketStream(brillo::StreamPtr ptr) : ptr_{std::move(ptr)} {}

  ~SocketStream() override = default;

  void Read(void* buffer,
            size_t size_to_read,
            const ReadCallback& callback) override;

  void Write(const void* buffer,
             size_t size_to_write,
             const WriteCallback& callback) override;

  void CancelPendingOperations() override;

  static std::unique_ptr<weave::Stream> ConnectBlocking(const std::string& host,
                                                        uint16_t port);

  static void TlsConnect(
      std::unique_ptr<weave::Stream> socket,
      const std::string& host,
      const weave::provider::Network::OpenSslSocketCallback& callback);

 private:
  brillo::StreamPtr ptr_;
  DISALLOW_COPY_AND_ASSIGN(SocketStream);
};

}  // namespace buffet

#endif  // BUFFET_SOCKET_STREAM_H_
