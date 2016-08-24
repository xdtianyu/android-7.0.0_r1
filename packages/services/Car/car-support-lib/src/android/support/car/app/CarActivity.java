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

package android.support.car.app;

import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.LayoutRes;
import android.support.car.Car;
import android.support.car.input.CarInputManager;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.View;
import android.view.Window;

/**
 * Abstraction for car UI. Car applications should implement this for car UI.
 * The API looks like {@link android.app.Activity}, and behaves like one, but it is neither
 * android {@link android.app.Activity} nor {@link android.app.Context}.
 * Applications should use {@link #getContext()} to get necessary {@link android.app.Context}.
 */
public abstract class CarActivity {
    private static final String TAG = "CarActivity";
    public interface RequestPermissionsRequestCodeValidator {
        public void validateRequestPermissionsRequestCode(int requestCode);
    }
    /**
     * Interface to connect {@link CarActivity} to {@link android.app.Activity} or other app model.
     * This interface provides utility for {@link CarActivity} to do things like manipulating view,
     * handling menu, and etc.
     */
    public abstract static class Proxy {
        abstract public void setIntent(Intent i);
        abstract public void setContentView(View view);
        abstract public void setContentView(int layoutResID);
        abstract public Resources getResources();
        abstract public View findViewById(int id);
        abstract public LayoutInflater getLayoutInflater();
        abstract public Intent getIntent();
        abstract public void finish();
        abstract public CarInputManager getCarInputManager();
        abstract public boolean isFinishing();
        abstract public MenuInflater getMenuInflater();
        abstract public void finishAfterTransition();
        abstract public Window getWindow();
        abstract public void setResult(int resultCode);
        abstract public void setResult(int resultCode, Intent data);

        public void requestPermissions(String[] permissions, int requestCode) {
            Log.w(TAG, "No support for requestPermissions");
        }
        public boolean shouldShowRequestPermissionRationale(String permission) {
            Log.w(TAG, "No support for shouldShowRequestPermissionRationale");
            return false;
        }
        public void startActivityForResult(Intent intent, int requestCode) {
            Log.w(TAG, "No support for startActivityForResult");
        };
    }

    /** @hide */
    public static final int CMD_ON_CREATE = 0;
    /** @hide */
    public static final int CMD_ON_START = 1;
    /** @hide */
    public static final int CMD_ON_RESTART = 2;
    /** @hide */
    public static final int CMD_ON_RESUME = 3;
    /** @hide */
    public static final int CMD_ON_PAUSE = 4;
    /** @hide */
    public static final int CMD_ON_STOP = 5;
    /** @hide */
    public static final int CMD_ON_DESTROY = 6;
    /** @hide */
    public static final int CMD_ON_BACK_PRESSED = 7;
    /** @hide */
    public static final int CMD_ON_SAVE_INSTANCE_STATE = 8;
    /** @hide */
    public static final int CMD_ON_RESTORE_INSTANCE_STATE = 9;
    /** @hide */
    public static final int CMD_ON_CONFIG_CHANGED = 10;
    /** @hide */
    public static final int CMD_ON_REQUEST_PERMISSIONS_RESULT = 11;
    /** @hide */
    public static final int CMD_ON_NEW_INTENT = 12;
    /** @hide */
    public static final int CMD_ON_ACTIVITY_RESULT = 13;
    /** @hide */
    public static final int CMD_ON_POST_RESUME = 14;
    /** @hide */
    public static final int CMD_ON_LOW_MEMORY = 15;

    private final Proxy mProxy;
    private final Context mContext;
    private final Car mCar;
    private final Handler mHandler = new Handler();

    public CarActivity(Proxy proxy, Context context, Car car) {
        mProxy = proxy;
        mContext = context;
        mCar = car;
    }

    public Context getContext() {
        return mContext;
    }

    public Car getCar() {
        if (mCar == null) {
            throw new IllegalStateException("The default Car is not available. You can either " +
                    "create a Car by yourself or indicate the need of the default Car in the " +
                    "CarProxyActivity's constructor.");
        }
        return mCar;
    }

    public Resources getResources() {
        return mProxy.getResources();
    }

    public void setContentView(View view) {
        mProxy.setContentView(view);
    }

    public void setContentView(@LayoutRes int resourceId) {
        mProxy.setContentView(resourceId);
    }

    public LayoutInflater getLayoutInflater() {
        return mProxy.getLayoutInflater();
    }

    public Intent getIntent() {
        return mProxy.getIntent();
    }

    public void setResult(int resultCode){
        mProxy.setResult(resultCode);
    }

    public void setResult(int resultCode, Intent data) {
        mProxy.setResult(resultCode, data);
    }

    public CarInputManager getInputManager() {
        return mProxy.getCarInputManager();
    }

    public View findViewById(int id) {
        return mProxy.findViewById(id);
    }

    public void finish() {
        mProxy.finish();
    }

    public boolean isFinishing() {
        return mProxy.isFinishing();
    }

    public boolean shouldShowRequestPermissionRationale(String permission) {
        return mProxy.shouldShowRequestPermissionRationale(permission);
    }

    public void requestPermissions(String[] permissions, int requestCode) {
        if (this instanceof RequestPermissionsRequestCodeValidator) {
            ((RequestPermissionsRequestCodeValidator) this)
                    .validateRequestPermissionsRequestCode(requestCode);
        }
        mProxy.requestPermissions(permissions, requestCode);
    }

    public void setIntent(Intent i) {
        mProxy.setIntent(i);
    }

    public void finishAfterTransition() {
        mProxy.finishAfterTransition();
    }

    public MenuInflater getMenuInflater() {
        return mProxy.getMenuInflater();
    }

    public Window getWindow() {
        return mProxy.getWindow();
    }

    public Object getLastNonConfigurationInstance() {
        return null;
    }

    public void startActivityForResult(Intent intent, int requestCode) {
        mProxy.startActivityForResult(intent, requestCode);
    }

    public void runOnUiThread(Runnable runnable) {
        if (Thread.currentThread() == mHandler.getLooper().getThread()) {
            runnable.run();
        } else {
            mHandler.post(runnable);
        }
    }

    /** @hide */
    public void dispatchCmd(int cmd, Object... args) {

        switch (cmd) {
            case CMD_ON_CREATE:
                assertArgsLength(1, args);
                onCreate((Bundle) args[0]);
                break;
            case CMD_ON_START:
                onStart();
                break;
            case CMD_ON_RESTART:
                onRestart();
                break;
            case CMD_ON_RESUME:
                onResume();
                break;
            case CMD_ON_POST_RESUME:
                onPostResume();
                break;
            case CMD_ON_PAUSE:
                onPause();
                break;
            case CMD_ON_STOP:
                onStop();
                break;
            case CMD_ON_DESTROY:
                onDestroy();
                break;
            case CMD_ON_BACK_PRESSED:
                onBackPressed();
                break;
            case CMD_ON_SAVE_INSTANCE_STATE:
                assertArgsLength(1, args);
                onSaveInstanceState((Bundle) args[0]);
                break;
            case CMD_ON_RESTORE_INSTANCE_STATE:
                assertArgsLength(1, args);
                onRestoreInstanceState((Bundle) args[0]);
                break;
            case CMD_ON_REQUEST_PERMISSIONS_RESULT:
                assertArgsLength(3, args);
                onRequestPermissionsResult(((Integer) args[0]).intValue(),
                        (String[]) args[1], convertArray((Integer[]) args[2]));
                break;
            case CMD_ON_CONFIG_CHANGED:
                assertArgsLength(1, args);
                onConfigurationChanged((Configuration) args[0]);
                break;
            case CMD_ON_NEW_INTENT:
                assertArgsLength(1, args);
                onNewIntent((Intent) args[0]);
                break;
            case CMD_ON_ACTIVITY_RESULT:
                assertArgsLength(3, args);
                onActivityResult(((Integer) args[0]).intValue(), ((Integer) args[1]).intValue(),
                        (Intent) args[2]);
                break;
            case CMD_ON_LOW_MEMORY:
                onLowMemory();
                break;
            default:
                throw new RuntimeException("Unknown dispatch cmd for CarActivity, " + cmd);
        }

    }

    protected void onCreate(Bundle savedInstanceState) {
    }

    protected void onStart() {
    }

    protected void onRestart() {
    }

    protected void onResume() {
    }

    protected void onPostResume() {
    }

    protected void onPause() {
    }

    protected void onStop() {
    }

    protected void onDestroy() {
    }

    protected void onRestoreInstanceState(Bundle savedInstanceState) {
    }

    protected void onSaveInstanceState(Bundle outState) {
    }

    protected void onBackPressed() {
    }

    protected void onConfigurationChanged(Configuration newConfig) {
    }

    protected void onNewIntent(Intent intent) {
    }

    public void onRequestPermissionsResult(int requestCode, String[] permissions,
            int[] grantResults) {
    }

    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
    }

    public Object onRetainNonConfigurationInstance() {
        return null;
    }

    // TODO: hook up panel menu if it's needed in any apps.
    public boolean onCreatePanelMenu(int featureId, Menu menu) {
        return false; // default menu will not be displayed.
    }

    public View onCreateView(View parent, String name, Context context, AttributeSet attrs) {
        // CarFragmentActivity can override this to dispatch onCreateView to fragments
        return null;
    }

    public void onLowMemory() {
    }

    private void assertArgsLength(int length, Object... args) {
        if (args == null || args.length != length) {
            throw new IllegalArgumentException(
                    String.format("Wrong number of parameters. Expected: %d Actual: %d",
                            length, args == null ? 0 : args.length));
        }
    }

    private static int[] convertArray(Integer[] array) {
        int[] results = new int[array.length];
        for(int i = 0; i < results.length; i++) {
            results[i] = array[i].intValue();
        }
        return results;
    }
}
