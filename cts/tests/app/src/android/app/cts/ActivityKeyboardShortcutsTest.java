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

package android.app.cts;

import android.app.Activity;
import android.app.stubs.KeyboardShortcutsActivity;
import android.test.ActivityInstrumentationTestCase2;
import android.view.KeyEvent;
import android.view.KeyboardShortcutGroup;
import android.view.Menu;
import android.widget.PopupMenu;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests functionality in Activity related to Keyboard Shortcuts.
 */
public class ActivityKeyboardShortcutsTest
        extends ActivityInstrumentationTestCase2<KeyboardShortcutsActivity> {

    private Activity mActivity;
    private Menu mMenu;

    public ActivityKeyboardShortcutsTest() {
        super(KeyboardShortcutsActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
        mMenu = new PopupMenu(mActivity, null).getMenu();
    }

    public void testOnProvideKeyboardShortcuts() {
        List<KeyboardShortcutGroup> data = new ArrayList<>();
        mActivity.onCreateOptionsMenu(mMenu);
        mActivity.onProvideKeyboardShortcuts(data, mMenu, -1);

        assertEquals(1, data.size());
        assertEquals(1, data.get(0).getItems().size());
        assertEquals(KeyboardShortcutsActivity.ITEM_1_NAME,
            data.get(0).getItems().get(0).getLabel());
        assertEquals(KeyboardShortcutsActivity.ITEM_1_SHORTCUT,
            data.get(0).getItems().get(0).getBaseCharacter());
        assertEquals(KeyEvent.META_CTRL_ON, data.get(0).getItems().get(0).getModifiers());
    }
}
