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

package android.support.car;

/**
 * CarAppContextManager allows applications to set and listen for the current application context
 * like active navigation or active voice command. Usually only one instance of such application
 * should run in the system, and other app setting the flag for the matching app should
 * lead into other app to stop.
 * @hide
 */
public abstract class CarAppContextManager implements CarManagerBase {
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

    /**
     * Register listener to monitor app context change. Only one listener can be registered and
     * registering multiple times will lead into only the last listener to be active.
     * @param listener
     * @param contextFilter Flags of contexts to get notification.
     * @throws CarNotConnectedException
     */
    public abstract void registerContextListener(AppContextChangeListener listener,
            int contextFilter) throws CarNotConnectedException;

    /**
     * Unregister listener and stop listening context change events. If app has owned a context
     * by {@link #setActiveContext(int)}, it will be reset to inactive state.
     * @throws CarNotConnectedException
     */
    public abstract void unregisterContextListener() throws CarNotConnectedException;

    /**
     * Retrieve currently active contexts.
     * @return
     * @throws CarNotConnectedException
     */
    public abstract int getActiveAppContexts() throws CarNotConnectedException;

    /**
     * Check if the current process is owning the given context.
     * @param context
     * @return
     * @throws CarNotConnectedException
     */
    public abstract boolean isOwningContext(int context) throws CarNotConnectedException;

    /**
     * Set the given contexts as active. By setting this, the application is becoming owner
     * of the context, and will get {@link AppContextChangeListener#onAppContextOwnershipLoss(int)}
     * if ownership is given to other app by calling this. Fore-ground app will have higher priority
     * and other app cannot set the same context while owner is in fore-ground.
     * Before calling this, {@link #registerContextListener(AppContextChangeListener, int)} should
     * be called first. Otherwise, it will throw IllegalStateException
     * @param contexts
     * @throws IllegalStateException If listener was not registered.
     * @throws SecurityException If owner cannot be changed.
     * @throws CarNotConnectedException
     */
    public abstract void setActiveContexts(AppContextOwnershipChangeListener ownershipListener,
            int contexts) throws IllegalStateException, SecurityException, CarNotConnectedException;

    /**
     * Reset the given contexts, i.e. mark them as inactive. This also involves releasing ownership
     * for the context.
     * @param contexts
     * @throws CarNotConnectedException
     */
    public abstract void resetActiveContexts(int contexts) throws CarNotConnectedException;
}
