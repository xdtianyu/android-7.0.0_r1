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

package android.view.cts;

import android.view.cts.R;

import android.app.Instrumentation;
import android.test.ActivityInstrumentationTestCase2;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.SearchEvent;

public class SearchEventTest extends ActivityInstrumentationTestCase2<SearchEventActivity> {

    private Instrumentation mInstrumentation;
    private SearchEventActivity mActivity;

    public SearchEventTest() {
        super(SearchEventActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mInstrumentation = getInstrumentation();
        mActivity = getActivity();
    }

    public void testTest() throws Exception {
        mInstrumentation.sendKeyDownUpSync(KeyEvent.KEYCODE_SEARCH);
        SearchEvent se = mActivity.getTestSearchEvent();
        assertNotNull(se);
        InputDevice id = se.getInputDevice();
        assertNotNull(id);
        assertEquals(-1, id.getId());
    }
}
