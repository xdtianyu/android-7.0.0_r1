// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/streams/openssl_stream_bio.h>

#include <openssl/bio.h>

#include <base/numerics/safe_conversions.h>
#include <brillo/streams/stream.h>

namespace brillo {

namespace {

// Internal functions for implementing OpenSSL BIO on brillo::Stream.
int stream_write(BIO* bio, const char* buf, int size) {
  brillo::Stream* stream = static_cast<brillo::Stream*>(bio->ptr);
  size_t written = 0;
  BIO_clear_retry_flags(bio);
  if (!stream->WriteNonBlocking(buf, size, &written, nullptr))
    return -1;

  if (written == 0) {
    // Socket's output buffer is full, try again later.
    BIO_set_retry_write(bio);
    return -1;
  }
  return base::checked_cast<int>(written);
}

int stream_read(BIO* bio, char* buf, int size) {
  brillo::Stream* stream = static_cast<brillo::Stream*>(bio->ptr);
  size_t read = 0;
  BIO_clear_retry_flags(bio);
  bool eos = false;
  if (!stream->ReadNonBlocking(buf, size, &read, &eos, nullptr))
    return -1;

  if (read == 0 && !eos) {
    // If no data is available on the socket and it is still not closed,
    // ask OpenSSL to try again later.
    BIO_set_retry_read(bio);
    return -1;
  }
  return base::checked_cast<int>(read);
}

// NOLINTNEXTLINE(runtime/int)
long stream_ctrl(BIO* bio, int cmd, long /* num */, void* /* ptr */) {
  if (cmd == BIO_CTRL_FLUSH) {
    brillo::Stream* stream = static_cast<brillo::Stream*>(bio->ptr);
    return stream->FlushBlocking(nullptr) ? 1 : 0;
  }
  return 0;
}

int stream_new(BIO* bio) {
  bio->shutdown = 0;  // By default do not close underlying stream on shutdown.
  bio->init = 0;
  bio->num = -1;  // not used.
  return 1;
}

int stream_free(BIO* bio) {
  if (!bio)
    return 0;

  if (bio->init) {
    bio->ptr = nullptr;
    bio->init = 0;
  }
  return 1;
}

// BIO_METHOD structure describing the BIO built on top of brillo::Stream.
BIO_METHOD stream_method = {
    0x7F | BIO_TYPE_SOURCE_SINK,  // type: 0x7F is an arbitrary unused type ID.
    "stream",      // name
    stream_write,  // write function
    stream_read,   // read function
    nullptr,       // puts function, not implemented
    nullptr,       // gets function, not implemented
    stream_ctrl,   // control function
    stream_new,    // creation
    stream_free,   // free
    nullptr,       // callback function, not used
};

}  // anonymous namespace

BIO* BIO_new_stream(brillo::Stream* stream) {
  BIO* bio = BIO_new(&stream_method);
  if (bio) {
    bio->ptr = stream;
    bio->init = 1;
  }
  return bio;
}

}  // namespace brillo
