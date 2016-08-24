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
 * limitations under the License
 */
package com.android.phone.vvm.omtp;

import com.android.internal.telephony.PhoneFactory;

/**
 * Helper methods for adding to Telephony local logs.
 */
public class LocalLogHelper {
    public static final String KEY = "OmtpVvm";
    private static final int MAX_OMTP_VVM_LOGS = 20;

    public static void log(String tag, String log) {
        try {
            PhoneFactory.addLocalLog(KEY, MAX_OMTP_VVM_LOGS);
        } catch (IllegalArgumentException e){
        } finally {
            PhoneFactory.localLog(KEY, tag + ": " + log);
        }
    }
}
