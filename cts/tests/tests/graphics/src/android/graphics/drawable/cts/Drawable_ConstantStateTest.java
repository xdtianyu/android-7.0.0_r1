/*
 * Copyright (C) 2008 The Android Open Source Project
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

package android.graphics.drawable.cts;

import android.graphics.drawable.Drawable;
import android.graphics.drawable.Drawable.ConstantState;
import android.test.AndroidTestCase;

public class Drawable_ConstantStateTest extends AndroidTestCase {

    public void testNewDrawable() {
        MockConstantState mock = new MockConstantState();
        ConstantState cs = mock;

        assertEquals(null, cs.newDrawable());
        assertTrue(mock.hasCalledNewDrawable());
        mock.reset();

        assertEquals(null, cs.newDrawable(mContext.getResources()));
        assertTrue(mock.hasCalledNewDrawable());
        mock.reset();

        assertEquals(null, cs.newDrawable(mContext.getResources(), mContext.getTheme()));
        assertTrue(mock.hasCalledNewDrawable());
    }

    public void testCanApplyTheme() {
        ConstantState cs = new MockConstantState();
        assertFalse(cs.canApplyTheme());
    }

    public static class MockConstantState extends ConstantState {
        private boolean mCalledNewDrawable;

        @Override
        public Drawable newDrawable() {
            mCalledNewDrawable = true;
            return null;
        }

        @Override
        public int getChangingConfigurations() {
            return 0;
        }

        public boolean hasCalledNewDrawable() {
            return mCalledNewDrawable;
        }

        public void reset() {
            mCalledNewDrawable = false;
        }
    }
}
