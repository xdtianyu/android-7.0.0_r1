/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.services.telephony;

import android.content.Context;

import android.content.Intent;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.os.UserHandle;
import android.provider.Settings;
import android.telephony.ServiceState;

import com.android.internal.os.SomeArgs;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;

/**
 * Helper class that implements special behavior related to emergency calls. Specifically, this
 * class handles the case of the user trying to dial an emergency number while the radio is off
 * (i.e. the device is in airplane mode), by forcibly turning the radio back on, waiting for it to
 * come up, and then retrying the emergency call.
 */
public class EmergencyCallHelper {

    /**
     * Receives the result of the EmergencyCallHelper's attempt to turn on the radio.
     */
    interface Callback {
        void onComplete(boolean isRadioReady);
    }

    // Number of times to retry the call, and time between retry attempts.
    public static final int MAX_NUM_RETRIES = 5;
    public static final long TIME_BETWEEN_RETRIES_MILLIS = 5000;  // msec

    // Handler message codes; see handleMessage()
    private static final int MSG_START_SEQUENCE = 1;
    private static final int MSG_SERVICE_STATE_CHANGED = 2;
    private static final int MSG_RETRY_TIMEOUT = 3;

    private final Context mContext;

    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_START_SEQUENCE:
                    SomeArgs args = (SomeArgs) msg.obj;
                    Phone phone = (Phone) args.arg1;
                    EmergencyCallHelper.Callback callback =
                            (EmergencyCallHelper.Callback) args.arg2;
                    args.recycle();

                    startSequenceInternal(phone, callback);
                    break;
                case MSG_SERVICE_STATE_CHANGED:
                    onServiceStateChanged((ServiceState) ((AsyncResult) msg.obj).result);
                    break;
                case MSG_RETRY_TIMEOUT:
                    onRetryTimeout();
                    break;
                default:
                    Log.wtf(this, "handleMessage: unexpected message: %d.", msg.what);
                    break;
            }
        }
    };


    private Callback mCallback;  // The callback to notify upon completion.
    private Phone mPhone;  // The phone that will attempt to place the call.
    private int mNumRetriesSoFar;

    public EmergencyCallHelper(Context context) {
        Log.d(this, "EmergencyCallHelper constructor.");
        mContext = context;
    }

    /**
     * Starts the "turn on radio" sequence. This is the (single) external API of the
     * EmergencyCallHelper class.
     *
     * This method kicks off the following sequence:
     * - Power on the radio.
     * - Listen for the service state change event telling us the radio has come up.
     * - Retry if we've gone {@link #TIME_BETWEEN_RETRIES_MILLIS} without any response from the
     *   radio.
     * - Finally, clean up any leftover state.
     *
     * This method is safe to call from any thread, since it simply posts a message to the
     * EmergencyCallHelper's handler (thus ensuring that the rest of the sequence is entirely
     * serialized, and runs only on the handler thread.)
     */
    public void startTurnOnRadioSequence(Phone phone, Callback callback) {
        Log.d(this, "startTurnOnRadioSequence");

        SomeArgs args = SomeArgs.obtain();
        args.arg1 = phone;
        args.arg2 = callback;
        mHandler.obtainMessage(MSG_START_SEQUENCE, args).sendToTarget();
    }

    /**
     * Actual implementation of startTurnOnRadioSequence(), guaranteed to run on the handler thread.
     * @see #startTurnOnRadioSequence
     */
    private void startSequenceInternal(Phone phone, Callback callback) {
        Log.d(this, "startSequenceInternal()");

        // First of all, clean up any state left over from a prior emergency call sequence. This
        // ensures that we'll behave sanely if another startTurnOnRadioSequence() comes in while
        // we're already in the middle of the sequence.
        cleanup();

        mPhone = phone;
        mCallback = callback;


        // No need to check the current service state here, since the only reason to invoke this
        // method in the first place is if the radio is powered-off. So just go ahead and turn the
        // radio on.

        powerOnRadio();  // We'll get an onServiceStateChanged() callback
                         // when the radio successfully comes up.

        // Next step: when the SERVICE_STATE_CHANGED event comes in, we'll retry the call; see
        // onServiceStateChanged(). But also, just in case, start a timer to make sure we'll retry
        // the call even if the SERVICE_STATE_CHANGED event never comes in for some reason.
        startRetryTimer();
    }

    /**
     * Handles the SERVICE_STATE_CHANGED event. Normally this event tells us that the radio has
     * finally come up. In that case, it's now safe to actually place the emergency call.
     */
    private void onServiceStateChanged(ServiceState state) {
        Log.d(this, "onServiceStateChanged(), new state = %s.", state);

        // Possible service states:
        // - STATE_IN_SERVICE        // Normal operation
        // - STATE_OUT_OF_SERVICE    // Still searching for an operator to register to,
        //                           // or no radio signal
        // - STATE_EMERGENCY_ONLY    // Phone is locked; only emergency numbers are allowed
        // - STATE_POWER_OFF         // Radio is explicitly powered off (airplane mode)

        if (isOkToCall(state.getState(), mPhone.getState())) {
            // Woo hoo!  It's OK to actually place the call.
            Log.d(this, "onServiceStateChanged: ok to call!");

            onComplete(true);
            cleanup();
        } else {
            // The service state changed, but we're still not ready to call yet. (This probably was
            // the transition from STATE_POWER_OFF to STATE_OUT_OF_SERVICE, which happens
            // immediately after powering-on the radio.)
            //
            // So just keep waiting; we'll probably get to either STATE_IN_SERVICE or
            // STATE_EMERGENCY_ONLY very shortly. (Or even if that doesn't happen, we'll at least do
            // another retry when the RETRY_TIMEOUT event fires.)
            Log.d(this, "onServiceStateChanged: not ready to call yet, keep waiting.");
        }
    }

    private boolean isOkToCall(int serviceState, PhoneConstants.State phoneState) {
        // Once we reach either STATE_IN_SERVICE or STATE_EMERGENCY_ONLY, it's finally OK to place
        // the emergency call.
        return ((phoneState == PhoneConstants.State.OFFHOOK)
                || (serviceState == ServiceState.STATE_IN_SERVICE)
                || (serviceState == ServiceState.STATE_EMERGENCY_ONLY)) ||

                // Allow STATE_OUT_OF_SERVICE if we are at the max number of retries.
                (mNumRetriesSoFar == MAX_NUM_RETRIES &&
                 serviceState == ServiceState.STATE_OUT_OF_SERVICE);
    }

    /**
     * Handles the retry timer expiring.
     */
    private void onRetryTimeout() {
        PhoneConstants.State phoneState = mPhone.getState();
        int serviceState = mPhone.getServiceState().getState();
        Log.d(this, "onRetryTimeout():  phone state = %s, service state = %d, retries = %d.",
               phoneState, serviceState, mNumRetriesSoFar);

        // - If we're actually in a call, we've succeeded.
        // - Otherwise, if the radio is now on, that means we successfully got out of airplane mode
        //   but somehow didn't get the service state change event.  In that case, try to place the
        //   call.
        // - If the radio is still powered off, try powering it on again.

        if (isOkToCall(serviceState, phoneState)) {
            Log.d(this, "onRetryTimeout: Radio is on. Cleaning up.");

            // Woo hoo -- we successfully got out of airplane mode.
            onComplete(true);
            cleanup();
        } else {
            // Uh oh; we've waited the full TIME_BETWEEN_RETRIES_MILLIS and the radio is still not
            // powered-on.  Try again.

            mNumRetriesSoFar++;
            Log.d(this, "mNumRetriesSoFar is now " + mNumRetriesSoFar);

            if (mNumRetriesSoFar > MAX_NUM_RETRIES) {
                Log.w(this, "Hit MAX_NUM_RETRIES; giving up.");
                cleanup();
            } else {
                Log.d(this, "Trying (again) to turn on the radio.");
                powerOnRadio();  // Again, we'll (hopefully) get an onServiceStateChanged() callback
                                 // when the radio successfully comes up.
                startRetryTimer();
            }
        }
    }

    /**
     * Attempt to power on the radio (i.e. take the device out of airplane mode.)
     * Additionally, start listening for service state changes; we'll eventually get an
     * onServiceStateChanged() callback when the radio successfully comes up.
     */
    private void powerOnRadio() {
        Log.d(this, "powerOnRadio().");

        // We're about to turn on the radio, so arrange to be notified when the sequence is
        // complete.
        registerForServiceStateChanged();

        // If airplane mode is on, we turn it off the same way that the Settings activity turns it
        // off.
        if (Settings.Global.getInt(mContext.getContentResolver(),
                                   Settings.Global.AIRPLANE_MODE_ON, 0) > 0) {
            Log.d(this, "==> Turning off airplane mode.");

            // Change the system setting
            Settings.Global.putInt(mContext.getContentResolver(),
                                   Settings.Global.AIRPLANE_MODE_ON, 0);

            // Post the broadcast intend for change in airplane mode
            // TODO: We really should not be in charge of sending this broadcast.
            //     If changing the setting is sufficent to trigger all of the rest of the logic,
            //     then that should also trigger the broadcast intent.
            Intent intent = new Intent(Intent.ACTION_AIRPLANE_MODE_CHANGED);
            intent.putExtra("state", false);
            mContext.sendBroadcastAsUser(intent, UserHandle.ALL);
        } else {
            // Otherwise, for some strange reason the radio is off (even though the Settings
            // database doesn't think we're in airplane mode.)  In this case just turn the radio
            // back on.
            Log.d(this, "==> (Apparently) not in airplane mode; manually powering radio on.");
            mPhone.setRadioPower(true);
        }
    }

    /**
     * Clean up when done with the whole sequence: either after successfully turning on the radio,
     * or after bailing out because of too many failures.
     *
     * The exact cleanup steps are:
     * - Notify callback if we still hadn't sent it a response.
     * - Double-check that we're not still registered for any telephony events
     * - Clean up any extraneous handler messages (like retry timeouts) still in the queue
     *
     * Basically this method guarantees that there will be no more activity from the
     * EmergencyCallHelper until someone kicks off the whole sequence again with another call to
     * {@link #startTurnOnRadioSequence}
     *
     * TODO: Do the work for the comment below:
     * Note we don't call this method simply after a successful call to placeCall(), since it's
     * still possible the call will disconnect very quickly with an OUT_OF_SERVICE error.
     */
    private void cleanup() {
        Log.d(this, "cleanup()");

        // This will send a failure call back if callback has yet to be invoked.  If the callback
        // was already invoked, it's a no-op.
        onComplete(false);

        unregisterForServiceStateChanged();
        cancelRetryTimer();

        // Used for unregisterForServiceStateChanged() so we null it out here instead.
        mPhone = null;
        mNumRetriesSoFar = 0;
    }

    private void startRetryTimer() {
        cancelRetryTimer();
        mHandler.sendEmptyMessageDelayed(MSG_RETRY_TIMEOUT, TIME_BETWEEN_RETRIES_MILLIS);
    }

    private void cancelRetryTimer() {
        mHandler.removeMessages(MSG_RETRY_TIMEOUT);
    }

    private void registerForServiceStateChanged() {
        // Unregister first, just to make sure we never register ourselves twice.  (We need this
        // because Phone.registerForServiceStateChanged() does not prevent multiple registration of
        // the same handler.)
        unregisterForServiceStateChanged();
        mPhone.registerForServiceStateChanged(mHandler, MSG_SERVICE_STATE_CHANGED, null);
    }

    private void unregisterForServiceStateChanged() {
        // This method is safe to call even if we haven't set mPhone yet.
        if (mPhone != null) {
            mPhone.unregisterForServiceStateChanged(mHandler);  // Safe even if unnecessary
        }
        mHandler.removeMessages(MSG_SERVICE_STATE_CHANGED);  // Clean up any pending messages too
    }

    private void onComplete(boolean isRadioReady) {
        if (mCallback != null) {
            Callback tempCallback = mCallback;
            mCallback = null;
            tempCallback.onComplete(isRadioReady);
        }
    }
}
