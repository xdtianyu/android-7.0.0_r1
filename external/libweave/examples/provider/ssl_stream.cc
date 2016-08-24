// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/provider/ssl_stream.h"

#include <openssl/err.h>

#include <base/bind.h>
#include <base/bind_helpers.h>
#include <weave/provider/task_runner.h>

namespace weave {
namespace examples {

namespace {

void AddSslError(ErrorPtr* error,
                 const tracked_objects::Location& location,
                 const std::string& error_code,
                 unsigned long ssl_error_code) {
  ERR_load_BIO_strings();
  SSL_load_error_strings();
  Error::AddToPrintf(error, location, error_code, "%s: %s",
                     ERR_lib_error_string(ssl_error_code),
                     ERR_reason_error_string(ssl_error_code));
}

void RetryAsyncTask(provider::TaskRunner* task_runner,
                    const tracked_objects::Location& location,
                    const base::Closure& task) {
  task_runner->PostDelayedTask(FROM_HERE, task,
                               base::TimeDelta::FromMilliseconds(100));
}

}  // namespace

void SSLStream::SslDeleter::operator()(BIO* bio) const {
  BIO_free(bio);
}

void SSLStream::SslDeleter::operator()(SSL* ssl) const {
  SSL_free(ssl);
}

void SSLStream::SslDeleter::operator()(SSL_CTX* ctx) const {
  SSL_CTX_free(ctx);
}

SSLStream::SSLStream(provider::TaskRunner* task_runner,
                     std::unique_ptr<BIO, SslDeleter> stream_bio)
    : task_runner_{task_runner} {
  ctx_.reset(SSL_CTX_new(TLSv1_2_client_method()));
  CHECK(ctx_);
  ssl_.reset(SSL_new(ctx_.get()));

  SSL_set_bio(ssl_.get(), stream_bio.get(), stream_bio.get());
  stream_bio.release();  // Owned by ssl now.
  SSL_set_connect_state(ssl_.get());
}

SSLStream::~SSLStream() {
  CancelPendingOperations();
}

void SSLStream::RunTask(const base::Closure& task) {
  task.Run();
}

void SSLStream::Read(void* buffer,
                     size_t size_to_read,
                     const ReadCallback& callback) {
  int res = SSL_read(ssl_.get(), buffer, size_to_read);
  if (res > 0) {
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&SSLStream::RunTask, weak_ptr_factory_.GetWeakPtr(),
                   base::Bind(callback, res, nullptr)),
        {});
    return;
  }

  int err = SSL_get_error(ssl_.get(), res);

  if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
    return RetryAsyncTask(
        task_runner_, FROM_HERE,
        base::Bind(&SSLStream::Read, weak_ptr_factory_.GetWeakPtr(), buffer,
                   size_to_read, callback));
  }

  ErrorPtr weave_error;
  AddSslError(&weave_error, FROM_HERE, "read_failed", err);
  return task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(&SSLStream::RunTask, weak_ptr_factory_.GetWeakPtr(),
                 base::Bind(callback, 0, base::Passed(&weave_error))),
      {});
}

void SSLStream::Write(const void* buffer,
                      size_t size_to_write,
                      const WriteCallback& callback) {
  int res = SSL_write(ssl_.get(), buffer, size_to_write);
  if (res > 0) {
    buffer = static_cast<const char*>(buffer) + res;
    size_to_write -= res;
    if (size_to_write == 0) {
      return task_runner_->PostDelayedTask(
          FROM_HERE,
          base::Bind(&SSLStream::RunTask, weak_ptr_factory_.GetWeakPtr(),
                     base::Bind(callback, nullptr)),
          {});
    }

    return RetryAsyncTask(
        task_runner_, FROM_HERE,
        base::Bind(&SSLStream::Write, weak_ptr_factory_.GetWeakPtr(), buffer,
                   size_to_write, callback));
  }

  int err = SSL_get_error(ssl_.get(), res);

  if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
    return RetryAsyncTask(
        task_runner_, FROM_HERE,
        base::Bind(&SSLStream::Write, weak_ptr_factory_.GetWeakPtr(), buffer,
                   size_to_write, callback));
  }

  ErrorPtr weave_error;
  AddSslError(&weave_error, FROM_HERE, "write_failed", err);
  task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(&SSLStream::RunTask, weak_ptr_factory_.GetWeakPtr(),
                            base::Bind(callback, base::Passed(&weave_error))),
      {});
}

void SSLStream::CancelPendingOperations() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void SSLStream::Connect(
    provider::TaskRunner* task_runner,
    const std::string& host,
    uint16_t port,
    const provider::Network::OpenSslSocketCallback& callback) {
  SSL_library_init();

  char end_point[255];
  snprintf(end_point, sizeof(end_point), "%s:%u", host.c_str(), port);

  std::unique_ptr<BIO, SslDeleter> stream_bio(BIO_new_connect(end_point));
  CHECK(stream_bio);
  BIO_set_nbio(stream_bio.get(), 1);

  std::unique_ptr<SSLStream> stream{
      new SSLStream{task_runner, std::move(stream_bio)}};
  ConnectBio(std::move(stream), callback);
}

void SSLStream::ConnectBio(
    std::unique_ptr<SSLStream> stream,
    const provider::Network::OpenSslSocketCallback& callback) {
  BIO* bio = SSL_get_rbio(stream->ssl_.get());
  if (BIO_do_connect(bio) == 1)
    return DoHandshake(std::move(stream), callback);

  auto task_runner = stream->task_runner_;
  if (BIO_should_retry(bio)) {
    return RetryAsyncTask(
        task_runner, FROM_HERE,
        base::Bind(&SSLStream::ConnectBio, base::Passed(&stream), callback));
  }

  ErrorPtr error;
  AddSslError(&error, FROM_HERE, "connect_failed", ERR_get_error());
  task_runner->PostDelayedTask(
      FROM_HERE, base::Bind(callback, nullptr, base::Passed(&error)), {});
}

void SSLStream::DoHandshake(
    std::unique_ptr<SSLStream> stream,
    const provider::Network::OpenSslSocketCallback& callback) {
  int res = SSL_do_handshake(stream->ssl_.get());
  auto task_runner = stream->task_runner_;
  if (res == 1) {
    return task_runner->PostDelayedTask(
        FROM_HERE, base::Bind(callback, base::Passed(&stream), nullptr), {});
  }

  res = SSL_get_error(stream->ssl_.get(), res);

  if (res == SSL_ERROR_WANT_READ || res == SSL_ERROR_WANT_WRITE) {
    return RetryAsyncTask(
        task_runner, FROM_HERE,
        base::Bind(&SSLStream::DoHandshake, base::Passed(&stream), callback));
  }

  ErrorPtr error;
  AddSslError(&error, FROM_HERE, "handshake_failed", res);
  task_runner->PostDelayedTask(
      FROM_HERE, base::Bind(callback, nullptr, base::Passed(&error)), {});
}

}  // namespace examples
}  // namespace weave
