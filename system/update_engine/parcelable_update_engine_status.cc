//
// Copyright (C) 2016 The Android Open Source Project
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

#include "update_engine/parcelable_update_engine_status.h"

#include <binder/Parcel.h>

namespace android {
namespace brillo {

status_t ParcelableUpdateEngineStatus::writeToParcel(Parcel* parcel) const {
  status_t status;

  status = parcel->writeInt64(last_checked_time_);
  if (status != OK) {
    return status;
  }

  status = parcel->writeDouble(progress_);
  if (status != OK) {
    return status;
  }

  status = parcel->writeString16(current_operation_);
  if (status != OK) {
    return status;
  }

  status = parcel->writeString16(new_version_);
  if (status != OK) {
    return status;
  }

  return parcel->writeInt64(new_size_);
}

status_t ParcelableUpdateEngineStatus::readFromParcel(const Parcel* parcel) {
  status_t status;

  status = parcel->readInt64(&last_checked_time_);
  if (status != OK) {
    return status;
  }

  status = parcel->readDouble(&progress_);
  if (status != OK) {
    return status;
  }

  status = parcel->readString16(&current_operation_);
  if (status != OK) {
    return status;
  }

  status = parcel->readString16(&new_version_);
  if (status != OK) {
    return status;
  }

  return parcel->readInt64(&new_size_);
}

}  // namespace brillo
}  // namespace android
