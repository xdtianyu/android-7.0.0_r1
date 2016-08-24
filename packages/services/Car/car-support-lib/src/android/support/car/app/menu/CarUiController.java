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

package android.support.car.app.menu;

import android.graphics.Bitmap;
import android.os.Bundle;
import android.support.car.app.CarAppUtil;
import android.view.View;
import android.widget.EditText;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;

/**
 * A controller for a {@link android.support.car.app.CarActivity} to manipulate its car UI, and
 * under the hood it talks to a car ui provider.
 */
public abstract class CarUiController {
    static final String PROJECTED_UI_CONTROLLER =
            "com.google.android.car.ProjectedCarUiController";

    protected final CarDrawerActivity mActivity;

    public CarUiController(CarDrawerActivity activity) {
        mActivity = activity;
        validateCarUiPackage();
    }

    public static CarUiController createCarUiController(CarDrawerActivity activity) {
        if (CarAppUtil.isEmbeddedCar(activity.getContext())) {
            return new EmbeddedCarUiController(activity);
        } else {
            return getProjectedCarUiController(PROJECTED_UI_CONTROLLER, activity);
        }
    }

    private static CarUiController getProjectedCarUiController(String className,
            CarDrawerActivity activity) {
        Class uiControllerClass = null;
        try {
            uiControllerClass = Class.forName(className);
        } catch (ClassNotFoundException e) {
            throw new IllegalArgumentException("Cannot find ProjectedCarUiController:" +
                    className, e);
        }
        Constructor<?> ctor;
        try {
            ctor = uiControllerClass.getDeclaredConstructor(CarDrawerActivity.class);
        } catch (NoSuchMethodException e) {
            throw new IllegalArgumentException("Cannot construct ProjectedCarUiController," +
                    " no constructor: " + className, e);
        }
        try {
            return (CarUiController) ctor.newInstance(activity);
        } catch (InstantiationException | IllegalAccessException | IllegalArgumentException
                | InvocationTargetException e) {
            throw new IllegalArgumentException(
                    "Cannot construct ProjectedCarUiController, constructor failed for "
                            + uiControllerClass.getName(), e);
        }
    }

    public abstract void validateCarUiPackage();

    public abstract int getFragmentContainerId();

    public abstract void setTitle(CharSequence title);

    public abstract void setScrimColor(int color);

    public abstract View getContentView();

    public abstract void registerCarMenuCallbacks(CarMenuCallbacks callbacks);

    public abstract void restoreMenuButtonDrawable();

    public abstract void setMenuButtonBitmap(Bitmap bitmap);

    public abstract void setLightMode();

    /**
     * Set the System UI to be dark.
     */
    public abstract void setDarkMode();

    /**
     * Set the System UI to be dark during day mode and light during night mode.
     */
    public abstract void setAutoLightDarkMode();

    /**
     * Sets the application background to the given {@link android.graphics.Bitmap}.
     *
     * @param bitmap to use as background.
     */
    public abstract void setBackground(Bitmap bitmap);

    public abstract void onRestoreInstanceState(Bundle savedState);

    public abstract void onSaveInstanceState(Bundle outState);

    public abstract void closeDrawer();

    public abstract void openDrawer();

    public abstract void showMenu(String id, String title);

    public abstract void onStart();

    public abstract void onResume();

    public abstract void onPause();

    public abstract void onStop();

    public abstract void showSearchBox(View.OnClickListener listener);

    public abstract void setSearchBoxColors(int backgroundColor, int googleLogoColor, int textColor,
                                            int hintTextColor);

    public abstract void setSearchBoxEditListener(SearchBoxEditListener listener);

    public abstract EditText startInput(
            String hint, View.OnClickListener searchBoxClickListener);

    public abstract CharSequence getText();

    public abstract void stopInput();

    public abstract void setSearchBoxEndView(View view);

    public abstract void onChildrenChanged(String parentId);

    public abstract void onChildChanged(String parentId, Bundle item);

    public abstract void showToast(String msg, int duration);
}
