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

package android.assist.testapp;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

public class LifecycleActivity extends Activity {
    private static final String TAG = "LifecycleActivity";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "LifecycleActivity created");
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.i(TAG, "Activity has resumed");
        sendBroadcast(new Intent("android.intent.action.lifecycle_hasResumed"));
    }

    @Override
    protected void onPause() {
        Log.i(TAG, "activity was paused");
        sendBroadcast(new Intent("android.intent.action.lifecycle_onpause"));
        super.onPause();
    }

    @Override
    protected void onStop() {
        Log.i(TAG, "activity was stopped");
        sendBroadcast(new Intent("android.intent.action.lifecycle_onstop"));
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        Log.i(TAG, "activity was destroyed");
        sendBroadcast(new Intent("android.intent.action.lifecycle_ondestroy"));
        super.onDestroy();
    }
}
