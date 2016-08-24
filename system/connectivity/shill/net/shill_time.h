//
// Copyright (C) 2012 The Android Open Source Project
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

#ifndef SHILL_NET_SHILL_TIME_H_
#define SHILL_NET_SHILL_TIME_H_

#include <sys/time.h>
#include <time.h>

#include <string>

#include <base/lazy_instance.h>

#include "shill/net/shill_export.h"

namespace shill {

// Timestamp encapsulates a |monotonic| and a |boottime| clock that can be used
// to compare the relative order and distance of events as well as a
// |wall_clock| time that can be used for presenting the time in human-readable
// format. Note that the monotonic clock does not necessarily advance during
// suspend, while boottime clock does include any time that the system is
// suspended.
struct SHILL_EXPORT Timestamp {
  Timestamp() : monotonic{} {}
  Timestamp(const struct timeval& in_monotonic,
            const struct timeval& in_boottime,
            const std::string& in_wall_clock)
      : monotonic(in_monotonic),
        boottime(in_boottime),
        wall_clock(in_wall_clock) {}

  struct timeval monotonic;
  struct timeval boottime;
  std::string wall_clock;
};

// A "sys/time.h" abstraction allowing mocking in tests.
class SHILL_EXPORT Time {
 public:
  virtual ~Time();

  static Time* GetInstance();

  // Returns CLOCK_MONOTONIC time, or 0 if a failure occurred.
  virtual bool GetSecondsMonotonic(time_t* seconds);

  // Returns CLOCK_BOOTTIME time, or 0 if a failure occurred.
  virtual bool GetSecondsBoottime(time_t* seconds);

  // On success, sets |tv| to CLOCK_MONOTONIC time, and returns 0.
  virtual int GetTimeMonotonic(struct timeval* tv);

  // On success, sets |tv| to CLOCK_BOOTTIME time, and returns 0.
  virtual int GetTimeBoottime(struct timeval* tv);

  // gettimeofday
  virtual int GetTimeOfDay(struct timeval* tv, struct timezone* tz);

  // Returns a snapshot of the current time.
  virtual Timestamp GetNow();

  virtual time_t GetSecondsSinceEpoch() const;

  static std::string FormatTime(const struct tm& date_time, suseconds_t usec);

 protected:
  Time();

 private:
  friend struct base::DefaultLazyInstanceTraits<Time>;

  DISALLOW_COPY_AND_ASSIGN(Time);
};

}  // namespace shill

#endif  // SHILL_NET_SHILL_TIME_H_
