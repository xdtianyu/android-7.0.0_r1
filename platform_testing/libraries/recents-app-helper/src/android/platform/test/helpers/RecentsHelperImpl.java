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

package android.platform.test.helpers;

import android.app.Instrumentation;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.util.Log;
import android.widget.EditText;

import junit.framework.Assert;

public class RecentsHelperImpl extends AbstractRecentsHelper {
    private static final String LOG_TAG = RecentsHelperImpl.class.getSimpleName();
    private static final String UI_PACKAGE = "com.android.systemui";

    private static final long RECENTS_SELECTION_TIMEOUT = 5000;

    public RecentsHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getPackage() {
        throw new UnsupportedOperationException("This method is not supported for Recents");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getLauncherName() {
        throw new UnsupportedOperationException("This method is not supported for Recents");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void open() {
        try {
            mDevice.pressRecentApps();
            mDevice.waitForIdle();
        } catch (RemoteException ex) {
            Log.e(LOG_TAG, ex.toString());
        }
    }


    /**
     * {@inheritDoc}
     */
    @Override
    public void exit() {
        mDevice.pressHome();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dismissInitialDialogs() {
        // Nothing to do.
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void flingRecents(Direction dir) {
        UiObject2 recentsScroller = getRecentsScroller();
        Assert.assertNotNull("Unable to find scrolling mechanism for Recents", recentsScroller);
        recentsScroller.setGestureMargin(recentsScroller.getVisibleBounds().height() / 4);
        recentsScroller.fling(dir);
    }

    private UiObject2 getRecentsScroller() {
        return mDevice.wait(Until.findObject(By.res(UI_PACKAGE, "recents_view")),
                RECENTS_SELECTION_TIMEOUT);
    }
}
