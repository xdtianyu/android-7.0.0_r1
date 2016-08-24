// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_EXAMPLES_PROVIDER_EVENT_DELETER_H
#define LIBWEAVE_EXAMPLES_PROVIDER_EVENT_DELETER_H

#include <memory>

#include <evhtp.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <openssl/ssl.h>

namespace weave {
namespace examples {

// Defines overloaded deletion methods for various event_ objects
// so we can use one unique_ptr definition for all of them
class EventDeleter {
 public:
  void operator()(evbuffer* buf) { evbuffer_free(buf); }
  void operator()(evhtp_t* evhtp) {
    if (evhtp->ssl_ctx) {
      // Work around a double-free bug in recent versions of libevhtp.
      // https://github.com/ellzey/libevhtp/pull/208
      SSL_CTX_free(evhtp->ssl_ctx);
      evhtp->ssl_ctx = nullptr;
    }
    evhtp_unbind_socket(evhtp);
    evhtp_free(evhtp);
  }
  void operator()(evhtp_connection_t* conn) { evhtp_connection_free(conn); }
  void operator()(evhtp_request_t* req) { evhtp_request_free(req); }
  void operator()(event_base* base) { event_base_free(base); }
  void operator()(event* ev) {
    event_del(ev);
    event_free(ev);
  }
};

template <typename T>
using EventPtr = std::unique_ptr<T, EventDeleter>;

}  // namespace examples
}  // namespace weave

#endif  // LIBWEAVE_EXAMPLES_PROVIDER_EVENT_DELETER_H
