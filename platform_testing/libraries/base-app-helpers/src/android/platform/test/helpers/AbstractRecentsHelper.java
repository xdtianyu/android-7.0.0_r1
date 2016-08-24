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

package android.platform.test.helpers;

import android.app.Instrumentation;
import android.support.test.uiautomator.Direction;

public abstract class AbstractRecentsHelper extends AbstractStandardAppHelper {

    public AbstractRecentsHelper(Instrumentation instr) {
        super(instr);
    }

    /**
     * Setup expectations: "Recents" is open.
     *
     * Flings the recent apps in the specified direction.
     * @param dir the direction for the apps to move
     */
    public abstract void flingRecents(Direction dir);
}
