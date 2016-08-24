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

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.Bundle;
import android.os.IBinder;
import android.support.car.Car;
import android.support.car.ServiceConnectionListener;
import android.support.car.input.CarInputManager;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.View;
import android.view.Window;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;

import java.lang.ref.WeakReference;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * android Activity controlling / proxying {@link CarActivity}. Applications should have its own
 * {@link android.app.Activity} overriding only constructor.
 */
public class CarProxyActivity extends Activity {
    private static final String TAG = "CarProxyActivity";

    private final Class mCarActivityClass;
    private final boolean mNeedConnectedCar;
    private Car mCar;
    // no synchronization, but main thread only
    private CarActivity mCarActivity;
    // no synchronization, but main thread only
    private CarInputManager mInputManager;

    private final CopyOnWriteArrayList<Pair<Integer, Object[]>> mCmds =
            new CopyOnWriteArrayList<Pair<Integer, Object[]>>();
    private final ServiceConnectionListener mConnectionListener= new ServiceConnectionListener() {

        @Override
        public void onServiceSuspended(int cause) {
            Log.w(TAG, "Car service suspended: " + cause);
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            Log.w(TAG, "Car service disconnected: " + name.toString());
        }

        @Override
        public void onServiceConnectionFailed(int cause) {
            Log.w(TAG, "Car service connection failed: " + cause);
        }

        @Override
        public void onServiceConnected(ComponentName name) {
            for (Pair<Integer, Object[]> cmd: mCmds) {
                mCarActivity.dispatchCmd(cmd.first, cmd.second);
            }
            mCmds.clear();
        }
    };

    private final CarActivity.Proxy mCarActivityProxy = new CarActivity.Proxy() {
        @Override
        public void setContentView(View view) {
            CarProxyActivity.this.setContentView(view);
        }

        @Override
        public void setContentView(int layoutResID) {
            CarProxyActivity.this.setContentView(layoutResID);
        }

        @Override
        public View findViewById(int id) {
            return CarProxyActivity.this.findViewById(id);
        }

        @Override
        public Resources getResources() {
            return CarProxyActivity.this.getResources();
        }

        @Override
        public void finish() {
            CarProxyActivity.this.finish();
        }

        @Override
        public LayoutInflater getLayoutInflater() {
            return CarProxyActivity.this.getLayoutInflater();
        }

        @Override
        public Intent getIntent() {
            return CarProxyActivity.this.getIntent();
        }

        @Override
        public CarInputManager getCarInputManager() {
            return CarProxyActivity.this.mInputManager;
        }

        @Override
        public void requestPermissions(String[] permissions, int requestCode) {
            CarProxyActivity.this.requestPermissions(permissions, requestCode);
        }

        @Override
        public boolean shouldShowRequestPermissionRationale(String permission) {
            return CarProxyActivity.this.shouldShowRequestPermissionRationale(permission);
        }

        @Override
        public void setIntent(Intent i) {
            CarProxyActivity.this.setIntent(i);
        }

        @Override
        public void setResult(int resultCode) {
            CarProxyActivity.this.setResult(resultCode);
        }

        @Override
        public void setResult(int resultCode, Intent data) {
            CarProxyActivity.this.setResult(resultCode, data);
        }

        @Override
        public MenuInflater getMenuInflater() {
            return CarProxyActivity.this.getMenuInflater();
        }

        @Override
        public void finishAfterTransition() {
            CarProxyActivity.this.finishAfterTransition();
        }

        @Override
        public void startActivityForResult(Intent intent, int requestCode) {
            CarProxyActivity.this.startActivityForResult(intent, requestCode);
        }

        @Override
        public boolean isFinishing() {
            return CarProxyActivity.this.isFinishing();
        }

        @Override
        public Window getWindow() {
            return CarProxyActivity.this.getWindow();
        }
    };

    public CarProxyActivity(Class carActivityClass) {
        this(carActivityClass, false);
    }

    public CarProxyActivity(Class carActivityClass, boolean needCar) {
        mCarActivityClass = carActivityClass;
        mNeedConnectedCar = needCar;
    }

    private void createCarActivity() {
        if (mNeedConnectedCar) {
            mCar = Car.createCar(this, mConnectionListener);
            mCar.connect();
        }
        Constructor<?> ctor;
        try {
            ctor = mCarActivityClass.getDeclaredConstructor(CarActivity.Proxy.class,
                        Context.class, Car.class);
        } catch (NoSuchMethodException e) {
            StringBuilder msg = new StringBuilder(
                    "Cannot construct given CarActivity, no constructor for ");
            msg.append(mCarActivityClass.getName());
            msg.append("\nAvailable constructors are [");
            final Constructor<?>[] others = mCarActivityClass.getConstructors();
            for (int i=0; i<others.length; i++ ) {
                msg.append("\n  ");
                msg.append(others[i].toString());
            }
            msg.append("\n]");
            throw new RuntimeException(msg.toString(), e);
        }
        try {
            mCarActivity = (CarActivity) ctor.newInstance(mCarActivityProxy, this, mCar);
        } catch (InstantiationException | IllegalAccessException | IllegalArgumentException
                | InvocationTargetException e) {
            throw new RuntimeException("Cannot construct given CarActivity, constructor failed for "
                    + mCarActivityClass.getName(), e);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        createCarActivity();
        super.onCreate(savedInstanceState);
        mInputManager = new EmbeddedInputManager(this);
        handleCmd(CarActivity.CMD_ON_CREATE, savedInstanceState);
    }

    @Override
    public View onCreateView(View parent, String name, Context context, AttributeSet attrs) {
        final View view = mCarActivity.onCreateView(parent, name, context, attrs);
        if (view != null) {
            return view;
        }
        return super.onCreateView(parent, name, context, attrs);
    }

    @Override
    protected void onStart() {
        super.onStart();
        handleCmd(CarActivity.CMD_ON_START);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        handleCmd(CarActivity.CMD_ON_ACTIVITY_RESULT, requestCode, resultCode, data);
    }

    @Override
    protected void onRestart() {
        super.onRestart();
        handleCmd(CarActivity.CMD_ON_RESTART);
    }

    @Override
    protected void onResume() {
        super.onResume();
        handleCmd(CarActivity.CMD_ON_RESUME);
    }

    @Override
    protected void onPause() {
        super.onPause();
        handleCmd(CarActivity.CMD_ON_PAUSE);
    }

    @Override
    protected void onStop() {
        super.onStop();
        handleCmd(CarActivity.CMD_ON_STOP);
    }

    @Override
    public Object onRetainNonConfigurationInstance() {
        return mCarActivity.onRetainNonConfigurationInstance();
    }

    @Override
    public void onBackPressed() {
        handleCmd(CarActivity.CMD_ON_BACK_PRESSED);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        handleCmd(CarActivity.CMD_ON_DESTROY);
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        handleCmd(CarActivity.CMD_ON_SAVE_INSTANCE_STATE, outState);
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedState) {
        super.onRestoreInstanceState(savedState);
        handleCmd(CarActivity.CMD_ON_RESTORE_INSTANCE_STATE, savedState);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        handleCmd(CarActivity.CMD_ON_CONFIG_CHANGED, newConfig);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
            int[] results) {
        handleCmd(CarActivity.CMD_ON_REQUEST_PERMISSIONS_RESULT,
                new Integer(requestCode), permissions, convertArray(results));
    }

    @Override
    protected void onNewIntent(Intent i) {
        handleCmd(CarActivity.CMD_ON_NEW_INTENT, i);
    }

    @Override
    public void onLowMemory() {
        super.onLowMemory();
        handleCmd(CarActivity.CMD_ON_LOW_MEMORY);
    }

    private static final class EmbeddedInputManager extends CarInputManager {
        private static final String TAG = "EmbeddedInputManager";

        private final InputMethodManager mInputManager;
        private final WeakReference<CarProxyActivity> mActivity;

        public EmbeddedInputManager(CarProxyActivity activity) {
            mActivity = new WeakReference<CarProxyActivity>(activity);
            mInputManager = (InputMethodManager) mActivity.get()
                    .getSystemService(Context.INPUT_METHOD_SERVICE);
        }

        @Override
        public void startInput(EditText view) {
            view.requestFocus();
            mInputManager.showSoftInput(view, 0);
        }

        @Override
        public void stopInput() {
            if (mActivity.get() == null) {
                return;
            }

            View view = mActivity.get().getCurrentFocus();
            if (view != null) {
                mInputManager.hideSoftInputFromWindow(view.getWindowToken(), 0);
            } else {
                Log.e(TAG, "stopInput called, but no view is accepting input");
            }
        }

        @Override
        public boolean isValid() {
            return mActivity.get() != null;
        }

        @Override
        public boolean isInputActive() {
            return mInputManager.isActive();
        }

        @Override
        public boolean isCurrentCarEditable(EditText view) {
            return mInputManager.isActive(view);
        }
    }

    private void handleCmd(int cmd, Object... args) {
        if (!mNeedConnectedCar || (mCar != null && mCar.isConnected())) {
            mCarActivity.dispatchCmd(cmd, args);
        } else {
            // not connected yet. queue it and return.
            Pair<Integer, Object[]> cmdToQ =
                    new Pair<Integer, Object[]>(Integer.valueOf(cmd), args);
            mCmds.add(cmdToQ);
        }
    }

    private static Integer[] convertArray(int[] array) {
        Integer[] grantResults = new Integer[array.length];
        for (int i = 0; i < array.length; i++) {
            grantResults[i] = array[i];
        }
        return grantResults;
    }
}
