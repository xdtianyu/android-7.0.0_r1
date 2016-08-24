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

package android.text.style.cts;

import android.app.PendingIntent;
import android.content.Intent;
import android.os.Parcel;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.text.style.EasyEditSpan;

public class EasyEditSpanTest extends AndroidTestCase {
    @SmallTest
    public void testConstructor() {
        new EasyEditSpan();
        new EasyEditSpan(PendingIntent.getActivity(getContext(), 0, new Intent(), 0));
        Parcel p = Parcel.obtain();
        try {
            new EasyEditSpan(p);
        } finally {
            p.recycle();
        }
    }

    @SmallTest
    public void testDescribeContents_doesNotThrowException() {
        EasyEditSpan easyEditSpan = new EasyEditSpan();
        easyEditSpan.describeContents();
    }

    @SmallTest
    public void testGetSpanTypeId_doesNotThrowException() {
        EasyEditSpan easyEditSpan = new EasyEditSpan();
        easyEditSpan.getSpanTypeId();
    }

    @SmallTest
    public void testWriteToParcel() {
        Parcel p = Parcel.obtain();
        try {
            EasyEditSpan easyEditSpan = new EasyEditSpan();
            easyEditSpan.writeToParcel(p, 0);
            p.setDataPosition(0);
            new EasyEditSpan(p);
        } finally {
            p.recycle();
        }
    }
}
