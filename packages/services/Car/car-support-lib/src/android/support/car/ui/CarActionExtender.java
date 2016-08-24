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
package android.support.car.ui;

import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.NotificationCompat;

/**
 * Android Auto {@link android.app.Notification.Action} extender.
 * NOTE: this will move into the platform and support-lib when the API stabilizes.
 */
public class CarActionExtender implements NotificationCompat.Action.Extender {
    private static final String EXTRA_AUTO_EXTENDER = "android.auto.EXTENSIONS";
    private static final String EXTRA_INTENT = "intent";

    private Intent mIntent;

    public CarActionExtender() {
    }

    public CarActionExtender(NotificationCompat.Action action) {
        Bundle autoBundle = action.getExtras();

        if (autoBundle != null) {
            mIntent = autoBundle.getParcelable(EXTRA_INTENT);
        }
    }

    @Override
    public NotificationCompat.Action.Builder extend(NotificationCompat.Action.Builder builder) {
        Bundle autoBundle = new Bundle();

        autoBundle.putParcelable(EXTRA_INTENT, mIntent);

        builder.getExtras().putBundle(EXTRA_AUTO_EXTENDER, autoBundle);
        return builder;
    }

    public void setIntent(Intent intent) {
        mIntent = intent;
    }

    public Intent getIntent() {
        return mIntent;
    }
}
