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

import android.content.pm.PackageManager.NameNotFoundException;

public interface IStandardAppHelper {

    /**
     * Setup expectation: On the launcher home screen.
     *
     * Launches the desired application.
     */
    abstract void open();

    /**
     * Setup expectation: None
     *
     * Presses back until the launcher package is visible, i.e. the home screen. This can be
     * overriden for custom functionality, however consider and document the exit state if doing so.
     */
    abstract void exit();

    /**
     * Setup expectations: This application is on the initial launch screen.
     *
     * This method will dismiss all visible relevant dialogs and block until this process is
     * complete.
     */
    abstract void dismissInitialDialogs();

    /**
     * Setup expectations: None
     *
     * @return the package name for this helper's application.
     */
    abstract String getPackage();

    /**
     * Setup expectations: None.
     *
     * @return the name for this application in the launcher.
     */
    abstract String getLauncherName();

    /**
     * Setup expectations: None
     *
     * This method will return the version String from PackageManager.
     * @param pkgName the application package
     * @throws NameNotFoundException if the package is not found in PM
     * @return the version as a String
     */
    abstract String getVersion() throws NameNotFoundException;
}
