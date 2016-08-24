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
package com.android.managedprovisioning.uiflows;

import android.content.Intent;
import android.test.ActivityUnitTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.view.ViewGroup;
import android.webkit.WebView;

@SmallTest
public class WebActivityTest extends ActivityUnitTestCase<WebActivity> {
    private static final String TEST_URL = "http://www.test.com/support";

    public WebActivityTest() {
        super(WebActivity.class);
    }

    public void testNoUrl() {
        startActivity(WebActivity.createIntent(getInstrumentation().getTargetContext(),
                null, null), null, null);
        assertTrue(isFinishCalled());
    }

    public void testUrlLaunched() {
        startActivity(WebActivity.createIntent(getInstrumentation().getTargetContext(),
                TEST_URL, null), null, null);
        assertFalse(isFinishCalled());
        WebView webView = (WebView) ((ViewGroup) getActivity().findViewById(android.R.id.content))
                .getChildAt(0);
        assertEquals(TEST_URL, webView.getUrl());
    }
}
