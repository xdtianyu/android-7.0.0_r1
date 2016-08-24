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
 * limitations under the License
 */

package com.android.phone.vvm.omtp;

import android.content.Context;
import android.content.Intent;
import android.preference.PreferenceManager;
import android.test.AndroidTestCase;
import android.util.ArraySet;

import com.android.phone.vvm.omtp.OmtpBootCompletedReceiver.SubIdProcessor;

import java.util.Set;

public class OmtpBootCompletedReceiverTests extends AndroidTestCase {
    OmtpBootCompletedReceiver mReceiver = new OmtpBootCompletedReceiver();
    @Override
    public void setUp() {
    }

    @Override
    public void tearDown() {
        PreferenceManager
                .getDefaultSharedPreferences(getContext().createDeviceProtectedStorageContext())
                .edit().clear().apply();
    }

    public void testReadWriteList() {
        readWriteList(new int[] {1});
    }

    public void testReadWriteList_Multiple() {
        readWriteList(new int[] {1, 2});
    }

    public void testReadWriteList_Duplicate() {
        readWriteList(new int[] {1, 1});
    }

    private void readWriteList(int[] values) {
        for (int value : values) {
            OmtpBootCompletedReceiver.addDeferredSubId(getContext(), value);
        }
        TestSubIdProcessor processor = new TestSubIdProcessor(values);
        mReceiver.setSubIdProcessorForTest(processor);
        Intent intent = new Intent(Intent.ACTION_BOOT_COMPLETED);
        mReceiver.onReceive(getContext(), intent);
        processor.assertMatch();
        // after onReceive() is called the list should be empty
        TestSubIdProcessor emptyProcessor = new TestSubIdProcessor(new int[] {});
        mReceiver.setSubIdProcessorForTest(processor);
        mReceiver.onReceive(getContext(), intent);
        processor.assertMatch();
    }

    private static class TestSubIdProcessor implements SubIdProcessor {
        private final Set<Integer> mExpectedSubIds;

        public TestSubIdProcessor(int[] expectedSubIds) {
            mExpectedSubIds = new ArraySet<>();
            for(int subId : expectedSubIds){
                mExpectedSubIds.add(subId);
            }
        }

        @Override
        public void process(Context context, int subId){
            assertTrue(mExpectedSubIds.contains(subId));
            mExpectedSubIds.remove(subId);
        }

        public void assertMatch(){
            assertTrue(mExpectedSubIds.isEmpty());
        }
    }
}
