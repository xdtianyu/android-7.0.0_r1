/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.android.wallpaper.livepicker;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.WallpaperManager;
import android.app.WallpaperInfo;
import android.app.Dialog;
import android.content.DialogInterface;
import android.graphics.Rect;
import android.service.wallpaper.IWallpaperConnection;
import android.service.wallpaper.IWallpaperService;
import android.service.wallpaper.IWallpaperEngine;
import android.service.wallpaper.WallpaperSettingsActivity;
import android.content.ServiceConnection;
import android.content.Intent;
import android.content.Context;
import android.content.ComponentName;
import android.os.RemoteException;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.Bundle;
import android.service.wallpaper.WallpaperService;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.ViewGroup;
import android.view.Window;
import android.view.LayoutInflater;
import android.util.Log;
import android.widget.TextView;

import java.io.IOException;

public class LiveWallpaperPreview extends Activity {
    static final String EXTRA_LIVE_WALLPAPER_INFO = "android.live_wallpaper.info";

    private static final String LOG_TAG = "LiveWallpaperPreview";

    private WallpaperManager mWallpaperManager;
    private WallpaperConnection mWallpaperConnection;

    private String mSettings;
    private String mPackageName;
    private Intent mWallpaperIntent;

    private View mView;
    private Dialog mDialog;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        init();
    }

    protected void init() {
        Bundle extras = getIntent().getExtras();
        WallpaperInfo info = extras.getParcelable(EXTRA_LIVE_WALLPAPER_INFO);
        if (info == null) {
            setResult(RESULT_CANCELED);
            finish();
        }

        initUI(info);
    }

    protected void initUI(WallpaperInfo info) {
        mSettings = info.getSettingsActivity();
        mPackageName = info.getPackageName();
        mWallpaperIntent = new Intent(WallpaperService.SERVICE_INTERFACE)
                .setClassName(info.getPackageName(), info.getServiceName());

        final ActionBar actionBar = getActionBar();
        actionBar.setCustomView(R.layout.live_wallpaper_preview);
        mView = actionBar.getCustomView();

        mWallpaperManager = WallpaperManager.getInstance(this);
        mWallpaperConnection = new WallpaperConnection(mWallpaperIntent);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        if (mSettings != null) {
            getMenuInflater().inflate(R.menu.menu_preview, menu);
        }
        return super.onCreateOptionsMenu(menu);
    }

    public void setLiveWallpaper(final View v) {
        if (mWallpaperManager.getWallpaperId(WallpaperManager.FLAG_LOCK) < 0) {
            // The lock screen does not have a wallpaper, so no need to prompt; can only set both.
            try {
                setLiveWallpaper(v.getRootView().getWindowToken());
                setResult(RESULT_OK);
            } catch (RuntimeException e) {
                Log.w(LOG_TAG, "Failure setting wallpaper", e);
            }
            finish();
        } else {
            // Otherwise, prompt to either set on home or both home and lock screen.
            new AlertDialog.Builder(this)
                    .setTitle(R.string.set_live_wallpaper)
                    .setItems(R.array.which_wallpaper_options, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            try {
                                setLiveWallpaper(v.getRootView().getWindowToken());
                                if (which == 1) {
                                    // "Home screen and lock screen"; clear the lock screen so it
                                    // shows through to the live wallpaper on home.
                                    mWallpaperManager.clear(WallpaperManager.FLAG_LOCK);
                                }
                                setResult(RESULT_OK);
                            } catch (RuntimeException e) {
                                Log.w(LOG_TAG, "Failure setting wallpaper", e);
                            } catch (IOException e) {
                                Log.w(LOG_TAG, "Failure setting wallpaper", e);
                            }
                            finish();
                        }
                    })
                    .show();
        }
    }

    private void setLiveWallpaper(IBinder windowToken) {
        mWallpaperManager.setWallpaperComponent(mWallpaperIntent.getComponent());
        mWallpaperManager.setWallpaperOffsetSteps(0.5f, 0.0f);
        mWallpaperManager.setWallpaperOffsets(windowToken, 0.5f, 0.0f);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.configure) {
            Intent intent = new Intent();
            intent.setComponent(new ComponentName(mPackageName, mSettings));
            intent.putExtra(WallpaperSettingsActivity.EXTRA_PREVIEW_MODE, true);
            startActivity(intent);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mWallpaperConnection != null && mWallpaperConnection.mEngine != null) {
            try {
                mWallpaperConnection.mEngine.setVisibility(true);
            } catch (RemoteException e) {
                // Ignore
            }
        }
    }
    
    @Override
    public void onPause() {
        super.onPause();
        if (mWallpaperConnection != null && mWallpaperConnection.mEngine != null) {
            try {
                mWallpaperConnection.mEngine.setVisibility(false);
            } catch (RemoteException e) {
                // Ignore
            }
        }
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        showLoading();

        mView.post(new Runnable() {
            public void run() {
                if (!mWallpaperConnection.connect()) {
                    mWallpaperConnection = null;
                }
            }
        });
    }

    private void showLoading() {
        LayoutInflater inflater = LayoutInflater.from(this);
        TextView content = (TextView) inflater.inflate(R.layout.live_wallpaper_loading, null);

        mDialog = new Dialog(this, android.R.style.Theme_Black);

        Window window = mDialog.getWindow();
        WindowManager.LayoutParams lp = window.getAttributes();

        lp.width = WindowManager.LayoutParams.MATCH_PARENT;
        lp.height = WindowManager.LayoutParams.MATCH_PARENT;
        window.setType(WindowManager.LayoutParams.TYPE_APPLICATION_MEDIA);

        mDialog.setContentView(content, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT
        ));
        mDialog.show();
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        
        if (mDialog != null) mDialog.dismiss();
        
        if (mWallpaperConnection != null) {
            mWallpaperConnection.disconnect();
        }
        mWallpaperConnection = null;
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        if (mWallpaperConnection != null && mWallpaperConnection.mEngine != null) {
            MotionEvent dup = MotionEvent.obtainNoHistory(ev);
            try {
                mWallpaperConnection.mEngine.dispatchPointer(dup);
            } catch (RemoteException e) {
            }
        }
        
        if (ev.getAction() == MotionEvent.ACTION_DOWN) {
            onUserInteraction();
        }
        boolean handled = getWindow().superDispatchTouchEvent(ev);
        if (!handled) {
            handled = onTouchEvent(ev);
        }

        if (!handled && mWallpaperConnection != null && mWallpaperConnection.mEngine != null) {
            int action = ev.getActionMasked();
            try {
                if (action == MotionEvent.ACTION_UP) {
                    mWallpaperConnection.mEngine.dispatchWallpaperCommand(
                            WallpaperManager.COMMAND_TAP,
                            (int) ev.getX(), (int) ev.getY(), 0, null);
                } else if (action == MotionEvent.ACTION_POINTER_UP) {
                    int pointerIndex = ev.getActionIndex();
                    mWallpaperConnection.mEngine.dispatchWallpaperCommand(
                            WallpaperManager.COMMAND_SECONDARY_TAP,
                            (int) ev.getX(pointerIndex), (int) ev.getY(pointerIndex), 0, null);
                }
            } catch (RemoteException e) {
            }
        }
        return handled;
    }
    
    class WallpaperConnection extends IWallpaperConnection.Stub implements ServiceConnection {
        final Intent mIntent;
        IWallpaperService mService;
        IWallpaperEngine mEngine;
        boolean mConnected;

        WallpaperConnection(Intent intent) {
            mIntent = intent;
        }

        public boolean connect() {
            synchronized (this) {
                if (!bindService(mIntent, this, Context.BIND_AUTO_CREATE)) {
                    return false;
                }

                mConnected = true;
                return true;
            }
        }
        
        public void disconnect() {
            synchronized (this) {
                mConnected = false;
                if (mEngine != null) {
                    try {
                        mEngine.destroy();
                    } catch (RemoteException e) {
                        // Ignore
                    }
                    mEngine = null;
                }
                unbindService(this);
                mService = null;
            }
        }
        
        public void onServiceConnected(ComponentName name, IBinder service) {
            if (mWallpaperConnection == this) {
                mService = IWallpaperService.Stub.asInterface(service);
                try {
                    final View view = mView;
                    final View root = view.getRootView();
                    mService.attach(this, view.getWindowToken(),
                            WindowManager.LayoutParams.TYPE_APPLICATION_MEDIA_OVERLAY,
                            true, root.getWidth(), root.getHeight(),
                            new Rect(0, 0, 0, 0));
                } catch (RemoteException e) {
                    Log.w(LOG_TAG, "Failed attaching wallpaper; clearing", e);
                }
            }
        }

        public void onServiceDisconnected(ComponentName name) {
            mService = null;
            mEngine = null;
            if (mWallpaperConnection == this) {
                Log.w(LOG_TAG, "Wallpaper service gone: " + name);
            }
        }
        
        public void attachEngine(IWallpaperEngine engine) {
            synchronized (this) {
                if (mConnected) {
                    mEngine = engine;
                    try {
                        engine.setVisibility(true);
                    } catch (RemoteException e) {
                        // Ignore
                    }
                } else {
                    try {
                        engine.destroy();
                    } catch (RemoteException e) {
                        // Ignore
                    }
                }
            }
        }
        
        public ParcelFileDescriptor setWallpaper(String name) {
            return null;
        }

        @Override
        public void engineShown(IWallpaperEngine engine) throws RemoteException {
        }
    }
}
