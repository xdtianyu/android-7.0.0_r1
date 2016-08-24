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

package android.widget.cts;

import android.app.Activity;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.PorterDuff.Mode;
import android.test.ActivityInstrumentationTestCase;
import android.test.UiThreadTest;
import android.test.suitebuilder.annotation.SmallTest;
import android.widget.Switch;
import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;

/**
 * Test {@link Switch}.
 */
@SmallTest
public class SwitchTest extends ActivityInstrumentationTestCase<SwitchCtsActivity> {
    private Activity mActivity;
    private Switch mSwitch;

    public SwitchTest() {
        super("android.widget.cts", SwitchCtsActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
        mSwitch = (Switch) mActivity.findViewById(R.id.switch_view);
    }

    @UiThreadTest
    public void testConstructor() {
        new Switch(mActivity);

        new Switch(mActivity, null);

        new Switch(mActivity, null, android.R.attr.switchStyle);

        new Switch(mActivity, null, 0, android.R.style.Widget_Material_Light_CompoundButton_Switch);
    }

    @UiThreadTest
    public void testAccessThumbTint() {
        assertEquals(Color.WHITE, mSwitch.getThumbTintList().getDefaultColor());
        assertEquals(Mode.SRC_OVER, mSwitch.getThumbTintMode());

        ColorStateList colors = ColorStateList.valueOf(Color.RED);
        mSwitch.setThumbTintList(colors);
        mSwitch.setThumbTintMode(Mode.XOR);

        assertSame(colors, mSwitch.getThumbTintList());
        assertEquals(Mode.XOR, mSwitch.getThumbTintMode());
    }

    @UiThreadTest
    public void testAccessTrackTint() {
        assertEquals(Color.BLACK, mSwitch.getTrackTintList().getDefaultColor());
        assertEquals(Mode.SRC_ATOP, mSwitch.getTrackTintMode());

        ColorStateList colors = ColorStateList.valueOf(Color.RED);
        mSwitch.setTrackTintList(colors);
        mSwitch.setTrackTintMode(Mode.XOR);

        assertSame(colors, mSwitch.getTrackTintList());
        assertEquals(Mode.XOR, mSwitch.getTrackTintMode());
    }
}
