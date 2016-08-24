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

package android.widget.cts;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.DatePickerDialog;
import android.content.Context;
import android.test.ActivityInstrumentationTestCase2;
import android.test.UiThreadTest;

/**
 * Test {@link DatePickerDialog}.
 */
public class DatePickerDialogTest extends
        ActivityInstrumentationTestCase2<DatePickerDialogCtsActivity> {

    private Activity mActivity;

    public DatePickerDialogTest() {
        super(DatePickerDialogCtsActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
    }

    @UiThreadTest
    @SuppressWarnings("deprecation")
    public void testConstructor() {
        new DatePickerDialog(mActivity, null, 1970, 1, 1);

        new DatePickerDialog(mActivity, AlertDialog.THEME_TRADITIONAL, null, 1970, 1, 1);

        new DatePickerDialog(mActivity, AlertDialog.THEME_HOLO_DARK, null, 1970, 1, 1);

        new DatePickerDialog(mActivity,
                android.R.style.Theme_Material_Dialog_Alert, null, 1970, 1, 1);

        try {
            new DatePickerDialog(null, null, 1970, 1, 1);
            fail("should throw NullPointerException");
        } catch (Exception e) {
        }
    }

    @UiThreadTest
    public void testShowDismiss() {
        DatePickerDialog d = createDatePickerDialog();

        d.show();
        assertTrue("Showing date picker", d.isShowing());

        d.show();
        assertTrue("Date picker still showing", d.isShowing());

        d.dismiss();
        assertFalse("Dismissed date picker", d.isShowing());

        d.dismiss();
        assertFalse("Date picker still dismissed", d.isShowing());
    }

    private MockDatePickerDialog createDatePickerDialog() {
        return new MockDatePickerDialog(mActivity, null, 1970, 1, 1);
    }

    private class MockDatePickerDialog extends DatePickerDialog {
        public MockDatePickerDialog(Context context, OnDateSetListener callBack,
                int year, int monthOfYear, int dayOfMonth) {
            super(context, callBack, year, monthOfYear, dayOfMonth);
        }
    }
}
