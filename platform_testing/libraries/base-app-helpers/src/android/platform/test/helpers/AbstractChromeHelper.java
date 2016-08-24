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

public abstract class AbstractChromeHelper extends AbstractStandardAppHelper {

    public AbstractChromeHelper(Instrumentation instr) {
        super(instr);
    }

    /**
     * Setup expectations: Chrome is open and on a standard page, i.e. a tab is open.
     *
     * This method will open the URL supplied and block until the page is open.
     */
    public abstract void openUrl(String url);

    /**
     * Setup expectations: Chrome is open on a page.
     *
     * This method will scroll the page as directed and block until idle.
     */
    public abstract void flingPage(Direction dir);

    /**
     * Setup expectations: Chrome is open on a page.
     *
     * This method will open the overload menu, indicated by three dots and block until open.
     */
    public abstract void openMenu();

    /**
     * Setup expectations: Chrome is open on a page and the tabs are treated as apps.
     *
     * This method will change the settings to treat tabs inside of Chrome and block until Chrome is
     * open on the original tab.
     */
    public abstract void mergeTabs();

    /**
     * Setup expectations: Chrome is open on a page and the tabs are merged.
     *
     * This method will change the settings to treat tabs outside of Chrome and block until Chrome
     * is open on the original tab.
     */
    public abstract void unmergeTabs();
}
