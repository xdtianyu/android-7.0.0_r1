/**
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.accessibilityservice.cts;

import android.app.UiAutomation;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.View;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.EditText;
import android.widget.TextView;

import android.accessibilityservice.cts.R;

/**
 * Test cases for actions taken on text views.
 */
public class AccessibilityTextActionTest extends
        AccessibilityActivityTestCase<AccessibilityTextTraversalActivity> {
    UiAutomation mUiAutomation;

    public AccessibilityTextActionTest() {
        super(AccessibilityTextTraversalActivity.class);
    }

    public void setUp() throws Exception {
        super.setUp();
        mUiAutomation = getInstrumentation().getUiAutomation();
    }

    public void tearDown() throws Exception {
        mUiAutomation.destroy();
        super.tearDown();
    }

    public void testNotEditableTextView_shouldNotExposeOrRespondToSetTextAction() {
        final TextView textView = (TextView) getActivity().findViewById(R.id.text);

        getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                textView.setVisibility(View.VISIBLE);
                textView.setText(getString(R.string.a_b));
            }
        });

        final AccessibilityNodeInfo text = mUiAutomation.getRootInActiveWindow()
                .findAccessibilityNodeInfosByText(getString(R.string.a_b)).get(0);

        assertFalse("Standard text view should not support SET_TEXT", text.getActionList()
                .contains(AccessibilityNodeInfo.AccessibilityAction.ACTION_SET_TEXT));
        assertEquals("Standard text view should not support SET_TEXT", 0,
                text.getActions() & AccessibilityNodeInfo.ACTION_SET_TEXT);
        Bundle args = new Bundle();
        args.putCharSequence(AccessibilityNodeInfo.ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE,
                getString(R.string.text_input_blah));
        assertFalse(text.performAction(AccessibilityNodeInfo.ACTION_SET_TEXT, args));

        getInstrumentation().waitForIdleSync();
        assertTrue("Text view should not update on failed set text",
                TextUtils.equals(getString(R.string.a_b), textView.getText()));
    }

    public void testEditableTextView_shouldExposeAndRespondToSetTextAction() {
        final TextView textView = (TextView) getActivity().findViewById(R.id.text);

        getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                textView.setVisibility(View.VISIBLE);
                textView.setText(getString(R.string.a_b), TextView.BufferType.EDITABLE);
            }
        });

        final AccessibilityNodeInfo text = mUiAutomation.getRootInActiveWindow()
                .findAccessibilityNodeInfosByText(getString(R.string.a_b)).get(0);

        assertTrue("Editable text view should support SET_TEXT", text.getActionList()
                .contains(AccessibilityNodeInfo.AccessibilityAction.ACTION_SET_TEXT));
        assertEquals("Editable text view should support SET_TEXT",
                AccessibilityNodeInfo.ACTION_SET_TEXT,
                text.getActions() & AccessibilityNodeInfo.ACTION_SET_TEXT);

        Bundle args = new Bundle();
        String textToSet = getString(R.string.text_input_blah);
        args.putCharSequence(
                AccessibilityNodeInfo.ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE, textToSet);

        assertTrue(text.performAction(AccessibilityNodeInfo.ACTION_SET_TEXT, args));

        getInstrumentation().waitForIdleSync();
        assertTrue("Editable text should update on set text",
                TextUtils.equals(textToSet, textView.getText()));
    }

    public void testEditText_shouldExposeAndRespondToSetTextAction() {
        final EditText editText = (EditText) getActivity().findViewById(R.id.edit);

        getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                editText.setVisibility(View.VISIBLE);
                editText.setText(getString(R.string.a_b));
            }
        });

        final AccessibilityNodeInfo text = mUiAutomation.getRootInActiveWindow()
                .findAccessibilityNodeInfosByText(getString(R.string.a_b)).get(0);

        assertTrue("EditText should support SET_TEXT", text.getActionList()
                .contains(AccessibilityNodeInfo.AccessibilityAction.ACTION_SET_TEXT));
        assertEquals("EditText view should support SET_TEXT",
                AccessibilityNodeInfo.ACTION_SET_TEXT,
                text.getActions() & AccessibilityNodeInfo.ACTION_SET_TEXT);

        Bundle args = new Bundle();
        String textToSet = getString(R.string.text_input_blah);
        args.putCharSequence(
                AccessibilityNodeInfo.ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE, textToSet);

        assertTrue(text.performAction(AccessibilityNodeInfo.ACTION_SET_TEXT, args));

        getInstrumentation().waitForIdleSync();
        assertTrue("EditText should update on set text",
                TextUtils.equals(textToSet, editText.getText()));
    }
}
