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
package android.car.cts;

import android.car.app.menu.CarMenuCallbacks;
import android.car.app.menu.SearchBoxEditListener;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.test.AndroidTestCase;
import android.util.Log;
import android.view.View;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Test the existence of compatibility apis in the car ui provider.
 *
 * This test will only be run on devices with automotive feature.
 */
public class CarUiProviderTest extends AndroidTestCase {
    private static final String TAG = "CarUiProviderTest";
    private static final String UI_ENTRY_CLASS_NAME = ".CarUiEntry";
    private static final String CAR_UI_PROVIDER_PKG = "android.car.ui.provider";

    private static final Map<String, Class<?>[]> COMPATIBILITY_APIS =
            new HashMap<String, Class<?>[]>();

    static {
        COMPATIBILITY_APIS.put("onStart", new Class<?>[]{});
        COMPATIBILITY_APIS.put("onResume", new Class<?>[]{});
        COMPATIBILITY_APIS.put("onPause", new Class<?>[]{});
        COMPATIBILITY_APIS.put("onStop", new Class<?>[]{});
        COMPATIBILITY_APIS.put("getContentView", new Class<?>[]{});
        COMPATIBILITY_APIS.put("setCarMenuCallbacks", new Class<?>[]{CarMenuCallbacks.class});
        COMPATIBILITY_APIS.put("getFragmentContainerId", new Class<?>[]{});
        COMPATIBILITY_APIS.put("setBackground", new Class<?>[]{Bitmap.class});
        COMPATIBILITY_APIS.put("hideMenuButton", new Class<?>[]{});
        COMPATIBILITY_APIS.put("restoreMenuDrawable", new Class<?>[]{});
        COMPATIBILITY_APIS.put("setMenuProgress", new Class<?>[]{float.class});
        COMPATIBILITY_APIS.put("setScrimColor", new Class<?>[]{int.class});
        COMPATIBILITY_APIS.put("setTitle", new Class<?>[]{CharSequence.class});
        COMPATIBILITY_APIS.put("setTitleText", new Class<?>[]{CharSequence.class});
        COMPATIBILITY_APIS.put("closeDrawer", new Class<?>[]{});
        COMPATIBILITY_APIS.put("openDrawer", new Class<?>[]{});
        COMPATIBILITY_APIS.put("showMenu", new Class<?>[]{String.class, String.class});
        COMPATIBILITY_APIS.put("setMenuButtonColor", new Class<?>[]{int.class});
        COMPATIBILITY_APIS.put("showTitle", new Class<?>[]{});
        COMPATIBILITY_APIS.put("hideTitle", new Class<?>[]{});
        COMPATIBILITY_APIS.put("setLightMode", new Class<?>[]{});
        COMPATIBILITY_APIS.put("setDarkMode", new Class<?>[]{});
        COMPATIBILITY_APIS.put("setAutoLightDarkMode", new Class<?>[]{});
        COMPATIBILITY_APIS.put("onRestoreInstanceState", new Class<?>[]{Bundle.class});
        COMPATIBILITY_APIS.put("onSaveInstanceState", new Class<?>[]{Bundle.class});
        COMPATIBILITY_APIS.put("showSearchBox", new Class<?>[]{View.OnClickListener.class});
        COMPATIBILITY_APIS.put("setSearchBoxEndView", new Class<?>[]{View.class});
        COMPATIBILITY_APIS.put("getSearchBoxText", new Class<?>[]{});
        COMPATIBILITY_APIS.put("showToast", new Class<?>[]{String.class, long.class});
        COMPATIBILITY_APIS.put("stopInput", new Class<?>[]{});
        COMPATIBILITY_APIS.put("startInput", new Class<?>[]{String.class,
                View.OnClickListener.class});
        COMPATIBILITY_APIS.put("setSearchBoxEditListener",
                new Class<?>[]{SearchBoxEditListener.class});
        COMPATIBILITY_APIS.put("setSearchBoxColors", new Class<?>[]{int.class, int.class, int.class,
                int.class});
    }

    private boolean mIsCar = false;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mIsCar = getContext().getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_AUTOMOTIVE
        );
    }

    public void testCarUiProvider() throws Exception {
        if (!mIsCar) {
            Log.d(TAG, "Bypass CarUiProviderTest on non-automotive devices");
            return;
        }
        checkCompatibilityApi();
    }

    private void checkCompatibilityApi() {
        List<String> missingApis = new ArrayList<String>();
        Class<?> loadedClass = null;
        try {
            Context carUiContext = getContext().createPackageContext(
                    CAR_UI_PROVIDER_PKG,
                    Context.CONTEXT_INCLUDE_CODE | Context.CONTEXT_IGNORE_SECURITY);

            int flag = getContext().getPackageManager()
                    .getApplicationInfo(CAR_UI_PROVIDER_PKG, 0).flags;
            assertEquals(true, (flag & ApplicationInfo.FLAG_SYSTEM) != 0
                    || (flag & ApplicationInfo.FLAG_UPDATED_SYSTEM_APP) != 0);

            ClassLoader classLoader = carUiContext.getClassLoader();
            loadedClass = classLoader.loadClass(CAR_UI_PROVIDER_PKG + UI_ENTRY_CLASS_NAME);
        } catch (PackageManager.NameNotFoundException e) {
            fail("CarUiProvider package does not exsit");
        } catch (ClassNotFoundException e) {
            fail("CarUiEntry class not found");
        }

        if (loadedClass == null) {
            fail("Fail to load CarUiEntry class");
        }

        for (Map.Entry<String, Class<?>[]> method : COMPATIBILITY_APIS.entrySet()) {
            try {
                loadedClass.getDeclaredMethod(method.getKey(), method.getValue());
            } catch (NoSuchMethodException e) {
                missingApis.add(method.getKey());
            }
        }
        assertEquals("Missing the following APIs from CarUiProvider"
                + Arrays.toString(missingApis.toArray()), 0, missingApis.size());
    }
}
