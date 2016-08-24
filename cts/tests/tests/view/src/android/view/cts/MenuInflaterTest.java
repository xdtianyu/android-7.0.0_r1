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

package android.view.cts;

import android.view.cts.R;

import android.app.Activity;
import android.content.res.Resources;
import android.cts.util.WidgetTestUtils;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.test.ActivityInstrumentationTestCase2;
import android.test.UiThreadTest;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.SubMenu;
import android.widget.PopupMenu;

/**
 * Test {@link MenuInflater}.
 */
public class MenuInflaterTest extends ActivityInstrumentationTestCase2<MenuInflaterCtsActivity> {
    private MenuInflater mMenuInflater;

    private Activity mActivity;

    public MenuInflaterTest() {
        super("android.view.cts", MenuInflaterCtsActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
    }

    private Menu createMenu(Activity context) {
        return new PopupMenu(context, null).getMenu();
    }

    @UiThreadTest
    public void testConstructor() {
        new MenuInflater(mActivity);
    }

    @UiThreadTest
    public void testInflate() {
        Menu menu = createMenu(mActivity);
        assertEquals(0, menu.size());

        if (mMenuInflater == null) {
            mMenuInflater = mActivity.getMenuInflater();
        }

        mMenuInflater.inflate(android.view.cts.R.menu.browser, menu);
        assertNotNull(menu);
        assertEquals(1, menu.size());

        try {
            mMenuInflater.inflate(0, menu);
            fail("should throw Resources.NotFoundException");
        } catch (Resources.NotFoundException e) {
        }

        try {
            mMenuInflater.inflate(android.view.cts.R.menu.browser, null);
            fail("should throw NullPointerException");
        } catch (NullPointerException e) {
        }
    }

    // Check wheher the objects are created correctly from xml files
    @UiThreadTest
    public void testInflateFromXml(){
        if (mMenuInflater == null) {
            mMenuInflater = mActivity.getMenuInflater();
        }

        // the visibility and shortcut
        Menu menu = createMenu(mActivity);
        mMenuInflater.inflate(R.menu.visible_shortcut, menu);

        assertTrue(menu.findItem(R.id.visible_item).isVisible());
        assertEquals('a', menu.findItem(R.id.visible_item).getAlphabeticShortcut());

        assertFalse(menu.findItem(R.id.hidden_item).isVisible());
        assertEquals('b', menu.findItem(R.id.hidden_item).getAlphabeticShortcut());

        assertEquals(R.id.hidden_group, menu.findItem(R.id.hidden_by_group).getGroupId());
        assertFalse(menu.findItem(R.id.hidden_by_group).isVisible());
        assertEquals('c', menu.findItem(R.id.hidden_by_group).getAlphabeticShortcut());

        // the titles and icons
        menu = createMenu(mActivity);
        mMenuInflater.inflate(android.view.cts.R.menu.title_icon, menu);

        assertEquals("Start", menu.findItem(R.id.start).getTitle());
        assertIconUsingDrawableRes((BitmapDrawable) menu.findItem(R.id.start).getIcon(),
                R.drawable.start);

        assertEquals("Pass", menu.findItem(R.id.pass).getTitle());
        assertIconUsingDrawableRes((BitmapDrawable) menu.findItem(R.id.pass).getIcon(),
                R.drawable.pass);

        assertEquals("Failed", menu.findItem(R.id.failed).getTitle());
        assertIconUsingDrawableRes((BitmapDrawable) menu.findItem(R.id.failed).getIcon(),
                R.drawable.failed);

        // the orders and categories
        menu = createMenu(mActivity);
        mMenuInflater.inflate(android.view.cts.R.menu.category_order, menu);
        // default category
        assertEquals(R.id.most_used_items, menu.findItem(R.id.first_most_item).getGroupId());
        assertEquals(1, menu.findItem(R.id.first_most_item).getOrder());

        assertEquals(R.id.most_used_items, menu.findItem(R.id.middle_most_item).getGroupId());
        assertEquals(3, menu.findItem(R.id.middle_most_item).getOrder());

        assertEquals(R.id.most_used_items, menu.findItem(R.id.last_most_item).getGroupId());
        assertEquals(5, menu.findItem(R.id.last_most_item).getOrder());

        // specific category (CATEGORY_SECONDARY)
        assertEquals(R.id.least_used_items, menu.findItem(R.id.first_least_item).getGroupId());
        assertEquals(Menu.CATEGORY_SECONDARY + 0, menu.findItem(R.id.first_least_item).getOrder());

        assertEquals(R.id.least_used_items, menu.findItem(R.id.middle_least_item).getGroupId());
        assertEquals(Menu.CATEGORY_SECONDARY + 2,
                menu.findItem(R.id.middle_least_item).getOrder());

        assertEquals(R.id.least_used_items, menu.findItem(R.id.last_least_item).getGroupId());
        assertEquals(Menu.CATEGORY_SECONDARY + 4, menu.findItem(R.id.last_least_item).getOrder());

        // the checkables
        menu = createMenu(mActivity);
        mMenuInflater.inflate(android.view.cts.R.menu.checkable, menu);
        // noncheckables
        assertEquals(R.id.noncheckable_group,
                menu.findItem(R.id.noncheckable_item_1).getGroupId());
        assertFalse(menu.findItem(R.id.noncheckable_item_1).isCheckable());

        assertEquals(R.id.noncheckable_group,
                menu.findItem(R.id.noncheckable_item_2).getGroupId());
        assertFalse(menu.findItem(R.id.noncheckable_item_2).isCheckable());

        assertEquals(R.id.noncheckable_group,
                menu.findItem(R.id.noncheckable_item_3).getGroupId());
        assertFalse(menu.findItem(R.id.noncheckable_item_3).isCheckable());

        // multiple checkables (item 2 and item 3 are both checked)
        assertEquals(R.id.checkable_group, menu.findItem(R.id.checkable_item_1).getGroupId());
        assertTrue(menu.findItem(R.id.checkable_item_1).isCheckable());
        assertFalse(menu.findItem(R.id.checkable_item_1).isChecked());

        assertEquals(R.id.checkable_group, menu.findItem(R.id.checkable_item_3).getGroupId());
        assertTrue(menu.findItem(R.id.checkable_item_2).isCheckable());
        assertTrue(menu.findItem(R.id.checkable_item_2).isChecked());

        assertEquals(R.id.checkable_group, menu.findItem(R.id.checkable_item_2).getGroupId());
        assertTrue(menu.findItem(R.id.checkable_item_3).isCheckable());
        assertTrue(menu.findItem(R.id.checkable_item_3).isChecked());

        // make item 1 checked and item 2 and item 3 will remain checked
        menu.findItem(R.id.checkable_item_1).setChecked(true);
        assertTrue(menu.findItem(R.id.checkable_item_1).isChecked());
        assertTrue(menu.findItem(R.id.checkable_item_2).isChecked());
        assertTrue(menu.findItem(R.id.checkable_item_3).isChecked());

        // exclusive checkables (only item 3 is checked)
        assertEquals(R.id.exclusive_checkable_group,
                menu.findItem(R.id.exclusive_checkable_item_1).getGroupId());
        assertTrue(menu.findItem(R.id.exclusive_checkable_item_1).isCheckable());
        assertFalse(menu.findItem(R.id.exclusive_checkable_item_1).isChecked());

        assertEquals(R.id.exclusive_checkable_group,
                menu.findItem(R.id.exclusive_checkable_item_3).getGroupId());
        assertTrue(menu.findItem(R.id.exclusive_checkable_item_2).isCheckable());
        assertFalse(menu.findItem(R.id.exclusive_checkable_item_2).isChecked());

        assertEquals(R.id.exclusive_checkable_group,
                menu.findItem(R.id.exclusive_checkable_item_2).getGroupId());
        assertTrue(menu.findItem(R.id.exclusive_checkable_item_3).isCheckable());
        assertTrue(menu.findItem(R.id.exclusive_checkable_item_3).isChecked());

        // make item 1 checked and item 3 will be unchecked
        menu.findItem(R.id.exclusive_checkable_item_1).setChecked(true);
        assertTrue(menu.findItem(R.id.exclusive_checkable_item_1).isChecked());
        assertFalse(menu.findItem(R.id.exclusive_checkable_item_2).isChecked());
        assertFalse(menu.findItem(R.id.exclusive_checkable_item_3).isChecked());

        // checkables without group (all in a sub menu)
        SubMenu subMenu = menu.findItem(R.id.submenu).getSubMenu();
        assertNotNull(subMenu);

        assertTrue(subMenu.findItem(R.id.nongroup_checkable_item_1).isCheckable());
        assertFalse(subMenu.findItem(R.id.nongroup_checkable_item_1).isChecked());

        assertTrue(subMenu.findItem(R.id.nongroup_checkable_item_2).isCheckable());
        assertTrue(subMenu.findItem(R.id.nongroup_checkable_item_2).isChecked());

        assertTrue(subMenu.findItem(R.id.nongroup_checkable_item_3).isCheckable());
        assertTrue(subMenu.findItem(R.id.nongroup_checkable_item_3).isChecked());

        // make item 1 checked and item 2 and item 3 will remain checked
        subMenu.findItem(R.id.nongroup_checkable_item_1).setChecked(true);
        assertTrue(menu.findItem(R.id.nongroup_checkable_item_1).isChecked());
        assertTrue(menu.findItem(R.id.nongroup_checkable_item_2).isChecked());
        assertTrue(menu.findItem(R.id.nongroup_checkable_item_3).isChecked());
    }

    public void assertIconUsingDrawableRes(BitmapDrawable b, int resId) {
        Bitmap expected = BitmapFactory.decodeResource(mActivity.getResources(), resId);
        WidgetTestUtils.assertEquals(expected, b.getBitmap());
    }
}
