/*
 * Copyright (C) 2010 The Android Open Source Project
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

package vogar.target;

import android.util.Log;

/**
 * Logs everything to Android's log.
 */
public final class AndroidLog implements vogar.Log {

    private final String tag;

    public AndroidLog(String tag) {
        this.tag = tag;
    }

    @Override public void verbose(String s) {
        Log.v(tag, s);
    }

    @Override public void info(String s) {
        Log.i(tag, s);
    }

    @Override public void info(String s, Throwable exception) {
        Log.i(tag, s, exception);
    }

    @Override public void warn(String s) {
        Log.w(tag, s);
    }
}
