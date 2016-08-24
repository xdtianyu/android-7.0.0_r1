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
package com.android.car;

import android.annotation.NonNull;
import android.car.Car;
import android.content.Context;
import android.hardware.display.DisplayManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.SystemClock;
import android.util.Log;
import android.view.Display;

import com.android.car.hal.PowerHalService;
import com.android.car.hal.PowerHalService.PowerState;
import com.android.car.hal.VehicleHal;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;

import java.io.PrintWriter;
import java.util.LinkedList;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.CopyOnWriteArrayList;

public class CarPowerManagementService implements CarServiceBase,
    PowerHalService.PowerEventListener {

    /**
     * Listener for other services to monitor power events.
     */
    public interface PowerServiceEventListener {
        /**
         * Shutdown is happening
         */
        void onShutdown();

        /**
         * Entering deep sleep.
         */
        void onSleepEntry();

        /**
         * Got out of deep sleep.
         */
        void onSleepExit();
    }

    /**
     * Interface for components requiring processing time before shutting-down or
     * entering sleep, and wake-up after shut-down.
     */
    public interface PowerEventProcessingHandler {
        /**
         * Called before shutdown or sleep entry to allow running some processing. This call
         * should only queue such task in different thread and should return quickly.
         * Blocking inside this call can trigger watchdog timer which can terminate the
         * whole system.
         * @param shuttingDown whether system is shutting down or not (= sleep entry).
         * @return time necessary to run processing in ms. should return 0 if there is no
         *         processing necessary.
         */
        long onPrepareShutdown(boolean shuttingDown);

        /**
         * Called when power state is changed to ON state. Display can be either on or off.
         * @param displayOn
         */
        void onPowerOn(boolean displayOn);

        /**
         * Returns wake up time after system is fully shutdown. Power controller will power on
         * the system after this time. This power on is meant for regular maintenance kind of
         * operation.
         * @return 0 of wake up is not necessary.
         */
        int getWakeupTime();
    }

    /** Interface to abstract all system interaction. Separated for testing. */
    public interface SystemInteface {
        void setDisplayState(boolean on);
        void releaseAllWakeLocks();
        void shutdown();
        void enterDeepSleep(int wakeupTimeSec);
        void switchToPartialWakeLock();
        void switchToFullWakeLock();
        void startDisplayStateMonitoring(CarPowerManagementService service);
        void stopDisplayStateMonitoring();
        boolean isSystemSupportingDeepSleep();
        boolean isWakeupCausedByTimer();
    }

    private final Context mContext;
    private final PowerHalService mHal;
    private final SystemInteface mSystemInterface;
    private final HandlerThread mHandlerThread;
    private final PowerHandler mHandler;

    private final CopyOnWriteArrayList<PowerServiceEventListener> mListeners =
            new CopyOnWriteArrayList<>();
    private final CopyOnWriteArrayList<PowerEventProcessingHandlerWrapper>
            mPowerEventProcessingHandlers = new CopyOnWriteArrayList<>();

    @GuardedBy("this")
    private PowerState mCurrentState;
    @GuardedBy("this")
    private Timer mTimer;
    @GuardedBy("this")
    private long mProcessingStartTime;
    @GuardedBy("this")
    private long mLastSleepEntryTime;
    @GuardedBy("this")
    private final LinkedList<PowerState> mPendingPowerStates = new LinkedList<>();

    private final int SHUTDOWN_POLLING_INTERVAL_MS = 2000;
    private final int SHUTDOWN_EXTEND_MAX_MS = 5000;

    /**
     * Constructor for full functionality.
     */
    public CarPowerManagementService(@NonNull Context context) {
        this(context, VehicleHal.getInstance().getPowerHal(),
                new SystemIntefaceImpl(context));
    }

    /**
     * Constructor for full functionality. Can inject external interfaces
     */
    public CarPowerManagementService(@NonNull Context context, @NonNull PowerHalService powerHal,
            @NonNull SystemInteface systemInterface) {
        mContext = context;
        mHal = powerHal;
        mSystemInterface = systemInterface;
        mHandlerThread = new HandlerThread(CarLog.TAG_POWER);
        mHandlerThread.start();
        mHandler = new PowerHandler(mHandlerThread.getLooper());
    }

    /**
     * Create a dummy instance for unit testing purpose only. Instance constructed in this way
     * is not safe as members expected to be non-null are null.
     */
    @VisibleForTesting
    protected CarPowerManagementService() {
        mContext = null;
        mHal = null;
        mSystemInterface = null;
        mHandlerThread = null;
        mHandler = new PowerHandler(Looper.getMainLooper());
    }

    @Override
    public void init() {
        mHal.setListener(this);
        if (mHal.isPowerStateSupported()) {
            mHal.sendBootComplete();
            PowerState currentState = mHal.getCurrentPowerState();
            onApPowerStateChange(currentState);
        } else {
            Log.w(CarLog.TAG_POWER, "Vehicle hal does not support power state yet.");
            mSystemInterface.switchToFullWakeLock();
        }
        mSystemInterface.startDisplayStateMonitoring(this);
    }

    @Override
    public void release() {
        synchronized (this) {
            releaseTimerLocked();
            mCurrentState = null;
        }
        mSystemInterface.stopDisplayStateMonitoring();
        mHandler.cancelAll();
        mListeners.clear();
        mPowerEventProcessingHandlers.clear();
        mSystemInterface.releaseAllWakeLocks();
    }

    /**
     * Register listener to monitor power event. There is no unregister counter-part and the list
     * will be cleared when the service is released.
     * @param listener
     */
    public synchronized void registerPowerEventListener(PowerServiceEventListener listener) {
        mListeners.add(listener);
    }

    /**
     * Register PowerEventPreprocessingHandler to run pre-processing before shutdown or
     * sleep entry. There is no unregister counter-part and the list
     * will be cleared when the service is released.
     * @param handler
     */
    public synchronized void registerPowerEventProcessingHandler(
            PowerEventProcessingHandler handler) {
        mPowerEventProcessingHandlers.add(new PowerEventProcessingHandlerWrapper(handler));
        // onPowerOn will not be called if power on notification is already done inside the
        // handler thread. So request it once again here. Wrapper will have its own
        // gatekeeping to prevent calling onPowerOn twice.
        mHandler.handlePowerOn();
    }

    /**
     * Notifies earlier completion of power event processing. PowerEventProcessingHandler quotes
     * time necessary from onPrePowerEvent() call, but actual processing can finish earlier than
     * that, and this call can be called in such case to trigger shutdown without waiting further.
     *
     * @param handler PowerEventProcessingHandler that was already registered with
     *        {@link #registerPowerEventListener(PowerServiceEventListener)} call. If it was not
     *        registered before, this call will be ignored.
     */
    public void notifyPowerEventProcessingCompletion(PowerEventProcessingHandler handler) {
        long processingTime = 0;
        for (PowerEventProcessingHandlerWrapper wrapper : mPowerEventProcessingHandlers) {
            if (wrapper.handler == handler) {
                wrapper.markProcessingDone();
            } else if (!wrapper.isProcessingDone()) {
                processingTime = Math.max(processingTime, wrapper.getProcessingTime());
            }
        }
        long now = SystemClock.elapsedRealtime();
        long startTime;
        boolean shouldShutdown = true;
        synchronized (this) {
            startTime = mProcessingStartTime;
            if (mCurrentState == null) {
                return;
            }
            if (mCurrentState.state != PowerHalService.STATE_SHUTDOWN_PREPARE) {
                return;
            }
            if (mCurrentState.canEnterDeepSleep()) {
                shouldShutdown = false;
                if (mLastSleepEntryTime > mProcessingStartTime && mLastSleepEntryTime < now) {
                    // already slept
                    return;
                }
            }
        }
        if ((startTime + processingTime) <= now) {
            Log.i(CarLog.TAG_POWER, "Processing all done");
            mHandler.handleProcessingComplete(shouldShutdown);
        }
    }

    @Override
    public void dump(PrintWriter writer) {
        writer.println("*PowerManagementService*");
        writer.print("mCurrentState:" + mCurrentState);
        writer.print(",mProcessingStartTime:" + mProcessingStartTime);
        writer.println(",mLastSleepEntryTime:" + mLastSleepEntryTime);
        writer.println("**PowerEventProcessingHandlers");
        for (PowerEventProcessingHandlerWrapper wrapper : mPowerEventProcessingHandlers) {
            writer.println(wrapper.toString());
        }
    }

    @Override
    public void onApPowerStateChange(PowerState state) {
        synchronized (this) {
            mPendingPowerStates.addFirst(state);
        }
        mHandler.handlePowerStateChange();
    }

    private void doHandlePowerStateChange() {
        PowerState state = null;
        synchronized (this) {
            state = mPendingPowerStates.peekFirst();
            mPendingPowerStates.clear();
            if (state == null) {
                return;
            }
            if (!needPowerStateChange(state)) {
                return;
            }
            // now real power change happens. Whatever was queued before should be all cancelled.
            releaseTimerLocked();
            mHandler.cancelProcessingComplete();
        }

        Log.i(CarLog.TAG_POWER, "Power state change:" + state);
        switch (state.state) {
            case PowerHalService.STATE_ON_DISP_OFF:
                handleDisplayOff(state);
                notifyPowerOn(false);
                break;
            case PowerHalService.STATE_ON_FULL:
                handleFullOn(state);
                notifyPowerOn(true);
                break;
            case PowerHalService.STATE_SHUTDOWN_PREPARE:
                handleShutdownPrepare(state);
                break;
        }
    }

    private void handleDisplayOff(PowerState newState) {
        setCurrentState(newState);
        mSystemInterface.setDisplayState(false);
    }

    private void handleFullOn(PowerState newState) {
        setCurrentState(newState);
        mSystemInterface.setDisplayState(true);
    }

    @VisibleForTesting
    protected void notifyPowerOn(boolean displayOn) {
        for (PowerEventProcessingHandlerWrapper wrapper : mPowerEventProcessingHandlers) {
            wrapper.callOnPowerOn(displayOn);
        }
    }

    @VisibleForTesting
    protected long notifyPrepareShutdown(boolean shuttingDown) {
        long processingTimeMs = 0;
        for (PowerEventProcessingHandlerWrapper wrapper : mPowerEventProcessingHandlers) {
            long handlerProcessingTime = wrapper.handler.onPrepareShutdown(shuttingDown);
            if (handlerProcessingTime > processingTimeMs) {
                processingTimeMs = handlerProcessingTime;
            }
        }
        return processingTimeMs;
    }

    private void handleShutdownPrepare(PowerState newState) {
        setCurrentState(newState);
        mSystemInterface.setDisplayState(false);;
        boolean shouldShutdown = true;
        if (mHal.isDeepSleepAllowed() && mSystemInterface.isSystemSupportingDeepSleep() &&
                newState.canEnterDeepSleep()) {
            Log.i(CarLog.TAG_POWER, "starting sleep");
            shouldShutdown = false;
            doHandlePreprocessing(shouldShutdown);
            return;
        } else if (newState.canPostponeShutdown()) {
            Log.i(CarLog.TAG_POWER, "starting shutdown with processing");
            doHandlePreprocessing(shouldShutdown);
        } else {
            Log.i(CarLog.TAG_POWER, "starting shutdown immediately");
            synchronized (this) {
                releaseTimerLocked();
            }
            doHandleShutdown();
        }
    }

    private void releaseTimerLocked() {
        if (mTimer != null) {
            mTimer.cancel();
        }
        mTimer = null;
    }

    private void doHandlePreprocessing(boolean shuttingDown) {
        long processingTimeMs = 0;
        for (PowerEventProcessingHandlerWrapper wrapper : mPowerEventProcessingHandlers) {
            long handlerProcessingTime = wrapper.handler.onPrepareShutdown(shuttingDown);
            if (handlerProcessingTime > 0) {
                wrapper.setProcessingTimeAndResetProcessingDone(handlerProcessingTime);
            }
            if (handlerProcessingTime > processingTimeMs) {
                processingTimeMs = handlerProcessingTime;
            }
        }
        if (processingTimeMs > 0) {
            int pollingCount = (int)(processingTimeMs / SHUTDOWN_POLLING_INTERVAL_MS) + 1;
            Log.i(CarLog.TAG_POWER, "processing before shutdown expected for :" + processingTimeMs +
                    " ms, adding polling:" + pollingCount);
            synchronized (this) {
                mProcessingStartTime = SystemClock.elapsedRealtime();
                releaseTimerLocked();
                mTimer = new Timer();
                mTimer.scheduleAtFixedRate(new ShutdownProcessingTimerTask(shuttingDown,
                        pollingCount),
                        0 /*delay*/,
                        SHUTDOWN_POLLING_INTERVAL_MS);
            }
        } else {
            mHandler.handleProcessingComplete(shuttingDown);
        }
    }

    private void doHandleDeepSleep() {
        mHandler.cancelProcessingComplete();
        for (PowerServiceEventListener listener : mListeners) {
            listener.onSleepEntry();
        }
        int wakeupTimeSec = getWakeupTime();
        for (PowerEventProcessingHandlerWrapper wrapper : mPowerEventProcessingHandlers) {
            wrapper.resetPowerOnSent();
        }
        mHal.sendSleepEntry();
        synchronized (this) {
            mLastSleepEntryTime = SystemClock.elapsedRealtime();
        }
        if (!shouldDoFakeShutdown()) { // if it is mocked, do not enter sleep.
            mSystemInterface.enterDeepSleep(wakeupTimeSec);
        }
        mSystemInterface.releaseAllWakeLocks();
        mSystemInterface.switchToPartialWakeLock();
        mHal.sendSleepExit();
        for (PowerServiceEventListener listener : mListeners) {
            listener.onSleepExit();
        }
        if (mSystemInterface.isWakeupCausedByTimer()) {
            doHandlePreprocessing(false /*shuttingDown*/);
        } else {
            PowerState currentState = mHal.getCurrentPowerState();
            if (needPowerStateChange(currentState)) {
                onApPowerStateChange(currentState);
            } else { // power controller woke-up but no power state change. Just shutdown.
                Log.w(CarLog.TAG_POWER, "external sleep wake up, but no power state change:" +
                        currentState);
                doHandleShutdown();
            }
        }
    }

    private void doHandleNotifyPowerOn() {
        boolean displayOn = false;
        synchronized (this) {
            if (mCurrentState != null && mCurrentState.state == PowerHalService.SET_DISPLAY_ON) {
                displayOn = true;
            }
        }
        for (PowerEventProcessingHandlerWrapper wrapper : mPowerEventProcessingHandlers) {
            // wrapper will not send it forward if it is already called.
            wrapper.callOnPowerOn(displayOn);
        }
    }

    private boolean needPowerStateChange(PowerState newState) {
        synchronized (this) {
            if (mCurrentState != null && mCurrentState.equals(newState)) {
                return false;
            }
            return true;
        }
    }

    private void doHandleShutdown() {
        // now shutdown
        for (PowerServiceEventListener listener : mListeners) {
            listener.onShutdown();
        }
        int wakeupTimeSec = 0;
        if (mHal.isTimedWakeupAllowed()) {
            wakeupTimeSec = getWakeupTime();
        }
        mHal.sendShutdownStart(wakeupTimeSec);
        if (!shouldDoFakeShutdown()) {
            mSystemInterface.shutdown();
        }
    }

    private int getWakeupTime() {
        int wakeupTimeSec = 0;
        for (PowerEventProcessingHandlerWrapper wrapper : mPowerEventProcessingHandlers) {
            int t = wrapper.handler.getWakeupTime();
            if (t > wakeupTimeSec) {
                wakeupTimeSec = t;
            }
        }
        return wakeupTimeSec;
    }

    private void doHandleProcessingComplete(boolean shutdownWhenCompleted) {
        synchronized (this) {
            releaseTimerLocked();
            if (!shutdownWhenCompleted && mLastSleepEntryTime > mProcessingStartTime) {
                // entered sleep after processing start. So this could be duplicate request.
                Log.w(CarLog.TAG_POWER, "Duplicate sleep entry request, ignore");
                return;
            }
        }
        if (shutdownWhenCompleted) {
            doHandleShutdown();
        } else {
            doHandleDeepSleep();
        }
    }

    private synchronized void setCurrentState(PowerState state) {
        mCurrentState = state;
    }

    @Override
    public void onDisplayBrightnessChange(int brightness) {
        // TODO
    }

    private void doHandleDisplayBrightnessChange(int brightness) {
        //TODO
    }

    private void doHandleMainDisplayStateChange(boolean on) {
        //TODO
    }

    private boolean shouldDoFakeShutdown() {
        ICarImpl carImpl = ICarImpl.getInstance(mContext);
        if (!carImpl.isInMocking()) {
            return false;
        }
        CarTestService testService = (CarTestService) carImpl.getCarService(Car.TEST_SERVICE);
        return !testService.shouldDoRealShutdownInMocking();
    }

    public void handleMainDisplayChanged(boolean on) {
        mHandler.handleMainDisplayStateChange(on);
    }

    public Handler getHandler() {
        return mHandler;
    }

    private class PowerHandler extends Handler {

        private final int MSG_POWER_STATE_CHANGE = 0;
        private final int MSG_DISPLAY_BRIGHTNESS_CHANGE = 1;
        private final int MSG_MAIN_DISPLAY_STATE_CHANGE = 2;
        private final int MSG_PROCESSING_COMPLETE = 3;
        private final int MSG_NOTIFY_POWER_ON = 4;

        // Do not handle this immediately but with some delay as there can be a race between
        // display off due to rear view camera and delivery to here.
        private final long MAIN_DISPLAY_EVENT_DELAY_MS = 500;

        private PowerHandler(Looper looper) {
            super(looper);
        }

        private void handlePowerStateChange() {
            Message msg = obtainMessage(MSG_POWER_STATE_CHANGE);
            sendMessage(msg);
        }

        private void handleDisplayBrightnessChange(int brightness) {
            Message msg = obtainMessage(MSG_DISPLAY_BRIGHTNESS_CHANGE, brightness, 0);
            sendMessage(msg);
        }

        private void handleMainDisplayStateChange(boolean on) {
            removeMessages(MSG_MAIN_DISPLAY_STATE_CHANGE);
            Message msg = obtainMessage(MSG_MAIN_DISPLAY_STATE_CHANGE, Boolean.valueOf(on));
            sendMessageDelayed(msg, MAIN_DISPLAY_EVENT_DELAY_MS);
        }

        private void handleProcessingComplete(boolean shutdownWhenCompleted) {
            removeMessages(MSG_PROCESSING_COMPLETE);
            Message msg = obtainMessage(MSG_PROCESSING_COMPLETE, shutdownWhenCompleted ? 1 : 0, 0);
            sendMessage(msg);
        }

        private void handlePowerOn() {
            Message msg = obtainMessage(MSG_NOTIFY_POWER_ON);
            sendMessage(msg);
        }

        private void cancelProcessingComplete() {
            removeMessages(MSG_PROCESSING_COMPLETE);
        }

        private void cancelAll() {
            removeMessages(MSG_POWER_STATE_CHANGE);
            removeMessages(MSG_DISPLAY_BRIGHTNESS_CHANGE);
            removeMessages(MSG_MAIN_DISPLAY_STATE_CHANGE);
            removeMessages(MSG_PROCESSING_COMPLETE);
            removeMessages(MSG_NOTIFY_POWER_ON);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_POWER_STATE_CHANGE:
                    doHandlePowerStateChange();
                    break;
                case MSG_DISPLAY_BRIGHTNESS_CHANGE:
                    doHandleDisplayBrightnessChange(msg.arg1);
                    break;
                case MSG_MAIN_DISPLAY_STATE_CHANGE:
                    doHandleMainDisplayStateChange((Boolean) msg.obj);
                    break;
                case MSG_PROCESSING_COMPLETE:
                    doHandleProcessingComplete(msg.arg1 == 1);
                    break;
                case MSG_NOTIFY_POWER_ON:
                    doHandleNotifyPowerOn();
                    break;
            }
        }
    }

    private static class SystemIntefaceImpl implements SystemInteface {

        private final PowerManager mPowerManager;
        private final DisplayManager mDisplayManager;
        private final WakeLock mFullWakeLock;
        private final WakeLock mPartialWakeLock;
        private final DisplayStateListener mDisplayListener;
        private CarPowerManagementService mService;
        private boolean mDisplayStateSet;

        private SystemIntefaceImpl(Context context) {
            mPowerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
            mDisplayManager = (DisplayManager) context.getSystemService(Context.DISPLAY_SERVICE);
            mFullWakeLock = mPowerManager.newWakeLock(PowerManager.FULL_WAKE_LOCK, CarLog.TAG_POWER);
            mPartialWakeLock = mPowerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
                    CarLog.TAG_POWER);
            mDisplayListener = new DisplayStateListener();
        }

        @Override
        public void startDisplayStateMonitoring(CarPowerManagementService service) {
            synchronized (this) {
                mService = service;
                mDisplayStateSet = isMainDisplayOn();
            }
            mDisplayManager.registerDisplayListener(mDisplayListener, service.getHandler());
        }

        @Override
        public void stopDisplayStateMonitoring() {
            mDisplayManager.unregisterDisplayListener(mDisplayListener);
        }

        @Override
        public void setDisplayState(boolean on) {
            synchronized (this) {
                mDisplayStateSet = on;
            }
            if (on) {
                switchToFullWakeLock();
                Log.i(CarLog.TAG_POWER, "on display");
                mPowerManager.wakeUp(SystemClock.uptimeMillis());
            } else {
                switchToPartialWakeLock();
                Log.i(CarLog.TAG_POWER, "off display");
                mPowerManager.goToSleep(SystemClock.uptimeMillis());
            }
        }

        private boolean isMainDisplayOn() {
            Display disp = mDisplayManager.getDisplay(Display.DEFAULT_DISPLAY);
            return disp.getState() == Display.STATE_ON;
        }

        @Override
        public void shutdown() {
            mPowerManager.shutdown(false /* no confirm*/, null, true /* true */);
        }

        @Override
        public void enterDeepSleep(int wakeupTimeSec) {
            //TODO
        }

        @Override
        public boolean isSystemSupportingDeepSleep() {
            //TODO should return by checking some kernel suspend control sysfs
            return false;
        }

        @Override
        public void switchToPartialWakeLock() {
            if (!mPartialWakeLock.isHeld()) {
                mPartialWakeLock.acquire();
            }
            if (mFullWakeLock.isHeld()) {
                mFullWakeLock.release();
            }
        }

        @Override
        public void switchToFullWakeLock() {
            if (!mFullWakeLock.isHeld()) {
                mFullWakeLock.acquire();
            }
            if (mPartialWakeLock.isHeld()) {
                mPartialWakeLock.release();
            }
        }

        @Override
        public void releaseAllWakeLocks() {
            if (mPartialWakeLock.isHeld()) {
                mPartialWakeLock.release();
            }
            if (mFullWakeLock.isHeld()) {
                mFullWakeLock.release();
            }
        }

        @Override
        public boolean isWakeupCausedByTimer() {
            //TODO check wake up reason and do necessary operation information should come from
            // kernel. it can be either power on or wake up for maintenance
            // power on will involve GPIO trigger from power controller
            // its own wakeup will involve timer expiration.
            return false;
        }

        private void handleMainDisplayChanged() {
            boolean isOn = isMainDisplayOn();
            CarPowerManagementService service;
            synchronized (this) {
                if (mDisplayStateSet == isOn) { // same as what is set
                    return;
                }
                service = mService;
            }
            service.handleMainDisplayChanged(isOn);
        }

        private class DisplayStateListener implements DisplayManager.DisplayListener {

            @Override
            public void onDisplayAdded(int displayId) {
                //ignore
            }

            @Override
            public void onDisplayChanged(int displayId) {
                if (displayId == Display.DEFAULT_DISPLAY) {
                    handleMainDisplayChanged();
                }
            }

            @Override
            public void onDisplayRemoved(int displayId) {
                //ignore
            }
        }
    }

    private class ShutdownProcessingTimerTask extends TimerTask {
        private final boolean mShutdownWhenCompleted;
        private final int mExpirationCount;
        private int mCurrentCount;

        private ShutdownProcessingTimerTask(boolean shutdownWhenCompleted, int expirationCount) {
            mShutdownWhenCompleted = shutdownWhenCompleted;
            mExpirationCount = expirationCount;
            mCurrentCount = 0;
        }

        @Override
        public void run() {
            mCurrentCount++;
            if (mCurrentCount > mExpirationCount) {
                synchronized (CarPowerManagementService.this) {
                    releaseTimerLocked();
                }
                mHandler.handleProcessingComplete(mShutdownWhenCompleted);
            } else {
                mHal.sendShutdownPostpone(SHUTDOWN_EXTEND_MAX_MS);
            }
        }
    }

    private static class PowerEventProcessingHandlerWrapper {
        public final PowerEventProcessingHandler handler;
        private long mProcessingTime = 0;
        private boolean mProcessingDone = true;
        private boolean mPowerOnSent = false;

        public PowerEventProcessingHandlerWrapper(PowerEventProcessingHandler handler) {
            this.handler = handler;
        }

        public synchronized void setProcessingTimeAndResetProcessingDone(long processingTime) {
            mProcessingTime = processingTime;
            mProcessingDone = false;
        }

        public synchronized long getProcessingTime() {
            return mProcessingTime;
        }

        public synchronized void markProcessingDone() {
            mProcessingDone = true;
        }

        public synchronized boolean isProcessingDone() {
            return mProcessingDone;
        }

        public void callOnPowerOn(boolean displayOn) {
            boolean shouldCall = false;
            synchronized (this) {
                if (!mPowerOnSent) {
                    shouldCall = true;
                    mPowerOnSent = true;
                }
            }
            if (shouldCall) {
                handler.onPowerOn(displayOn);
            }
        }

        public synchronized void resetPowerOnSent() {
            mPowerOnSent = false;
        }

        @Override
        public String toString() {
            return "PowerEventProcessingHandlerWrapper [handler=" + handler + ", mProcessingTime="
                    + mProcessingTime + ", mProcessingDone=" + mProcessingDone + "]";
        }
    }
}
