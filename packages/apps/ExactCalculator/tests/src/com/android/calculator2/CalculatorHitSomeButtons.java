/**
 * Copyright (c) 2008, Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.calculator2;

import android.app.Activity;
import android.app.Instrumentation;
import android.app.Instrumentation.ActivityMonitor;
import android.content.Intent;
import android.content.IntentFilter;
import android.test.ActivityInstrumentationTestCase;
import android.test.suitebuilder.annotation.LargeTest;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.graphics.Rect;
import android.test.TouchUtils;

import com.android.calculator2.Calculator;
import com.android.calculator2.R;
import com.android.calculator2.CalculatorResult;

/**
 * Instrumentation tests for poking some buttons
 *
 */

public class CalculatorHitSomeButtons extends ActivityInstrumentationTestCase <Calculator>{
    public boolean setup = false;
    private static final String TAG = "CalculatorTests";
    Calculator mActivity = null;
    Instrumentation mInst = null;

    public CalculatorHitSomeButtons() {
        super("com.android.calculator2", Calculator.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mActivity = getActivity();
        mInst = getInstrumentation();
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
    }


    @LargeTest
    public void testPressSomeKeys() {
        Log.v(TAG, "Pressing some keys!");

        // Make sure that we clear the output
        press(KeyEvent.KEYCODE_ENTER);
        press(KeyEvent.KEYCODE_CLEAR);

        // 3 + 4 * 5 => 23
        press(KeyEvent.KEYCODE_3);
        press(KeyEvent.KEYCODE_PLUS);
        press(KeyEvent.KEYCODE_4);
        press(KeyEvent.KEYCODE_9 | KeyEvent.META_SHIFT_ON);
        press(KeyEvent.KEYCODE_5);
        press(KeyEvent.KEYCODE_ENTER);

        checkDisplay("23");
    }


    @LargeTest
    public void testTapSomeButtons() {
        // TODO: This probably makes way too many hardcoded assumptions about locale.
        // The calculator will need a routine to internationalize the output.
        // We should use that here, too.
        Log.v(TAG, "Tapping some buttons!");

        // Make sure that we clear the output
        tap(R.id.eq);
        tap(R.id.del);

        // 567 / 3 => 189
        tap(R.id.digit_5);
        tap(R.id.digit_6);
        tap(R.id.digit_7);
        tap(R.id.op_div);
        tap(R.id.digit_3);
        tap(R.id.dec_point);
        tap(R.id.eq);

        checkDisplay("189");

        // make sure we can continue calculations also
        // 189 - 789 => -600
        tap(R.id.op_sub);
        tap(R.id.digit_7);
        tap(R.id.digit_8);
        tap(R.id.digit_9);
        tap(R.id.eq);

        // Careful: the first digit in the expected value is \u2212, not "-" (a hyphen)
        checkDisplay(mActivity.getString(R.string.op_sub) + "600");

        tap(R.id.dec_point);
        tap(R.id.digit_5);
        tap(R.id.op_add);
        tap(R.id.dec_point);
        tap(R.id.digit_5);
        tap(R.id.eq);
        checkDisplay("1");

        tap(R.id.digit_5);
        tap(R.id.op_div);
        tap(R.id.digit_3);
        tap(R.id.dec_point);
        tap(R.id.digit_5);
        tap(R.id.op_mul);
        tap(R.id.digit_7);
        tap(R.id.eq);
        checkDisplay("10");
    }

    // helper functions
    private void press(int keycode) {
        mInst.sendKeyDownUpSync(keycode);
    }

    private void tap(int id) {
        View view = mActivity.findViewById(id);
        assertNotNull(view);
        TouchUtils.clickView(this, view);
    }

    private void checkDisplay(final String s) {
    /*
        FIXME: This doesn't yet work.
        try {
            Thread.sleep(20);
            runTestOnUiThread(new Runnable () {
                @Override
                public void run() {
                    Log.v(TAG, "Display:" + displayVal());
                    assertEquals(s, displayVal());
                }
            });
        } catch (Throwable e) {
            fail("unexpected exception" + e);
        }
    */
    }

    private String displayVal() {
        CalculatorResult display = (CalculatorResult) mActivity.findViewById(R.id.result);
        assertNotNull(display);

        TextView box = (TextView) display;
        assertNotNull(box);

        return box.getText().toString();
    }
}

