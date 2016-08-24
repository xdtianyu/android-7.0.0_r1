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

package com.android.managedprovisioning;

import android.accounts.Account;
import android.content.ComponentName;
import android.content.Intent;
import android.os.BaseBundle;
import android.os.Bundle;
import android.os.PersistableBundle;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import java.util.Set;

public class IntentStoreTest extends AndroidTestCase {

    private static final String SAMPLE_INTENT_STORE_NAME = "sample_intent_store_name";

    private static final ComponentName SAMPLE_COMPONENT_NAME
            = new ComponentName("package.sample", "class.sample");

    @Override
    public void tearDown() {
        createIntentStore().clear();
    }

    @SmallTest
    public void testAction() throws Exception {
        testIntentCanBeRecovered(new Intent("action.test"));
    }

    @SmallTest
    public void testComponent() throws Exception {
        testIntentCanBeRecovered(new Intent().setComponent(SAMPLE_COMPONENT_NAME));
    }

    @SmallTest
    public void testExtraStrings() throws Exception {
        testIntentCanBeRecovered(new Intent().putExtra("a", "b").putExtra("c", "d"));
    }

    @SmallTest
    public void testExtraInt() throws Exception {
        testIntentCanBeRecovered(new Intent().putExtra("a", 42));
    }

    @SmallTest
    public void testExtraLong() throws Exception {
        testIntentCanBeRecovered(new Intent().putExtra("a", 12345678910L));
    }

    @SmallTest
    public void testExtraBoolean() throws Exception {
        testIntentCanBeRecovered(new Intent().putExtra("a", true));
    }

    @SmallTest
    public void testExtraComponentName() throws Exception {
        testIntentCanBeRecovered(new Intent().putExtra("a", SAMPLE_COMPONENT_NAME));
    }

    @SmallTest
    public void testExtraAccount() throws Exception {
        testIntentCanBeRecovered(new Intent().putExtra("a",
                new Account("sample.type", "sample.name")));
    }

    @SmallTest
    public void testPersistableBundle() throws Exception {
        PersistableBundle pBundle = new PersistableBundle();
        pBundle.putString("a", "b");
        pBundle.putBoolean("c", false);
        Intent firstIntent = new Intent().putExtra("a", pBundle);
        // Utils.intentEquals() method doesn't support embedded Bundle, so we can't use it.
        IntentStore intentStore = createIntentStore();
        intentStore.save(firstIntent);
        Intent newIntent = intentStore.load();
        if (!TestUtils.bundleEquals(pBundle, (PersistableBundle) newIntent.getExtra("a"))) {
            TestUtils.failIntentsNotEqual(firstIntent, newIntent);
        }
    }

    /**
     * Tests that the intent can be saved, recovered,
     * and that the recovered intent is equal to the original one.
     */
    private void testIntentCanBeRecovered(Intent intent) throws Exception {
        createIntentStore().save(intent);
        // Use a different intent store to make sure that the intent store doesn't just save the
        // intent in memmory.
        IntentStore loadingIntentStore = createIntentStore();
        TestUtils.assertIntentEquals(intent, loadingIntentStore.load());
    }

    private IntentStore createIntentStore() {
        return new IntentStore(getContext(), SAMPLE_INTENT_STORE_NAME);
    }
}
