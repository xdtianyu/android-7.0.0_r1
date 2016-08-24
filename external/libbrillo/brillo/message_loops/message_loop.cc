// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/message_loops/message_loop.h>

#include <base/lazy_instance.h>
#include <base/logging.h>
#include <base/threading/thread_local.h>

namespace brillo {

namespace {

// A lazily created thread local storage for quick access to a thread's message
// loop, if one exists.  This should be safe and free of static constructors.
base::LazyInstance<base::ThreadLocalPointer<MessageLoop> >::Leaky lazy_tls_ptr =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

const MessageLoop::TaskId MessageLoop::kTaskIdNull = 0;

MessageLoop* MessageLoop::current() {
  DCHECK(lazy_tls_ptr.Pointer()->Get() != nullptr) <<
      "There isn't a MessageLoop for this thread. You need to initialize it "
      "first.";
  return lazy_tls_ptr.Pointer()->Get();
}

bool MessageLoop::ThreadHasCurrent() {
  return lazy_tls_ptr.Pointer()->Get() != nullptr;
}

void MessageLoop::SetAsCurrent() {
  DCHECK(lazy_tls_ptr.Pointer()->Get() == nullptr) <<
      "There's already a MessageLoop for this thread.";
  lazy_tls_ptr.Pointer()->Set(this);
}

void MessageLoop::ReleaseFromCurrent() {
  DCHECK(lazy_tls_ptr.Pointer()->Get() == this) <<
      "This is not the MessageLoop bound to the current thread.";
  lazy_tls_ptr.Pointer()->Set(nullptr);
}

MessageLoop::~MessageLoop() {
  if (lazy_tls_ptr.Pointer()->Get() == this)
    lazy_tls_ptr.Pointer()->Set(nullptr);
}

void MessageLoop::Run() {
  // Default implementation is to call RunOnce() blocking until there aren't
  // more tasks scheduled.
  while (!should_exit_ && RunOnce(true)) {}
  should_exit_ = false;
}

void MessageLoop::BreakLoop() {
  should_exit_ = true;
}

}  // namespace brillo
