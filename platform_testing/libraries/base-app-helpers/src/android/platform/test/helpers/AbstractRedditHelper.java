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

public abstract class AbstractRedditHelper extends AbstractStandardAppHelper {

    public AbstractRedditHelper(Instrumentation instr) {
        super(instr);
    }

    /*
     * Setup expectations: Reddit app is open.
     *
     * This method moves the Reddit app to the front page.
     */
    public abstract void goToFrontPage();

    /*
     * Setup expectations: Reddit app is on the front pages.
     *
     * This method moves the Reddit app to the first visible article's comment page.
     */
    public abstract void goToFirstArticleComments();

    /*
     * Setup expectations: Reddit app is on the front page.
     *
     * This method scrolls the front page.
     *
     * @param direction Direction in which to scroll, must be UP or DOWN
     * @param percent   Percent of page to scroll
     * @return boolean  Whether the page can still scroll in the given direction
     */
    public abstract boolean scrollFrontPage(Direction direction, float percent);

    /*
     * Setup expectations: Reddit app is on an article's comment page.
     *
     * This method scrolls the comment page.
     *
     * @param direction Direction in which to scroll, must be UP or DOWN
     * @param percent   Percent of page to scroll
     * @return boolean  Whether the page can still scroll in the given direction
     */
    public abstract boolean scrollCommentPage(Direction direction, float percent);
}
