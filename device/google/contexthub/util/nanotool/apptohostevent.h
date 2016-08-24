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

#ifndef APP_TO_HOST_EVENT_H_
#define APP_TO_HOST_EVENT_H_

#include "contexthub.h"
#include "nanomessage.h"

namespace android {

// Copied from nanohub eventnums.h
struct HostHubRawPacket {
    uint64_t appId;
    uint8_t dataLen; //not incl this header, 128 bytes max
    //raw data in unspecified format here
} __attribute((packed));

// The u64 appId used in nanohub is 40 bits vendor ID + 24 bits app ID (see seos.h)
constexpr uint64_t MakeAppId(uint64_t vendorId, uint32_t appId) {
    return (vendorId << 24) | (appId & 0x00FFFFFF);
}

constexpr uint64_t kAppIdVendorGoogle = 0x476f6f676cULL; // "Googl"

constexpr uint64_t kAppIdBoschBmi160Bmm150 = MakeAppId(kAppIdVendorGoogle, 2);
constexpr uint64_t kAppIdBoschBmp280       = MakeAppId(kAppIdVendorGoogle, 5);
constexpr uint64_t kAppIdAmsTmd2772        = MakeAppId(kAppIdVendorGoogle, 9);
constexpr uint64_t kAppIdRohmRpr0521       = MakeAppId(kAppIdVendorGoogle, 10);
constexpr uint64_t kAppIdAmsTmd4903        = MakeAppId(kAppIdVendorGoogle, 12);

/*
 * These classes represent events sent with event type EVT_APP_TO_HOST. This is
 * a generic container for arbitrary application-specific data, and is used for
 * passing back sensor calibration results, implementing app download, etc. The
 * parser must know the application ID to determine the data format.
 */

class AppToHostEvent : public ReadEventResponse {
  public:
    /*
     * Constructs and populates an AppToHostEvent instance. Returns nullptr if
     * the packet is malformed. The rest of the methods in this class are not
     * guaranteed to be safe unless the object is constructed from this
     * function.
     */
    static std::unique_ptr<AppToHostEvent> FromBytes(
        const std::vector<uint8_t>& buffer);

    uint64_t GetAppId() const;
    // Gets the length of the application-specific data segment
    uint8_t GetDataLen() const;
    // Returns a pointer to the application-specific data (i.e. past the header)
    const uint8_t *GetDataPtr() const;

    bool IsCalibrationEventForSensor(SensorType sensor_type) const;
    virtual bool IsValid() const;

  protected:
    const HostHubRawPacket *GetTypedData() const;
};

#define SENSOR_APP_MSG_CALIBRATION_RESULT (0)

struct SensorAppEventHeader {
    uint8_t msgId;
    uint8_t sensorType;
    uint8_t status; // 0 for success
} __attribute__((packed));

struct SingleAxisCalibrationResult : public SensorAppEventHeader {
    int32_t bias;
} __attribute__((packed));

struct TripleAxisCalibrationResult : public SensorAppEventHeader {
    int32_t xBias;
    int32_t yBias;
    int32_t zBias;
} __attribute__((packed));

struct FloatCalibrationResult : public SensorAppEventHeader {
    float value;
} __attribute__((packed));

struct FourAxisCalibrationResult : public SensorAppEventHeader {
    int32_t xBias;
    int32_t yBias;
    int32_t zBias;
    int32_t wBias;
} __attribute__((packed));


}  // namespace android

#endif // APP_TO_HOST_EVENT_H_
