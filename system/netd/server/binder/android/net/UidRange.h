/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NETD_SERVER_ANDROID_NET_UID_RANGE_H
#define NETD_SERVER_ANDROID_NET_UID_RANGE_H

#include <binder/Parcelable.h>

namespace android {

namespace net {

/*
 * C++ implementation of UidRange, a contiguous range of UIDs.
 */
class UidRange : public Parcelable {
public:
    UidRange() = default;
    virtual ~UidRange() = default;
    UidRange(const UidRange& range) = default;
    UidRange(int32_t start, int32_t stop);

    status_t writeToParcel(Parcel* parcel) const override;
    status_t readFromParcel(const Parcel* parcel) override;

    /*
     * Setters for UidRange start and stop UIDs.
     */
    void setStart(int32_t uid);
    void setStop(int32_t uid);

    /*
     * Getters for UidRange start and stop UIDs.
     */
    int32_t getStart() const;
    int32_t getStop() const;

    friend bool operator<(const UidRange& lhs, const UidRange& rhs) {
        return lhs.mStart != rhs.mStart ? (lhs.mStart < rhs.mStart) : (lhs.mStop < rhs.mStop);
    }

    friend bool operator==(const UidRange& lhs, const UidRange& rhs) {
        return (lhs.mStart == rhs.mStart && lhs.mStop == rhs.mStop);
    }

    friend bool operator!=(const UidRange& lhs, const UidRange& rhs) {
        return !(lhs == rhs);
    }

private:
    int32_t mStart = -1;
    int32_t mStop = -1;
};

}  // namespace net

}  // namespace android

#endif  // NETD_SERVER_ANDROID_NET_UID_RANGE_H
