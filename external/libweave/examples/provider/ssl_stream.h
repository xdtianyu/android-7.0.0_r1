// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_EXAMPLES_PROVIDER_SSL_STREAM_H_
#define LIBWEAVE_EXAMPLES_PROVIDER_SSL_STREAM_H_

#include <openssl/ssl.h>

#include <base/memory/weak_ptr.h>
#include <weave/provider/network.h>
#include <weave/stream.h>

namespace weave {

namespace provider {
class TaskRunner;
}

namespace examples {

class SSLStream : public Stream {
 public:
  ~SSLStream() override;

  void Read(void* buffer,
            size_t size_to_read,
            const ReadCallback& callback) override;

  void Write(const void* buffer,
             size_t size_to_write,
             const WriteCallback& callback) override;

  void CancelPendingOperations() override;

  static void Connect(provider::TaskRunner* task_runner,
                      const std::string& host,
                      uint16_t port,
                      const provider::Network::OpenSslSocketCallback& callback);

 private:
  struct SslDeleter {
    void operator()(BIO* bio) const;
    void operator()(SSL* ssl) const;
    void operator()(SSL_CTX* ctx) const;
  };

  SSLStream(provider::TaskRunner* task_runner,
            std::unique_ptr<BIO, SslDeleter> stream_bio);

  static void ConnectBio(
      std::unique_ptr<SSLStream> stream,
      const provider::Network::OpenSslSocketCallback& callback);
  static void DoHandshake(
      std::unique_ptr<SSLStream> stream,
      const provider::Network::OpenSslSocketCallback& callback);

  // Send task to this method with WeakPtr if callback should not be executed
  // after SSLStream is destroyed.
  void RunTask(const base::Closure& task);

  provider::TaskRunner* task_runner_{nullptr};
  std::unique_ptr<SSL_CTX, SslDeleter> ctx_;
  std::unique_ptr<SSL, SslDeleter> ssl_;

  base::WeakPtrFactory<SSLStream> weak_ptr_factory_{this};
};

}  // namespace examples
}  // namespace weave

#endif  // LIBWEAVE_EXAMPLES_PROVIDER_SSL_STREAM_H_
