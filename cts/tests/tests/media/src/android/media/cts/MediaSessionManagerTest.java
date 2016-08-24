/*
 * Copyright (C) 2014 The Android Open Source Project
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
package android.media.cts;

import android.content.Context;
import android.media.session.MediaController;
import android.media.session.MediaSessionManager;
import android.test.InstrumentationTestCase;
import android.test.UiThreadTest;

import java.util.List;

public class MediaSessionManagerTest extends InstrumentationTestCase {
    private MediaSessionManager mSessionManager;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mSessionManager = (MediaSessionManager) getInstrumentation().getTargetContext()
                .getSystemService(Context.MEDIA_SESSION_SERVICE);
    }

    public void testGetActiveSessions() throws Exception {
        try {
            List<MediaController> controllers = mSessionManager.getActiveSessions(null);
            fail("Expected security exception for unauthorized call to getActiveSessions");
        } catch (SecurityException e) {
            // Expected
        }
        // TODO enable a notification listener, test again, disable, test again
    }

    @UiThreadTest
    public void testAddOnActiveSessionsListener() throws Exception {
        try {
            mSessionManager.addOnActiveSessionsChangedListener(null, null);
            fail("Expected IAE for call to addOnActiveSessionsChangedListener");
        } catch (IllegalArgumentException e) {
            // Expected
        }

        MediaSessionManager.OnActiveSessionsChangedListener listener
                = new MediaSessionManager.OnActiveSessionsChangedListener() {
            @Override
            public void onActiveSessionsChanged(List<MediaController> controllers) {

            }
        };
        try {
            mSessionManager.addOnActiveSessionsChangedListener(listener, null);
            fail("Expected security exception for call to addOnActiveSessionsChangedListener");
        } catch (SecurityException e) {
            // Expected
        }

        // TODO enable a notification listener, test again, disable, verify
        // updates stopped
    }
}
