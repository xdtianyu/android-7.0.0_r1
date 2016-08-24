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
 * limitations under the License
 */
package android.systemui.cts;

import android.app.Activity;
import android.graphics.Color;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup.LayoutParams;


/**
 * An activity that exercises SYSTEM_UI_FLAG_LIGHT_STATUS_BAR.
 */
public class LightStatusBarActivity extends Activity {

    private View mContent;

    public void onCreate(Bundle bundle){
        super.onCreate(bundle);

        mContent = new View(this);
        mContent.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT,
                LayoutParams.MATCH_PARENT));
        setContentView(mContent);
    }

    public void setLightStatusBar(boolean lightStatusBar) {
        int vis = getWindow().getDecorView().getSystemUiVisibility();
        if (lightStatusBar) {
            vis |= View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR;
        } else {
            vis &= ~View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR;
        }
        getWindow().getDecorView().setSystemUiVisibility(vis);
    }

    public int getTop() {
        return mContent.getLocationOnScreen()[1];
    }

    public int getWidth() {
        return mContent.getWidth();
    }
}
