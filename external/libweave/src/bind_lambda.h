// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_BIND_LAMBDA_H_
#define LIBWEAVE_SRC_BIND_LAMBDA_H_

#include <base/bind.h>

////////////////////////////////////////////////////////////////////////////////
// This file is an extension to base/bind_internal.h and adds a RunnableAdapter
// class specialization that wraps a functor (including lambda objects), so
// they can be used in base::Callback/base::Bind constructs.
// By including this file you will gain the ability to write expressions like:
//    base::Callback<int(int)> callback = base::Bind([](int value) {
//      return value * value;
//    });
////////////////////////////////////////////////////////////////////////////////
namespace base {
namespace internal {

// LambdaAdapter is a helper class that specializes on different function call
// signatures and provides the RunType and Run() method required by
// RunnableAdapter<> class.
template <typename Lambda, typename Sig>
class LambdaAdapter;

// R(...)
template <typename Lambda, typename R, typename... Args>
class LambdaAdapter<Lambda, R (Lambda::*)(Args... args)> {
 public:
  typedef R(RunType)(Args...);
  LambdaAdapter(Lambda lambda) : lambda_(lambda) {}
  R Run(Args... args) { return lambda_(CallbackForward(args)...); }

 private:
  Lambda lambda_;
};

// R(...) const
template <typename Lambda, typename R, typename... Args>
class LambdaAdapter<Lambda, R (Lambda::*)(Args... args) const> {
 public:
  typedef R(RunType)(Args...);
  LambdaAdapter(Lambda lambda) : lambda_(lambda) {}
  R Run(Args... args) { return lambda_(CallbackForward(args)...); }

 private:
  Lambda lambda_;
};

template <typename Lambda>
class RunnableAdapter
    : public LambdaAdapter<Lambda, decltype(&Lambda::operator())> {
 public:
  explicit RunnableAdapter(Lambda lambda)
      : LambdaAdapter<Lambda, decltype(&Lambda::operator())>(lambda) {}
};

}  // namespace internal
}  // namespace base

#endif  // LIBWEAVE_SRC_BIND_LAMBDA_H_
