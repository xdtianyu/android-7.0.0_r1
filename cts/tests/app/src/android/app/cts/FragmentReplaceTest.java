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
package android.app.cts;

import android.app.stubs.FragmentTestActivity;
import android.app.stubs.FragmentTestActivity.TestFragment;
import android.app.stubs.R;
import android.test.ActivityInstrumentationTestCase2;
import android.test.UiThreadTest;

/**
 * Test to prevent regressions in FragmentManager fragment replace method. See b/24693644
 */
public class FragmentReplaceTest extends
        ActivityInstrumentationTestCase2<FragmentTestActivity> {
    private FragmentTestActivity mActivity;


    public FragmentReplaceTest() {
        super(FragmentTestActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
    }

    @UiThreadTest
    public void testReplaceFragment() throws Throwable {
        mActivity.getFragmentManager().beginTransaction()
                .add(R.id.content, new TestFragment(R.layout.fragment_a))
                .addToBackStack(null)
                .commit();
        mActivity.getFragmentManager().executePendingTransactions();
        assertNotNull(mActivity.findViewById(R.id.textA));
        assertNull(mActivity.findViewById(R.id.textB));
        assertNull(mActivity.findViewById(R.id.textC));

        mActivity.getFragmentManager().beginTransaction()
                .add(R.id.content, new TestFragment(R.layout.fragment_b))
                .addToBackStack(null)
                .commit();
        mActivity.getFragmentManager().executePendingTransactions();
        assertNotNull(mActivity.findViewById(R.id.textA));
        assertNotNull(mActivity.findViewById(R.id.textB));
        assertNull(mActivity.findViewById(R.id.textC));

        mActivity.getFragmentManager().beginTransaction()
                .replace(R.id.content, new TestFragment(R.layout.fragment_c))
                .addToBackStack(null)
                .commit();
        mActivity.getFragmentManager().executePendingTransactions();
        assertNull(mActivity.findViewById(R.id.textA));
        assertNull(mActivity.findViewById(R.id.textB));
        assertNotNull(mActivity.findViewById(R.id.textC));
    }
}
