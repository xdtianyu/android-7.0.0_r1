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

package android.text.method.cts;

import android.test.suitebuilder.annotation.SmallTest;
import android.text.InputType;
import android.text.method.BaseKeyListener;
import android.view.KeyEvent;
import android.widget.TextView.BufferType;

/**
 * Test backspace key handling of {@link android.text.method.BaseKeyListener}.
 */
public class BackspaceTest extends KeyListenerTestCase {
    private static final BaseKeyListener mKeyListener = new BaseKeyListener() {
        public int getInputType() {
            return InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL;
        }
    };

    // Sync the state to the TextView and call onKeyDown with KEYCODE_DEL key event.
    // Then update the state to the result of TextView.
    private void backspace(final EditorState state, int modifiers) {
        mActivity.runOnUiThread(new Runnable() {
            public void run() {
                mTextView.setText(state.mText, BufferType.EDITABLE);
                mTextView.setKeyListener(mKeyListener);
                mTextView.setSelection(state.mSelectionStart, state.mSelectionEnd);
            }
        });
        mInstrumentation.waitForIdleSync();
        assertTrue(mTextView.hasWindowFocus());

        final KeyEvent keyEvent = getKey(KeyEvent.KEYCODE_DEL, modifiers);
        mActivity.runOnUiThread(new Runnable() {
            public void run() {
                mTextView.onKeyDown(keyEvent.getKeyCode(), keyEvent);
            }
        });
        mInstrumentation.waitForIdleSync();

        state.mText = mTextView.getText();
        state.mSelectionStart = mTextView.getSelectionStart();
        state.mSelectionEnd = mTextView.getSelectionEnd();
    }

    @SmallTest
    public void testSurrogatePairs() {
        EditorState state = new EditorState();

        state.setByString("U+1F441 |");
        backspace(state, 0);
        state.assertEquals("|");

        state.setByString("U+1F441 U+1F5E8 |");
        backspace(state, 0);
        state.assertEquals("U+1F441 |");
        backspace(state, 0);
        state.assertEquals("|");
    }

    @SmallTest
    public void testReplacementSpan() {
        EditorState state = new EditorState();

        // ReplacementSpan will be set to "()" region.
        state.setByString("'abc' ( 'de' ) 'fg' |");
        backspace(state, 0);
        state.assertEquals("'abc' ( 'de' ) 'f' |");
        backspace(state, 0);
        state.assertEquals("'abc' ( 'de' ) |");
        backspace(state, 0);
        state.assertEquals("'abc' |");
        backspace(state, 0);
        state.assertEquals("'ab' |");
        backspace(state, 0);
        state.assertEquals("'a' |");
        backspace(state, 0);
        state.assertEquals("|");

        state.setByString("'abc' [ ( 'de' ) ] 'fg'");
        backspace(state, 0);
        state.assertEquals("'abc' | 'fg'");
        backspace(state, 0);
        state.assertEquals("'ab' | 'fg'");
        backspace(state, 0);
        state.assertEquals("'a' | 'fg'");
        backspace(state, 0);
        state.assertEquals("| 'fg'");
        backspace(state, 0);
        state.assertEquals("| 'fg'");

        state.setByString("'ab' [ 'c' ( 'de' ) 'f' ] 'g'");
        backspace(state, 0);
        state.assertEquals("'ab' | 'g'");
        backspace(state, 0);
        state.assertEquals("'a' | 'g'");
        backspace(state, 0);
        state.assertEquals("| 'g'");
        backspace(state, 0);
        state.assertEquals("| 'g'");
    }

    @SmallTest
    public void testCombiningEnclosingKeycaps() {
        EditorState state = new EditorState();

        // U+20E3 is COMBINING ENCLOSING KEYCAP.
        state.setByString("'1' U+20E3 |");
        backspace(state, 0);
        state.assertEquals("|");

        // Variation selector before COMBINING ECLOSING KEYCAP
        state.setByString("'1' U+FE0E U+20E3 |");
        backspace(state, 0);
        state.assertEquals("|");
    }

    @SmallTest
    public void testVariationSelector() {
        EditorState state = new EditorState();

        // U+FE0F is VARIATION SELECTOR-16.
        state.setByString("'#' U+FE0F |");
        backspace(state, 0);
        state.assertEquals("|");

        // U+E0100 is VARIATION SELECTOR-17.
        state.setByString("U+845B U+E0100 |");
        backspace(state, 0);
        state.assertEquals("|");
    }

    @SmallTest
    public void testFlags() {
        EditorState state = new EditorState();

        // U+1F1FA is REGIONAL INDICATOR SYMBOL LETTER U.
        // U+1F1F8 is REGIONAL INDICATOR SYMBOL LETTER S.
        state.setByString("U+1F1FA U+1F1F8 |");
        backspace(state, 0);
        state.assertEquals("|");

        state.setByString("'a' U+1F1FA U+1F1F8 |");
        backspace(state, 0);
        state.assertEquals("'a' |");
        backspace(state, 0);
        state.assertEquals("|");

        state.setByString("U+1F1FA U+1F1F8 U+1F1FA U+1F1F8 |");
        backspace(state, 0);
        state.assertEquals("U+1F1FA U+1F1F8 |");
        backspace(state, 0);
        state.assertEquals("|");

        state.setByString("'a' U+1F1FA U+1F1F8 'b' U+1F1FA U+1F1F8 |");
        backspace(state, 0);
        state.assertEquals("'a' U+1F1FA U+1F1F8 'b' |");
        backspace(state, 0);
        state.assertEquals("'a' U+1F1FA U+1F1F8 |");
        backspace(state, 0);
        state.assertEquals("'a' |");
        backspace(state, 0);
        state.assertEquals("|");
    }
}
