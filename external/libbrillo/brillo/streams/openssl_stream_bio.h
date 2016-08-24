// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_STREAMS_OPENSSL_STREAM_BIO_H_
#define LIBBRILLO_BRILLO_STREAMS_OPENSSL_STREAM_BIO_H_

#include <brillo/brillo_export.h>

// Forward-declare BIO as an alias to OpenSSL's internal bio_st structure.
using BIO = struct bio_st;

namespace brillo {

class Stream;

// Creates a new BIO that uses the brillo::Stream as the back-end storage.
// The created BIO does *NOT* own the |stream| and the stream must out-live
// the BIO.
// At the moment, only BIO_read and BIO_write operations are supported as well
// as BIO_flush. More functionality could be added to this when/if needed.
// The returned BIO performs *NON-BLOCKING* IO on the underlying stream.
BRILLO_EXPORT BIO* BIO_new_stream(brillo::Stream* stream);

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_STREAMS_OPENSSL_STREAM_BIO_H_
