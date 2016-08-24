//
// Copyright (C) 2013 The Android Open Source Project
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

#ifndef SHILL_RESULT_AGGREGATOR_H_
#define SHILL_RESULT_AGGREGATOR_H_

#include <base/cancelable_callback.h>
#include <base/macros.h>
#include <base/memory/ref_counted.h>

#include "shill/callbacks.h"
#include "shill/error.h"

namespace shill {

class EventDispatcher;

// The ResultAggregator is used to aggregate the result of multiple
// asynchronous operations. To use: construct a ResultAggregator, and
// Bind its ReportResult methods to some Callbacks. The ResultAggregator
// can also be constructed with an EventDispatcher pointer and timeout delay if
// we want to wait for a limited period of time for asynchronous operations
// to complete.
//
// When the Callbacks are destroyed, they will drop their references
// to the ResultAggregator. When all references to the ResultAggregator are
// destroyed, or if a timeout occurs, the ResultAggregator will invoke
// |callback_|. |callback_| will only be invoked exactly once by whichever of
// these two events occurs first.
//
// |callback_| will see Error type of Success if all Callbacks reported
// Success to ResultAggregator. If the timeout occurs, |callback_| will see
// Error type of OperationTimeout. Otherwise, |callback_| will see the first of
// the Errors reported to ResultAggregator.
//
// Note: If no callbacks invoked ReportResult and the ResultAggregator is
// destructed (before timing out), the ResultAggregator will be destructed
// silently and will not invoke |callback_|. This can cause unexpected
// behavior if the user expects |callback_| to be invoked after the
// result_aggregator goes out of scope. For example:
//
// void Manager::Foo() {
//   auto result_aggregator(make_scoped_refptr(new ResultAggregator(
//       Bind(&Manager::Func, AsWeakPtr()), dispatcher_, 1000)));
//   if (condition) {
//     LOG(ERROR) << "Failed!"
//     return;
//   }
//   ResultCallback aggregator_callback(
//       Bind(&ResultAggregator::ReportResult, result_aggregator));
//   devices_[0]->OnBeforeSuspend(aggregator_callback);
// }
//
// If |condition| is true and the function returns without passing the
// reference to |result_aggregator| to devices_[0], |result_aggregator| will
// be destructed upon returning from Manager::Foo and will never call
// Manager::Func(). This is problematic if the owner of |result_aggregator|
// expects Manager::Func to be called when |result_aggregator| goes out of
// scope.
//
// Another anomaly that can occur is it the ResultCallback that is being
// passed around is allowed to go out to scope without being run. If at least
// one object ran the ResultCallback, the ResultAggregator will invoke
// |callback_| upon going out of scope, even though there exists an object
// that was passed a ResultCallback but did not actually run it. This is
// incorrect behavior, as we assume that |callback_| will only be run if
// the ResultAggregator times out or if all objects that were passed the
// ResultCallback run it.
//
// In order to ensure that ResultAggregator behaves as it is meant to, follow
// these conventions when using it:
//   1) Always run any ResultCallback that is passed around before letting it
//      go out of scope.
//   2) If the ResultAggregator will go out of scope without passing any
//      ResultCallback objects (i.e. references to itself) to other objects,
//      invoke the callback the ResultAggregator was constructed with directly
//      before letting ResultAggregator go out of scope.

class ResultAggregator : public base::RefCounted<ResultAggregator> {
 public:
  explicit ResultAggregator(const ResultCallback& callback);
  ResultAggregator(const ResultCallback& callback, EventDispatcher* dispatcher,
                   int timeout_milliseconds);
  virtual ~ResultAggregator();

  void ReportResult(const Error& error);

 private:
  // Callback for timeout registered with EventDispatcher.
  void Timeout();

  base::WeakPtrFactory<ResultAggregator> weak_ptr_factory_;
  const ResultCallback callback_;
  base::CancelableClosure timeout_callback_;
  bool got_result_;
  bool timed_out_;
  Error error_;

  DISALLOW_COPY_AND_ASSIGN(ResultAggregator);
};

}  // namespace shill

#endif  // SHILL_RESULT_AGGREGATOR_H_
