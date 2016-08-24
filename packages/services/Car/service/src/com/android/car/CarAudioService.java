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

import android.car.media.CarAudioManager;
import android.car.media.ICarAudio;
import android.content.Context;
import android.media.AudioAttributes;
import android.media.AudioFocusInfo;
import android.media.AudioManager;
import android.media.audiopolicy.AudioPolicy;
import android.media.audiopolicy.AudioPolicy.AudioPolicyFocusListener;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

import com.android.car.hal.AudioHalService;
import com.android.car.hal.AudioHalService.AudioHalListener;
import com.android.car.hal.VehicleHal;
import com.android.internal.annotations.GuardedBy;

import java.io.PrintWriter;
import java.util.LinkedList;


public class CarAudioService extends ICarAudio.Stub implements CarServiceBase, AudioHalListener {

    private static final long FOCUS_RESPONSE_WAIT_TIMEOUT_MS = 1000;

    private static final String TAG_FOCUS = CarLog.TAG_AUDIO + ".FOCUS";

    private static final boolean DBG = true;

    private final AudioHalService mAudioHal;
    private final Context mContext;
    private final HandlerThread mFocusHandlerThread;
    private final CarAudioFocusChangeHandler mFocusHandler;
    private final CarAudioVolumeHandler mVolumeHandler;
    private final SystemFocusListener mSystemFocusListener;
    private AudioPolicy mAudioPolicy;
    private final Object mLock = new Object();
    @GuardedBy("mLock")
    private FocusState mCurrentFocusState = FocusState.STATE_LOSS;
    /** Focus state received, but not handled yet. Once handled, this will be set to null. */
    @GuardedBy("mLock")
    private FocusState mFocusReceived = null;
    @GuardedBy("mLock")
    private FocusRequest mLastFocusRequestToCar = null;
    @GuardedBy("mLock")
    private LinkedList<AudioFocusInfo> mPendingFocusChanges = new LinkedList<>();
    @GuardedBy("mLock")
    private AudioFocusInfo mTopFocusInfo = null;
    /** previous top which may be in ducking state */
    @GuardedBy("mLock")
    private AudioFocusInfo mSecondFocusInfo = null;

    private AudioRoutingPolicy mAudioRoutingPolicy;
    private final AudioManager mAudioManager;
    private final BottomAudioFocusListener mBottomAudioFocusHandler =
            new BottomAudioFocusListener();
    private final CarProxyAndroidFocusListener mCarProxyAudioFocusHandler =
            new CarProxyAndroidFocusListener();
    @GuardedBy("mLock")
    private int mBottomFocusState;
    @GuardedBy("mLock")
    private boolean mRadioActive = false;
    @GuardedBy("mLock")
    private boolean mCallActive = false;
    @GuardedBy("mLock")
    private int mCurrentAudioContexts = 0;

    private final AudioAttributes mAttributeBottom =
            CarAudioAttributesUtil.getAudioAttributesForCarUsage(
                    CarAudioAttributesUtil.CAR_AUDIO_USAGE_CARSERVICE_BOTTOM);
    private final AudioAttributes mAttributeCarExternal =
            CarAudioAttributesUtil.getAudioAttributesForCarUsage(
                    CarAudioAttributesUtil.CAR_AUDIO_USAGE_CARSERVICE_CAR_PROXY);

    public CarAudioService(Context context) {
        mAudioHal = VehicleHal.getInstance().getAudioHal();
        mContext = context;
        mFocusHandlerThread = new HandlerThread(CarLog.TAG_AUDIO);
        mSystemFocusListener = new SystemFocusListener();
        mFocusHandlerThread.start();
        mFocusHandler = new CarAudioFocusChangeHandler(mFocusHandlerThread.getLooper());
        mVolumeHandler = new CarAudioVolumeHandler(Looper.getMainLooper());
        mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
    }

    @Override
    public AudioAttributes getAudioAttributesForCarUsage(int carUsage) {
        return CarAudioAttributesUtil.getAudioAttributesForCarUsage(carUsage);
    }

    @Override
    public void init() {
        AudioPolicy.Builder builder = new AudioPolicy.Builder(mContext);
        builder.setLooper(Looper.getMainLooper());
        boolean isFocusSuported = mAudioHal.isFocusSupported();
        if (isFocusSuported) {
            builder.setAudioPolicyFocusListener(mSystemFocusListener);
        }
        mAudioPolicy = builder.build();
        if (isFocusSuported) {
            FocusState currentState = FocusState.create(mAudioHal.getCurrentFocusState());
            int r = mAudioManager.requestAudioFocus(mBottomAudioFocusHandler, mAttributeBottom,
                    AudioManager.AUDIOFOCUS_GAIN, AudioManager.AUDIOFOCUS_FLAG_DELAY_OK);
            synchronized (mLock) {
                if (r == AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
                    mBottomFocusState = AudioManager.AUDIOFOCUS_GAIN;
                } else {
                    mBottomFocusState = AudioManager.AUDIOFOCUS_LOSS_TRANSIENT;
                }
                mCurrentFocusState = currentState;
                mCurrentAudioContexts = 0;
            }
        }
        int r = mAudioManager.registerAudioPolicy(mAudioPolicy);
        if (r != 0) {
            throw new RuntimeException("registerAudioPolicy failed " + r);
        }
        mAudioHal.setListener(this);
        int audioHwVariant = mAudioHal.getHwVariant();
        mAudioRoutingPolicy = AudioRoutingPolicy.create(mContext, audioHwVariant);
        mAudioHal.setAudioRoutingPolicy(mAudioRoutingPolicy);
        //TODO set routing policy with new AudioPolicy API. This will control which logical stream
        //     goes to which physical stream.
    }

    @Override
    public void release() {
        mAudioManager.unregisterAudioPolicyAsync(mAudioPolicy);
        mAudioManager.abandonAudioFocus(mBottomAudioFocusHandler);
        mAudioManager.abandonAudioFocus(mCarProxyAudioFocusHandler);
        mFocusHandler.cancelAll();
        synchronized (mLock) {
            mCurrentFocusState = FocusState.STATE_LOSS;
            mLastFocusRequestToCar = null;
            mTopFocusInfo = null;
            mPendingFocusChanges.clear();
            mRadioActive = false;
        }
    }

    @Override
    public void dump(PrintWriter writer) {
        writer.println("*CarAudioService*");
        writer.println(" mCurrentFocusState:" + mCurrentFocusState +
                " mLastFocusRequestToCar:" + mLastFocusRequestToCar);
        writer.println(" mCurrentAudioContexts:0x" + Integer.toHexString(mCurrentAudioContexts));
        writer.println(" mCallActive:" + mCallActive + " mRadioActive:" + mRadioActive);
        mAudioRoutingPolicy.dump(writer);
    }

    @Override
    public void onFocusChange(int focusState, int streams, int externalFocus) {
        synchronized (mLock) {
            mFocusReceived = FocusState.create(focusState, streams, externalFocus);
            // wake up thread waiting for focus response.
            mLock.notifyAll();
        }
        mFocusHandler.handleFocusChange();
    }

    @Override
    public void onVolumeChange(int streamNumber, int volume, int volumeState) {
        mVolumeHandler.handleVolumeChange(new VolumeStateChangeEvent(streamNumber, volume,
                volumeState));
    }

    @Override
    public void onVolumeLimitChange(int streamNumber, int volume) {
        //TODO
    }

    @Override
    public void onStreamStatusChange(int state, int streamNumber) {
        mFocusHandler.handleStreamStateChange(state, streamNumber);
    }

    private void doHandleCarFocusChange() {
        int newFocusState = AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_INVALID;
        FocusState currentState;
        AudioFocusInfo topInfo;
        synchronized (mLock) {
            if (mFocusReceived == null) {
                // already handled
                return;
            }
            if (mFocusReceived.equals(mCurrentFocusState)) {
                // no change
                mFocusReceived = null;
                return;
            }
            if (DBG) {
                Log.d(TAG_FOCUS, "focus change from car:" + mFocusReceived);
            }
            topInfo = mTopFocusInfo;
            if (!mFocusReceived.equals(mCurrentFocusState.focusState)) {
                newFocusState = mFocusReceived.focusState;
            }
            mCurrentFocusState = mFocusReceived;
            currentState = mFocusReceived;
            mFocusReceived = null;
            if (mLastFocusRequestToCar != null &&
                    (mLastFocusRequestToCar.focusRequest ==
                        AudioHalService.VEHICLE_AUDIO_FOCUS_REQUEST_GAIN ||
                    mLastFocusRequestToCar.focusRequest ==
                        AudioHalService.VEHICLE_AUDIO_FOCUS_REQUEST_GAIN_TRANSIENT ||
                    mLastFocusRequestToCar.focusRequest ==
                        AudioHalService.VEHICLE_AUDIO_FOCUS_REQUEST_GAIN_TRANSIENT_MAY_DUCK) &&
                    (mCurrentFocusState.streams & mLastFocusRequestToCar.streams) !=
                    mLastFocusRequestToCar.streams) {
                Log.w(TAG_FOCUS, "streams mismatch, requested:0x" + Integer.toHexString(
                        mLastFocusRequestToCar.streams) + " got:0x" +
                        Integer.toHexString(mCurrentFocusState.streams));
                // treat it as focus loss as requested streams are not there.
                newFocusState = AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_LOSS;
            }
            mLastFocusRequestToCar = null;
            if (mRadioActive &&
                    (mCurrentFocusState.externalFocus &
                    AudioHalService.VEHICLE_AUDIO_EXT_FOCUS_CAR_PLAY_ONLY_FLAG) == 0) {
                // radio flag dropped
                newFocusState = AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_LOSS;
                mRadioActive = false;
            }
        }
        switch (newFocusState) {
            case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_GAIN:
                doHandleFocusGainFromCar(currentState, topInfo);
                break;
            case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_GAIN_TRANSIENT:
                doHandleFocusGainTransientFromCar(currentState, topInfo);
                break;
            case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_LOSS:
                doHandleFocusLossFromCar(currentState, topInfo);
                break;
            case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_LOSS_TRANSIENT:
                doHandleFocusLossTransientFromCar(currentState);
                break;
            case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_LOSS_TRANSIENT_CAN_DUCK:
                doHandleFocusLossTransientCanDuckFromCar(currentState);
                break;
            case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_LOSS_TRANSIENT_EXLCUSIVE:
                doHandleFocusLossTransientExclusiveFromCar(currentState);
                break;
        }
    }

    private void doHandleFocusGainFromCar(FocusState currentState, AudioFocusInfo topInfo) {
        if (isFocusFromCarServiceBottom(topInfo)) {
            Log.w(TAG_FOCUS, "focus gain from car:" + currentState +
                    " while bottom listener is top");
            mFocusHandler.handleFocusReleaseRequest();
        } else {
            mAudioManager.abandonAudioFocus(mCarProxyAudioFocusHandler);
        }
    }

    private void doHandleFocusGainTransientFromCar(FocusState currentState,
            AudioFocusInfo topInfo) {
        if ((currentState.externalFocus &
                (AudioHalService.VEHICLE_AUDIO_EXT_FOCUS_CAR_PERMANENT_FLAG |
                        AudioHalService.VEHICLE_AUDIO_EXT_FOCUS_CAR_TRANSIENT_FLAG)) == 0) {
            mAudioManager.abandonAudioFocus(mCarProxyAudioFocusHandler);
        } else {
            if (isFocusFromCarServiceBottom(topInfo) || isFocusFromCarProxy(topInfo)) {
                Log.w(TAG_FOCUS, "focus gain transient from car:" + currentState +
                        " while bottom listener or car proxy is top");
                mFocusHandler.handleFocusReleaseRequest();
            }
        }
    }

    private void doHandleFocusLossFromCar(FocusState currentState, AudioFocusInfo topInfo) {
        if (DBG) {
            Log.d(TAG_FOCUS, "doHandleFocusLossFromCar current:" + currentState +
                    " top:" + dumpAudioFocusInfo(topInfo));
        }
        boolean shouldRequestProxyFocus = false;
        if ((currentState.externalFocus &
                AudioHalService.VEHICLE_AUDIO_EXT_FOCUS_CAR_PERMANENT_FLAG) != 0) {
            shouldRequestProxyFocus = true;
        }
        if (isFocusFromCarProxy(topInfo)) {
            if ((currentState.externalFocus &
                    (AudioHalService.VEHICLE_AUDIO_EXT_FOCUS_CAR_PERMANENT_FLAG |
                            AudioHalService.VEHICLE_AUDIO_EXT_FOCUS_CAR_TRANSIENT_FLAG)) == 0) {
                // CarProxy in top, but no external focus: Drop it so that some other app
                // may pick up focus.
                mAudioManager.abandonAudioFocus(mCarProxyAudioFocusHandler);
                return;
            }
        } else if (!isFocusFromCarServiceBottom(topInfo)) {
            shouldRequestProxyFocus = true;
        }
        if (shouldRequestProxyFocus) {
            requestCarProxyFocus(AudioManager.AUDIOFOCUS_GAIN, 0);
        }
    }

    private void doHandleFocusLossTransientFromCar(FocusState currentState) {
        requestCarProxyFocus(AudioManager.AUDIOFOCUS_GAIN_TRANSIENT, 0);
    }

    private void doHandleFocusLossTransientCanDuckFromCar(FocusState currentState) {
        requestCarProxyFocus(AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK, 0);
    }

    private void doHandleFocusLossTransientExclusiveFromCar(FocusState currentState) {
        requestCarProxyFocus(AudioManager.AUDIOFOCUS_GAIN_TRANSIENT,
                AudioManager.AUDIOFOCUS_FLAG_LOCK);
    }

    private void requestCarProxyFocus(int androidFocus, int flags) {
        mAudioManager.requestAudioFocus(mCarProxyAudioFocusHandler, mAttributeCarExternal,
                androidFocus, flags, mAudioPolicy);
    }

    private void doHandleVolumeChange(VolumeStateChangeEvent event) {
        //TODO
    }

    private void doHandleStreamStatusChange(int streamNumber, int state) {
        //TODO
    }

    private boolean isFocusFromCarServiceBottom(AudioFocusInfo info) {
        if (info == null) {
            return false;
        }
        AudioAttributes attrib = info.getAttributes();
        if (info.getPackageName().equals(mContext.getPackageName()) &&
                CarAudioAttributesUtil.getCarUsageFromAudioAttributes(attrib) ==
                CarAudioAttributesUtil.CAR_AUDIO_USAGE_CARSERVICE_BOTTOM) {
            return true;
        }
        return false;
    }

    private boolean isFocusFromCarProxy(AudioFocusInfo info) {
        if (info == null) {
            return false;
        }
        AudioAttributes attrib = info.getAttributes();
        if (info.getPackageName().equals(mContext.getPackageName()) &&
                attrib != null &&
                CarAudioAttributesUtil.getCarUsageFromAudioAttributes(attrib) ==
                CarAudioAttributesUtil.CAR_AUDIO_USAGE_CARSERVICE_CAR_PROXY) {
            return true;
        }
        return false;
    }

    private boolean isFocusFromRadio(AudioFocusInfo info) {
        if (!mAudioHal.isRadioExternal()) {
            // if radio is not external, no special handling of radio is necessary.
            return false;
        }
        if (info == null) {
            return false;
        }
        AudioAttributes attrib = info.getAttributes();
        if (attrib != null &&
                CarAudioAttributesUtil.getCarUsageFromAudioAttributes(attrib) ==
                CarAudioManager.CAR_AUDIO_USAGE_RADIO) {
            return true;
        }
        return false;
    }

    /**
     * Re-evaluate current focus state and send focus request to car if new focus was requested.
     * @return true if focus change was requested to car.
     */
    private boolean reevaluateCarAudioFocusLocked() {
        if (mTopFocusInfo == null) {
            // should not happen
            Log.w(TAG_FOCUS, "reevaluateCarAudioFocusLocked, top focus info null");
            return false;
        }
        if (mTopFocusInfo.getLossReceived() != 0) {
            // top one got loss. This should not happen.
            Log.e(TAG_FOCUS, "Top focus holder got loss " +  dumpAudioFocusInfo(mTopFocusInfo));
            return false;
        }
        if (isFocusFromCarServiceBottom(mTopFocusInfo) || isFocusFromCarProxy(mTopFocusInfo)) {
            switch (mCurrentFocusState.focusState) {
                case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_GAIN:
                case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_GAIN_TRANSIENT:
                    // should not have focus. So enqueue release
                    mFocusHandler.handleFocusReleaseRequest();
                    break;
                case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_LOSS:
                    doHandleFocusLossFromCar(mCurrentFocusState, mTopFocusInfo);
                    break;
                case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_LOSS_TRANSIENT:
                    doHandleFocusLossTransientFromCar(mCurrentFocusState);
                    break;
                case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_LOSS_TRANSIENT_CAN_DUCK:
                    doHandleFocusLossTransientCanDuckFromCar(mCurrentFocusState);
                    break;
                case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_LOSS_TRANSIENT_EXLCUSIVE:
                    doHandleFocusLossTransientExclusiveFromCar(mCurrentFocusState);
                    break;
            }
            if (mRadioActive) { // radio is no longer active.
                mRadioActive = false;
            }
            return false;
        }
        mFocusHandler.cancelFocusReleaseRequest();
        AudioAttributes attrib = mTopFocusInfo.getAttributes();
        int logicalStreamTypeForTop = CarAudioAttributesUtil.getCarUsageFromAudioAttributes(attrib);
        int physicalStreamTypeForTop = mAudioRoutingPolicy.getPhysicalStreamForLogicalStream(
                logicalStreamTypeForTop);
        int audioContexts = 0;
        if (logicalStreamTypeForTop == CarAudioManager.CAR_AUDIO_USAGE_VOICE_CALL) {
            if (!mCallActive) {
                mCallActive = true;
                audioContexts |= AudioHalService.AUDIO_CONTEXT_CALL_FLAG;
            }
        } else {
            if (mCallActive) {
                mCallActive = false;
            }
            audioContexts = AudioHalService.logicalStreamToHalContextType(logicalStreamTypeForTop);
        }
        // other apps having focus
        int focusToRequest = AudioHalService.VEHICLE_AUDIO_FOCUS_REQUEST_RELEASE;
        int extFocus = AudioHalService.VEHICLE_AUDIO_EXT_FOCUS_NONE_FLAG;
        int streamsToRequest = 0x1 << physicalStreamTypeForTop;
        switch (mTopFocusInfo.getGainRequest()) {
            case AudioManager.AUDIOFOCUS_GAIN:
                if (isFocusFromRadio(mTopFocusInfo)) {
                    mRadioActive = true;
                    // audio context sending is only for audio from android.
                    audioContexts = 0;
                } else {
                    mRadioActive = false;
                }
                focusToRequest = AudioHalService.VEHICLE_AUDIO_FOCUS_REQUEST_GAIN;
                break;
            case AudioManager.AUDIOFOCUS_GAIN_TRANSIENT:
            case AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_EXCLUSIVE:
                // radio cannot be active
                mRadioActive = false;
                focusToRequest = AudioHalService.VEHICLE_AUDIO_FOCUS_REQUEST_GAIN_TRANSIENT;
                break;
            case AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK:
                audioContexts |= getAudioContext(mSecondFocusInfo);
                focusToRequest =
                    AudioHalService.VEHICLE_AUDIO_FOCUS_REQUEST_GAIN_TRANSIENT_MAY_DUCK;
                switch (mCurrentFocusState.focusState) {
                    case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_GAIN:
                        streamsToRequest |= mCurrentFocusState.streams;
                        focusToRequest = AudioHalService.VEHICLE_AUDIO_FOCUS_REQUEST_GAIN;
                        break;
                    case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_GAIN_TRANSIENT:
                        streamsToRequest |= mCurrentFocusState.streams;
                        focusToRequest = AudioHalService.VEHICLE_AUDIO_FOCUS_REQUEST_GAIN_TRANSIENT;
                        break;
                    case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_LOSS:
                    case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_LOSS_TRANSIENT:
                    case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_LOSS_TRANSIENT_CAN_DUCK:
                        break;
                    case AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_LOSS_TRANSIENT_EXLCUSIVE:
                        doHandleFocusLossTransientExclusiveFromCar(mCurrentFocusState);
                        return false;
                }
                break;
            default:
                streamsToRequest = 0;
                break;
        }
        if (mRadioActive) {
            // TODO any need to keep media stream while radio is active?
            //     Most cars do not allow that, but if mixing is possible, it can take media stream.
            //     For now, assume no mixing capability.
            int radioPhysicalStream = mAudioRoutingPolicy.getPhysicalStreamForLogicalStream(
                    CarAudioManager.CAR_AUDIO_USAGE_MUSIC);
            if (!isFocusFromRadio(mTopFocusInfo) &&
                    (physicalStreamTypeForTop == radioPhysicalStream)) {
                Log.i(CarLog.TAG_AUDIO, "Top stream is taking the same stream:" +
                    physicalStreamTypeForTop + " as radio, stopping radio");
                // stream conflict here. radio cannot be played
                extFocus = 0;
                mRadioActive = false;
                audioContexts &= ~AudioHalService.AUDIO_CONTEXT_RADIO_FLAG;
            } else {
                extFocus = AudioHalService.VEHICLE_AUDIO_EXT_FOCUS_CAR_PLAY_ONLY_FLAG;
                streamsToRequest &= ~(0x1 << radioPhysicalStream);
            }
        } else if (streamsToRequest == 0) {
            mCurrentAudioContexts = 0;
            mFocusHandler.handleFocusReleaseRequest();
            return false;
        }
        return sendFocusRequestToCarIfNecessaryLocked(focusToRequest, streamsToRequest, extFocus,
                audioContexts);
    }

    private static int getAudioContext(AudioFocusInfo info) {
        if (info == null) {
            return 0;
        }
        AudioAttributes attrib = info.getAttributes();
        if (attrib == null) {
            return AudioHalService.AUDIO_CONTEXT_UNKNOWN_FLAG;
        }
        return AudioHalService.logicalStreamToHalContextType(
                CarAudioAttributesUtil.getCarUsageFromAudioAttributes(attrib));
    }

    private boolean sendFocusRequestToCarIfNecessaryLocked(int focusToRequest,
            int streamsToRequest, int extFocus, int audioContexts) {
        if (needsToSendFocusRequestLocked(focusToRequest, streamsToRequest, extFocus,
                audioContexts)) {
            mLastFocusRequestToCar = FocusRequest.create(focusToRequest, streamsToRequest,
                    extFocus);
            mCurrentAudioContexts = audioContexts;
            if (DBG) {
                Log.d(TAG_FOCUS, "focus request to car:" + mLastFocusRequestToCar + " context:0x" +
                        Integer.toHexString(audioContexts));
            }
            mAudioHal.requestAudioFocusChange(focusToRequest, streamsToRequest, extFocus,
                    audioContexts);
            try {
                mLock.wait(FOCUS_RESPONSE_WAIT_TIMEOUT_MS);
            } catch (InterruptedException e) {
                //ignore
            }
            return true;
        }
        return false;
    }

    private boolean needsToSendFocusRequestLocked(int focusToRequest, int streamsToRequest,
            int extFocus, int audioContexts) {
        if (streamsToRequest != mCurrentFocusState.streams) {
            return true;
        }
        if (audioContexts != mCurrentAudioContexts) {
            return true;
        }
        if ((extFocus & mCurrentFocusState.externalFocus) != extFocus) {
            return true;
        }
        switch (focusToRequest) {
            case AudioHalService.VEHICLE_AUDIO_FOCUS_REQUEST_GAIN:
                if (mCurrentFocusState.focusState ==
                    AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_GAIN) {
                    return false;
                }
                break;
            case AudioHalService.VEHICLE_AUDIO_FOCUS_REQUEST_GAIN_TRANSIENT:
            case AudioHalService.VEHICLE_AUDIO_FOCUS_REQUEST_GAIN_TRANSIENT_MAY_DUCK:
                if (mCurrentFocusState.focusState ==
                        AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_GAIN ||
                    mCurrentFocusState.focusState ==
                        AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_GAIN_TRANSIENT) {
                    return false;
                }
                break;
            case AudioHalService.VEHICLE_AUDIO_FOCUS_REQUEST_RELEASE:
                if (mCurrentFocusState.focusState ==
                        AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_LOSS ||
                        mCurrentFocusState.focusState ==
                        AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_LOSS_TRANSIENT_EXLCUSIVE) {
                    return false;
                }
                break;
        }
        return true;
    }

    private void doHandleAndroidFocusChange() {
        boolean focusRequested = false;
        synchronized (mLock) {
            if (mPendingFocusChanges.isEmpty()) {
                // no entry. It was handled already.
                if (DBG) {
                    Log.d(TAG_FOCUS, "doHandleAndroidFocusChange, mPendingFocusChanges empty");
                }
                return;
            }
            AudioFocusInfo newTopInfo = mPendingFocusChanges.getFirst();
            mPendingFocusChanges.clear();
            if (mTopFocusInfo != null &&
                    newTopInfo.getClientId().equals(mTopFocusInfo.getClientId()) &&
                    newTopInfo.getGainRequest() == mTopFocusInfo.getGainRequest() &&
                    isAudioAttributesSame(
                            newTopInfo.getAttributes(), mTopFocusInfo.getAttributes())) {
                if (DBG) {
                    Log.d(TAG_FOCUS, "doHandleAndroidFocusChange, no change in top state:" +
                            dumpAudioFocusInfo(mTopFocusInfo));
                }
                // already in top somehow, no need to make any change
                return;
            }
            if (DBG) {
                Log.d(TAG_FOCUS, "top focus changed to:" + dumpAudioFocusInfo(newTopInfo));
            }
            if (newTopInfo.getGainRequest() == AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK) {
                mSecondFocusInfo = mTopFocusInfo;
            } else {
                mSecondFocusInfo = null;
            }
            mTopFocusInfo = newTopInfo;
            focusRequested = reevaluateCarAudioFocusLocked();
            if (DBG) {
                if (!focusRequested) {
                    Log.i(TAG_FOCUS, "focus not requested for top focus:" +
                            dumpAudioFocusInfo(newTopInfo) + " currentState:" + mCurrentFocusState);
                }
            }
            if (focusRequested && mFocusReceived == null) {
                Log.w(TAG_FOCUS, "focus response timed out, request sent" +
                        mLastFocusRequestToCar);
                // no response. so reset to loss.
                mFocusReceived = FocusState.STATE_LOSS;
                mCurrentAudioContexts = 0;
            }
        }
        // handle it if there was response or force handle it for timeout.
        if (focusRequested) {
            doHandleCarFocusChange();
        }
    }

    private void doHandleFocusRelease() {
        //TODO Is there a need to wait for the stopping of streams?
        boolean sent = false;
        synchronized (mLock) {
            if (mCurrentFocusState != FocusState.STATE_LOSS) {
                if (DBG) {
                    Log.d(TAG_FOCUS, "focus release to car");
                }
                mLastFocusRequestToCar = FocusRequest.STATE_RELEASE;
                sent = true;
                mAudioHal.requestAudioFocusChange(
                        AudioHalService.VEHICLE_AUDIO_FOCUS_REQUEST_RELEASE, 0, 0);
                try {
                    mLock.wait(FOCUS_RESPONSE_WAIT_TIMEOUT_MS);
                } catch (InterruptedException e) {
                    //ignore
                }
            } else if (DBG) {
                Log.d(TAG_FOCUS, "doHandleFocusRelease: do not send, already loss");
            }
        }
        // handle it if there was response.
        if (sent) {
            doHandleCarFocusChange();
        }
    }

    private static boolean isAudioAttributesSame(AudioAttributes one, AudioAttributes two) {
        if (one.getContentType() != two.getContentType()) {
            return false;
        }
        if (one.getUsage() != two.getUsage()) {
            return false;
        }
        return true;
    }

    private static String dumpAudioFocusInfo(AudioFocusInfo info) {
        StringBuilder builder = new StringBuilder();
        builder.append("afi package:" + info.getPackageName());
        builder.append("client id:" + info.getClientId());
        builder.append(",gain:" + info.getGainRequest());
        builder.append(",loss:" + info.getLossReceived());
        builder.append(",flag:" + info.getFlags());
        AudioAttributes attrib = info.getAttributes();
        if (attrib != null) {
            builder.append("," + attrib.toString());
        }
        return builder.toString();
    }

    private class SystemFocusListener extends AudioPolicyFocusListener {
        @Override
        public void onAudioFocusGrant(AudioFocusInfo afi, int requestResult) {
            if (afi == null) {
                return;
            }
            if (DBG) {
                Log.d(TAG_FOCUS, "onAudioFocusGrant " + dumpAudioFocusInfo(afi) +
                        " result:" + requestResult);
            }
            if (requestResult == AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
                synchronized (mLock) {
                    mPendingFocusChanges.addFirst(afi);
                }
                mFocusHandler.handleAndroidFocusChange();
            }
        }

        @Override
        public void onAudioFocusLoss(AudioFocusInfo afi, boolean wasNotified) {
            if (DBG) {
                Log.d(TAG_FOCUS, "onAudioFocusLoss " + dumpAudioFocusInfo(afi) +
                        " notified:" + wasNotified);
            }
            // ignore loss as tracking gain is enough. At least bottom listener will be
            // always there and getting focus grant. So it is safe to ignore this here.
        }
    }

    /**
     * Focus listener to take focus away from android apps as a proxy to car.
     */
    private class CarProxyAndroidFocusListener implements AudioManager.OnAudioFocusChangeListener {
        @Override
        public void onAudioFocusChange(int focusChange) {
            // Do not need to handle car's focus loss or gain separately. Focus monitoring
            // through system focus listener will take care all cases.
        }
    }

    /**
     * Focus listener kept at the bottom to check if there is any focus holder.
     *
     */
    private class BottomAudioFocusListener implements AudioManager.OnAudioFocusChangeListener {
        @Override
        public void onAudioFocusChange(int focusChange) {
            synchronized (mLock) {
                mBottomFocusState = focusChange;
            }
        }
    }

    private class CarAudioFocusChangeHandler extends Handler {
        private static final int MSG_FOCUS_CHANGE = 0;
        private static final int MSG_STREAM_STATE_CHANGE = 1;
        private static final int MSG_ANDROID_FOCUS_CHANGE = 2;
        private static final int MSG_FOCUS_RELEASE = 3;

        /** Focus release is always delayed this much to handle repeated acquire / release. */
        private static final long FOCUS_RELEASE_DELAY_MS = 500;

        private CarAudioFocusChangeHandler(Looper looper) {
            super(looper);
        }

        private void handleFocusChange() {
            Message msg = obtainMessage(MSG_FOCUS_CHANGE);
            sendMessage(msg);
        }

        private void handleStreamStateChange(int streamNumber, int state) {
            Message msg = obtainMessage(MSG_STREAM_STATE_CHANGE, streamNumber, state);
            sendMessage(msg);
        }

        private void handleAndroidFocusChange() {
            Message msg = obtainMessage(MSG_ANDROID_FOCUS_CHANGE);
            sendMessage(msg);
        }

        private void handleFocusReleaseRequest() {
            if (DBG) {
                Log.d(TAG_FOCUS, "handleFocusReleaseRequest");
            }
            cancelFocusReleaseRequest();
            Message msg = obtainMessage(MSG_FOCUS_RELEASE);
            sendMessageDelayed(msg, FOCUS_RELEASE_DELAY_MS);
        }

        private void cancelFocusReleaseRequest() {
            removeMessages(MSG_FOCUS_RELEASE);
        }

        private void cancelAll() {
            removeMessages(MSG_FOCUS_CHANGE);
            removeMessages(MSG_STREAM_STATE_CHANGE);
            removeMessages(MSG_ANDROID_FOCUS_CHANGE);
            removeMessages(MSG_FOCUS_RELEASE);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_FOCUS_CHANGE:
                    doHandleCarFocusChange();
                    break;
                case MSG_STREAM_STATE_CHANGE:
                    doHandleStreamStatusChange(msg.arg1, msg.arg2);
                    break;
                case MSG_ANDROID_FOCUS_CHANGE:
                    doHandleAndroidFocusChange();
                    break;
                case MSG_FOCUS_RELEASE:
                    doHandleFocusRelease();
                    break;
            }
        }
    }

    private class CarAudioVolumeHandler extends Handler {
        private static final int MSG_VOLUME_CHANGE = 0;

        private CarAudioVolumeHandler(Looper looper) {
            super(looper);
        }

        private void handleVolumeChange(VolumeStateChangeEvent event) {
            Message msg = obtainMessage(MSG_VOLUME_CHANGE, event);
            sendMessage(msg);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_VOLUME_CHANGE:
                    doHandleVolumeChange((VolumeStateChangeEvent) msg.obj);
                    break;
            }
        }
    }

    private static class VolumeStateChangeEvent {
        public final int stream;
        public final int volume;
        public final int state;

        public VolumeStateChangeEvent(int stream, int volume, int state) {
            this.stream = stream;
            this.volume = volume;
            this.state = state;
        }
    }

    /** Wrapper class for holding the current focus state from car. */
    private static class FocusState {
        public final int focusState;
        public final int streams;
        public final int externalFocus;

        private FocusState(int focusState, int streams, int externalFocus) {
            this.focusState = focusState;
            this.streams = streams;
            this.externalFocus = externalFocus;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) {
                return true;
            }
            if (!(o instanceof FocusState)) {
                return false;
            }
            FocusState that = (FocusState) o;
            return this.focusState == that.focusState && this.streams == that.streams &&
                    this.externalFocus == that.externalFocus;
        }

        @Override
        public String toString() {
            return "FocusState, state:" + focusState +
                    " streams:0x" + Integer.toHexString(streams) +
                    " externalFocus:0x" + Integer.toHexString(externalFocus);
        }

        public static FocusState create(int focusState, int streams, int externalAudios) {
            return new FocusState(focusState, streams, externalAudios);
        }

        public static FocusState create(int[] state) {
            return create(state[AudioHalService.FOCUS_STATE_ARRAY_INDEX_STATE],
                    state[AudioHalService.FOCUS_STATE_ARRAY_INDEX_STREAMS],
                    state[AudioHalService.FOCUS_STATE_ARRAY_INDEX_EXTERNAL_FOCUS]);
        }

        public static FocusState STATE_LOSS =
                new FocusState(AudioHalService.VEHICLE_AUDIO_FOCUS_STATE_LOSS, 0, 0);
    }

    /** Wrapper class for holding the focus requested to car. */
    private static class FocusRequest {
        public final int focusRequest;
        public final int streams;
        public final int externalFocus;

        private FocusRequest(int focusRequest, int streams, int externalFocus) {
            this.focusRequest = focusRequest;
            this.streams = streams;
            this.externalFocus = externalFocus;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) {
                return true;
            }
            if (!(o instanceof FocusRequest)) {
                return false;
            }
            FocusRequest that = (FocusRequest) o;
            return this.focusRequest == that.focusRequest && this.streams == that.streams &&
                    this.externalFocus == that.externalFocus;
        }

        @Override
        public String toString() {
            return "FocusRequest, request:" + focusRequest +
                    " streams:0x" + Integer.toHexString(streams) +
                    " externalFocus:0x" + Integer.toHexString(externalFocus);
        }

        public static FocusRequest create(int focusRequest, int streams, int externalFocus) {
            switch (focusRequest) {
                case AudioHalService.VEHICLE_AUDIO_FOCUS_REQUEST_RELEASE:
                    return STATE_RELEASE;
            }
            return new FocusRequest(focusRequest, streams, externalFocus);
        }

        public static FocusRequest STATE_RELEASE =
                new FocusRequest(AudioHalService.VEHICLE_AUDIO_FOCUS_REQUEST_RELEASE, 0, 0);
    }
}
