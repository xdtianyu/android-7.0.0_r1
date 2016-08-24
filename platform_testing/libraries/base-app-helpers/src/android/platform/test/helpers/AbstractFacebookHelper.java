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

public abstract class AbstractFacebookHelper extends AbstractStandardAppHelper {

    public AbstractFacebookHelper(Instrumentation instr) {
        super(instr);
    }

    /**
     * Setup expectations: Facebook is on the home page.
     *
     * This method scrolls the home page.
     *
     * @param dir the direction to scroll
     */
    public abstract void scrollHomePage(Direction dir);

    /**
     * Setup expectations: Facebook app is open.
     *
     * This method keeps pressing the back button until Facebook is on the homepage.
     */
    public abstract void goToHomePage();

    /**
     * Setup expectations: Facebook app is on the home page.
     *
     * This method moves the Facebook app to the News Feed tab of the home page.
     */
    public abstract void goToNewsFeed();

    /**
     * Setup expectations: Facebook is on the News Feed tab of the home page.
     *
     * This method moves the Facebook app to the status update page.
     */
    public abstract void goToStatusUpdate();

    /**
     * Setup expectations: Facebook is on the status update page.
     *
     * This method clicks on the status update text field to move the keyboard cursor there
     */
    public abstract void clickStatusUpdateTextField();

    /**
     * Setup expections: Facebook is on the status update page.
     *
     * This method sets the status update text.
     *
     * @param statusText text for status update
     */
    public abstract void setStatusText(String statusText);

    /**
     * Setup expectations: Facebook is on the status update page.
     *
     * This method posts the status update.
     */
    public abstract void postStatusUpdate();

    /**
     * Setup expectations: Facebook app is on the login page.
     *
     * This method attempts to log in using the specified username and password.
     *
     * @param username username of Facebook account
     * @param password password of Facebook account
     */
    public abstract void login(String username, String password);
}
