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

package android.app.stubs;

import android.app.Activity;
import android.view.Menu;

public class KeyboardShortcutsActivity extends Activity {

    public static final String ITEM_1_NAME = "item 1";
    public static final char ITEM_1_SHORTCUT = 'i';

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        menu.add(ITEM_1_NAME).setAlphabeticShortcut(ITEM_1_SHORTCUT);
        return true;
    }
}
