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
package android.support.car.app.menu;

import android.car.app.menu.CarUiEntry;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.support.car.app.menu.compat.EmbeddedCarMenuCallbacksCompat;
import android.support.car.app.menu.compat.EmbeddedSearchBoxEditListenerCompat;
import android.view.View;
import android.widget.EditText;

import java.lang.reflect.InvocationTargetException;
/**
 * A {@link android.support.car.app.menu.CarUiController} that talks to embedded car ui provider.
 * @hide
 */
public class EmbeddedCarUiController extends CarUiController {

    private static final String TAG = "EmbeddedCarUiController";
    // TODO: load the package name and class name from resources
    private static final String UI_ENTRY_CLASS_NAME = ".CarUiEntry";
    private static final String CAR_UI_PROVIDER_PKG = "android.car.ui.provider";
    private static final String CAR_SERVICE_PKG = "com.android.car";

    private CarUiEntry mCarUiEntry;
    private EmbeddedCarMenuCallbacksCompat mCallback;

    public EmbeddedCarUiController(CarDrawerActivity activity) {
        super(activity);
        try {
            Context carUiContext = mActivity.getContext().createPackageContext(
                    CAR_UI_PROVIDER_PKG,
                    Context.CONTEXT_INCLUDE_CODE | Context.CONTEXT_IGNORE_SECURITY);

            ClassLoader classLoader = carUiContext.getClassLoader();
            Class<?> loadedClass = classLoader.loadClass(CAR_UI_PROVIDER_PKG + UI_ENTRY_CLASS_NAME);
            mCarUiEntry = (CarUiEntry) loadedClass.getConstructor(Context.class, Context.class)
                    .newInstance(carUiContext, mActivity.getContext());
        } catch (PackageManager.NameNotFoundException | ClassNotFoundException e) {
            throw new RuntimeException("Cannot find CarUiEntry from " + CAR_UI_PROVIDER_PKG + "/"
                    + UI_ENTRY_CLASS_NAME, e);
        } catch (IllegalAccessException | InvocationTargetException | InstantiationException
                | NoSuchMethodException  e) {
            throw new RuntimeException("Cannot cast CarUiEntry.", e);
        }
    }

    @Override
    public void validateCarUiPackage() {
        try {
            PackageManager packageManager = mActivity.getContext().getPackageManager();
            int flag = packageManager.getApplicationInfo(CAR_UI_PROVIDER_PKG, 0).flags;
            if ((flag & ApplicationInfo.FLAG_SYSTEM) == 0
                    && (flag & ApplicationInfo.FLAG_UPDATED_SYSTEM_APP) == 0) {
                throw new SecurityException("CarUiProvider is not a system app!");
            }

            // Do not change the order of the two packages as it need to be in sync with
            // the error message.
            int signatureMatchResult =
                    packageManager.checkSignatures(CAR_SERVICE_PKG, CAR_UI_PROVIDER_PKG);
            if (signatureMatchResult != PackageManager.SIGNATURE_MATCH) {
                throw new SecurityException("CarUiProvider and CarService signature check" +
                        " failed. " + getSignatureFailureMessage(signatureMatchResult));
            }
        } catch (PackageManager.NameNotFoundException e) {
            throw new RuntimeException("Cannot find CarUiProvider" + CAR_UI_PROVIDER_PKG, e);
        }
    }

    @Override
    public int getFragmentContainerId() {
        return mCarUiEntry.getFragmentContainerId();
    }

    @Override
    public void setTitle(CharSequence title) {
        mCarUiEntry.setTitle(title);
    }

    @Override
    public void setScrimColor(int color) {
        mCarUiEntry.setScrimColor(color);
    }

    @Override
    public View getContentView() {
        return mCarUiEntry.getContentView();
    }

    @Override
    public void registerCarMenuCallbacks(final CarMenuCallbacks callbacks) {
        mCallback = new EmbeddedCarMenuCallbacksCompat(mActivity, callbacks);
        mCarUiEntry.setCarMenuCallbacks(mCallback);
    }

    @Override
    public void restoreMenuButtonDrawable() {
        mCarUiEntry.restoreMenuDrawable();
    }

    @Override
    public void setMenuButtonBitmap(Bitmap bitmap) {
        mCarUiEntry.setMenuButtonBitmap(bitmap);
    }

    @Override
    public void setLightMode() {
        mCarUiEntry.setLightMode();
    }

    @Override
    public void setDarkMode() {
        mCarUiEntry.setDarkMode();
    }

    @Override
    public void setAutoLightDarkMode() {
        mCarUiEntry.setAutoLightDarkMode();
    }

    @Override
    public void setBackground(Bitmap bitmap) {
        mCarUiEntry.setBackground(bitmap);
    }

    @Override
    public void onRestoreInstanceState(Bundle savedState) {
        mCarUiEntry.onRestoreInstanceState(savedState);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        mCarUiEntry.onSaveInstanceState(outState);
    }

    @Override
    public void closeDrawer() {
        mCarUiEntry.closeDrawer();
    }

    @Override
    public void openDrawer() {
        mCarUiEntry.openDrawer();
    }

    @Override
    public void showMenu(String id, String title) {
        mCarUiEntry.showMenu(id, title);
    }

    @Override
    public void onStart() {
        mCarUiEntry.onStart();
    }

    @Override
    public void onResume() {
        mCarUiEntry.onResume();
    }

    @Override
    public void onPause() {
        mCarUiEntry.onPause();
    }

    @Override
    public void onStop() {
        mCarUiEntry.onStop();
    }

    @Override
    public void showSearchBox(View.OnClickListener listener) {
        mCarUiEntry.showSearchBox(listener);
    }

    @Override
    public void setSearchBoxColors(int backgroundColor, int searchLogocolor, int textColor, int hintTextColor) {
        mCarUiEntry.setSearchBoxColors(backgroundColor, searchLogocolor, textColor, hintTextColor);
    }

    @Override
    public void setSearchBoxEditListener(SearchBoxEditListener listener) {
        mCarUiEntry.setSearchBoxEditListener(new EmbeddedSearchBoxEditListenerCompat(listener));
    }

    @Override
    public CharSequence getText() {
        return mCarUiEntry.getSearchBoxText();
    }

    @Override
    public void stopInput() {
        mCarUiEntry.stopInput();
    }

    @Override
    public EditText startInput(String hint, View.OnClickListener listener) {
        return mCarUiEntry.startInput(hint, listener);
    }

    @Override
    public void setSearchBoxEndView(View view) {
        mCarUiEntry.setSearchBoxEndView(view);
    }

    @Override
    public void onChildChanged(String parentId, Bundle item) {
        mCallback.onChildChanged(parentId, item);
    }

    @Override
    public void onChildrenChanged(String parentId) {
        mCallback.onChildrenChanged(parentId);
    }

    @Override
    public void showToast(String msg, int duration) {
        mCarUiEntry.showToast(msg, duration);
    }

    /**
     * Return more informative error message from the PackageManager's signature check result.
     */
    private static final String getSignatureFailureMessage(int code) {
        switch (code) {
            case PackageManager.SIGNATURE_NEITHER_SIGNED:
                return "Both CarService and CarUiProvider are not signed";
            case PackageManager.SIGNATURE_FIRST_NOT_SIGNED:
                return "CarService not signed";
            case PackageManager.SIGNATURE_SECOND_NOT_SIGNED:
                return "CarUiProvider not signed";
            case PackageManager.SIGNATURE_NO_MATCH:
                return "Signatures do not match";
            case PackageManager.SIGNATURE_UNKNOWN_PACKAGE:
                return "CarService not found";
            default:
                return "Unknown error code";
        }
    }
}
