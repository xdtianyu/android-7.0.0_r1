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

package com.android.tv.data.epg;

import android.content.Context;

import com.android.tv.data.Channel;
import com.android.tv.data.Program;

import java.util.Collections;
import java.util.List;

/**
 * A stub class to read EPG.
 */
public class StubEpgReader implements EpgReader{
    public StubEpgReader(Context context) {
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public long getEpgTimestamp() {
        return 0;
    }

    @Override
    public List<Channel> getChannels() {
        return Collections.EMPTY_LIST;
    }

    @Override
    public List<Program> getPrograms(long channelId) {
        return Collections.EMPTY_LIST;
    }
}
