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

package android.car;

import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteException;

import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

/**
 * CarAppContextManager allows applications to set and listen for the current application context
 * like active navigation or active voice command. Usually only one instance of such application
 * should run in the system, and other app setting the flag for the matching app should
 * lead into other app to stop.
 * @hide
 */
public class CarAppContextManager implements CarManagerBase {
    /**
     * Listener to get notification for app getting information on app context change.
     */
    public interface AppContextChangeListener {
        /**
         * Application context has changed. Note that {@link CarAppContextManager} instance
         * causing the change will not get this notification.
         * @param activeContexts
         */
        void onAppContextChange(int activeContexts);
    }

    /**
     * Listener to get notification for app getting information on app context ownership loss.
     */
    public interface AppContextOwnershipChangeListener {
        /**
         * Lost ownership for the context, which happens when other app has set the context.
         * The app losing context should stop the action associated with the context.
         * For example, navigaiton app currently running active navigation should stop navigation
         * upon getting this for {@link CarAppContextManager#APP_CONTEXT_NAVIGATION}.
         * @param context
         */
        void onAppContextOwnershipLoss(int context);
    }

    /** @hide */
    public static final int APP_CONTEXT_START_FLAG = 0x1;
    /**
     * Flag for active navigation.
     */
    public static final int APP_CONTEXT_NAVIGATION = 0x1;
    /**
     * Flag for active voice command.
     */
    public static final int APP_CONTEXT_VOICE_COMMAND = 0x2;
    /**
     * Update this after adding a new flag.
     * @hide
     */
    public static final int APP_CONTEXT_END_FLAG = 0x2;

    private final IAppContext mService;
    private final Handler mHandler;
    private final IAppContextListenerImpl mBinderListener;
    private final Map<Integer, AppContextOwnershipChangeListener> mOwnershipListeners;

    private AppContextChangeListener mListener;
    private int mContextFilter;

    /**
     * @hide
     */
    CarAppContextManager(IBinder service, Looper looper) {
        mService = IAppContext.Stub.asInterface(service);
        mHandler = new Handler(looper);
        mBinderListener = new IAppContextListenerImpl(this);
        mOwnershipListeners = new HashMap<Integer, AppContextOwnershipChangeListener>();
    }

    /**
     * Register listener to monitor app context change. Only one listener can be registered and
     * registering multiple times will lead into only the last listener to be active.
     * @param listener
     * @param contextFilter Flags of cotexts to get notification.
     * @throws CarNotConnectedException
     */
    public void registerContextListener(AppContextChangeListener listener, int contextFilter)
            throws CarNotConnectedException {
        if (listener == null) {
            throw new IllegalArgumentException("null listener");
        }
        synchronized(this) {
            if (mListener == null || mContextFilter != contextFilter) {
                try {
                    mService.registerContextListener(mBinderListener, contextFilter);
                } catch (RemoteException e) {
                    throw new CarNotConnectedException(e);
                }
            }
            mListener = listener;
            mContextFilter = contextFilter;
        }
    }

    /**
     * Unregister listener and stop listening context change events. If app has owned a context
     * by {@link #setActiveContext(int)}, it will be reset to inactive state.
     * @throws CarNotConnectedException
     */
    public void unregisterContextListener() throws CarNotConnectedException {
        synchronized(this) {
            try {
                mService.unregisterContextListener(mBinderListener);
            } catch (RemoteException e) {
                throw new CarNotConnectedException(e);
            }
            mListener = null;
            mContextFilter = 0;
        }
    }

    public int getActiveAppContexts() throws CarNotConnectedException {
        try {
            return mService.getActiveAppContexts();
        } catch (RemoteException e) {
            throw new CarNotConnectedException(e);
        }
    }

    public boolean isOwningContext(int context) throws CarNotConnectedException {
        try {
            return mService.isOwningContext(mBinderListener, context);
        } catch (RemoteException e) {
            throw new CarNotConnectedException(e);
        }
    }

    /**
     * Set the given contexts as active. By setting this, the application is becoming owner
     * of the context, and will get
     * {@link AppContextOwnershipChangeListener#onAppContextOwnershipLoss(int)}
     * if ownership is given to other app by calling this. Fore-ground app will have higher priority
     * and other app cannot set the same context while owner is in fore-ground.
     * Only one listener per context can be registered and
     * registering multiple times will lead into only the last listener to be active.
     * @param ownershipListener
     * @param contexts
     * @throws CarNotConnectedException
     * @throws SecurityException If owner cannot be changed.
     */
    public void setActiveContexts(AppContextOwnershipChangeListener ownershipListener, int contexts)
            throws SecurityException, CarNotConnectedException {
        if (ownershipListener == null) {
            throw new IllegalArgumentException("null listener");
        }
        synchronized (this) {
            try {
                mService.setActiveContexts(mBinderListener, contexts);
            } catch (RemoteException e) {
                throw new CarNotConnectedException(e);
            }
            for (int flag = APP_CONTEXT_START_FLAG; flag <= APP_CONTEXT_END_FLAG; flag <<= 1) {
                if ((flag & contexts) != 0) {
                    mOwnershipListeners.put(flag, ownershipListener);
                }
            }
        }
    }

    /**
     * Reset the given contexts, i.e. mark them as inactive. This also involves releasing ownership
     * for the context.
     * @param contexts
     * @throws CarNotConnectedException
     */
    public void resetActiveContexts(int contexts) throws CarNotConnectedException {
        try {
            mService.resetActiveContexts(mBinderListener, contexts);
        } catch (RemoteException e) {
            throw new CarNotConnectedException(e);
        }
        synchronized (this) {
            for (int flag = APP_CONTEXT_START_FLAG; flag <= APP_CONTEXT_END_FLAG; flag <<= 1) {
                if ((flag & contexts) != 0) {
                    mOwnershipListeners.remove(flag);
                }
            }
        }
    }

    @Override
    public void onCarDisconnected() {
        // nothing to do
    }

    private void handleAppContextChange(int activeContexts) {
        AppContextChangeListener listener;
        int newContext;
        synchronized (this) {
            if (mListener == null) {
                return;
            }
            listener = mListener;
            newContext = activeContexts & mContextFilter;
        }
        listener.onAppContextChange(newContext);
    }

    private void handleAppContextOwnershipLoss(int context) {
        AppContextOwnershipChangeListener listener;
        synchronized (this) {
            listener = mOwnershipListeners.get(context);
            if (listener == null) {
                return;
            }
        }
        listener.onAppContextOwnershipLoss(context);
    }

    private static class IAppContextListenerImpl extends IAppContextListener.Stub {

        private final WeakReference<CarAppContextManager> mManager;

        private IAppContextListenerImpl(CarAppContextManager manager) {
            mManager = new WeakReference<>(manager);
        }

        @Override
        public void onAppContextChange(final int activeContexts) {
            final CarAppContextManager manager = mManager.get();
            if (manager == null) {
                return;
            }
            manager.mHandler.post(new Runnable() {
                @Override
                public void run() {
                    manager.handleAppContextChange(activeContexts);
                }
            });
        }

        @Override
        public void onAppContextOwnershipLoss(final int context) {
            final CarAppContextManager manager = mManager.get();
            if (manager == null) {
                return;
            }
            manager.mHandler.post(new Runnable() {
                @Override
                public void run() {
                    manager.handleAppContextOwnershipLoss(context);
                }
            });
        }
    }
}
