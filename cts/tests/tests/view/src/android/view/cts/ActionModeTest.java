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

package android.view.cts;

import android.graphics.Rect;
import android.test.ActivityInstrumentationTestCase2;
import android.test.UiThreadTest;
import android.view.ActionMode;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;

public class ActionModeTest extends ActivityInstrumentationTestCase2<ActionModeCtsActivity> {

    public ActionModeTest() {
        super(ActionModeCtsActivity.class);
    }

    public void testSetType() {
        ActionMode actionMode = new MockActionMode();
        assertEquals(ActionMode.TYPE_PRIMARY, actionMode.getType());

        actionMode.setType(ActionMode.TYPE_FLOATING);
        assertEquals(ActionMode.TYPE_FLOATING, actionMode.getType());

        actionMode.setType(ActionMode.TYPE_PRIMARY);
        assertEquals(ActionMode.TYPE_PRIMARY, actionMode.getType());
    }

    public void testInvalidateContentRectDoesNotInvalidateFull() {
        MockActionMode actionMode = new MockActionMode();

        actionMode.invalidateContentRect();

        assertFalse(actionMode.mInvalidateWasCalled);
    }

    public void testInvalidateContentRectOnFloatingCallsCallback() {
        final View view = getActivity().contentView;
        final MockActionModeCallback2 callback = new MockActionModeCallback2();

        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                ActionMode mode = view.startActionMode(callback, ActionMode.TYPE_FLOATING);
                assertNotNull(mode);
                mode.invalidateContentRect();
            }
        });
        getInstrumentation().waitForIdleSync();

        assertTrue(callback.mIsOnGetContentRectCalled);
    }

    public void testSetAndGetTitleOptionalHint() {
        MockActionMode actionMode = new MockActionMode();

        // Check default value.
        assertFalse(actionMode.getTitleOptionalHint());
        // Test set and get.
        actionMode.setTitleOptionalHint(true);
        assertTrue(actionMode.getTitleOptionalHint());
        actionMode.setTitleOptionalHint(false);
        assertFalse(actionMode.getTitleOptionalHint());
    }

    public void testSetAndGetTag() {
        MockActionMode actionMode = new MockActionMode();
        Object tag = new Object();

        // Check default value.
        assertNull(actionMode.getTag());

        actionMode.setTag(tag);
        assertSame(tag, actionMode.getTag());
    }

    public void testIsTitleOptional() {
        MockActionMode actionMode = new MockActionMode();

        // Check default value.
        assertFalse(actionMode.isTitleOptional());
    }

    public void testIsUiFocusable() {
        MockActionMode actionMode = new MockActionMode();

        // Check default value.
        assertTrue(actionMode.isUiFocusable());
    }

    public void testHide() {
        MockActionMode actionMode = new MockActionMode();

        actionMode.hide(0);
        actionMode.hide(ActionMode.DEFAULT_HIDE_DURATION);
    }

    public void testOnWindowFocusChanged() {
        MockActionMode actionMode = new MockActionMode();

        actionMode.onWindowFocusChanged(true);
        actionMode.onWindowFocusChanged(false);
    }

    private static class MockActionModeCallback2 extends ActionMode.Callback2 {
        boolean mIsOnGetContentRectCalled = false;

        @Override
        public boolean onCreateActionMode(ActionMode mode, Menu menu) {
            return true;
        }

        @Override
        public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
            return true;
        }

        @Override
        public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
            return false;
        }

        @Override
        public void onDestroyActionMode(ActionMode mode) {}

        @Override
        public void onGetContentRect(ActionMode mode, View view, Rect outRect) {
            mIsOnGetContentRectCalled = true;
            super.onGetContentRect(mode, view, outRect);
        }
    }

    private static class MockActionMode extends ActionMode {
        boolean mInvalidateWasCalled = false;

        @Override
        public void setTitle(CharSequence title) {}

        @Override
        public void setTitle(int resId) {}

        @Override
        public void setSubtitle(CharSequence subtitle) {}

        @Override
        public void setSubtitle(int resId) {}

        @Override
        public void setCustomView(View view) {}

        @Override
        public void invalidate() {
            mInvalidateWasCalled = true;
        }

        @Override
        public void finish() {}

        @Override
        public Menu getMenu() {
            return null;
        }

        @Override
        public CharSequence getTitle() {
            return null;
        }

        @Override
        public CharSequence getSubtitle() {
            return null;
        }

        @Override
        public View getCustomView() {
            return null;
        }

        @Override
        public MenuInflater getMenuInflater() {
            return null;
        }
    }
}
