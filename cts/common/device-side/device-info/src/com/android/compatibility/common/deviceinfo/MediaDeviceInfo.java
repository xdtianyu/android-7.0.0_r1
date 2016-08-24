/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.compatibility.common.deviceinfo;

import android.app.ActivityManager;
import android.app.ActivityManager.MemoryInfo;
import android.content.Context;
import android.os.Bundle;
import android.os.Build;

import android.media.CamcorderProfile;
import android.media.MediaCodecInfo;
import android.media.MediaCodecInfo.CodecProfileLevel;
import android.media.MediaCodecList;

import com.android.compatibility.common.util.DeviceInfoStore;

import java.util.ArrayList;
import java.util.List;

/**
 * Media information collector.
 */
public final class MediaDeviceInfo extends DeviceInfo {

    @Override
    protected void collectDeviceInfo(DeviceInfoStore store) throws Exception {
        store.startArray("media_codec_info");
        for (int i = 0; i < MediaCodecList.getCodecCount(); i++) {
            MediaCodecInfo info = MediaCodecList.getCodecInfoAt(i);

            store.startGroup();
            store.addResult("name", info.getName());
            store.addResult("encoder", info.isEncoder());

            store.startArray("supported_type");
            for (String type : info.getSupportedTypes()) {

                store.startGroup();
                store.addResult("type", type);
                if (info.getCapabilitiesForType(type).profileLevels.length > 0) {
                    store.startArray("codec_profile_level");
                    for (CodecProfileLevel profileLevel :
                             info.getCapabilitiesForType(type).profileLevels) {
                        store.startGroup();
                        store.addResult("level", profileLevel.level);
                        store.addResult("profile", profileLevel.profile);
                        store.endGroup();
                    }
                    store.endArray(); // codec_profile_level
                }
                store.endGroup();
            }
            store.endArray();
            store.endGroup();
        }

        store.endArray(); // media_codec_profile
    }
}
