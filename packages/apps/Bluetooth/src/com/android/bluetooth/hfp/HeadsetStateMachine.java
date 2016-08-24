/*
 * Copyright (C) 2012 The Android Open Source Project
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

/**
 * Bluetooth Handset StateMachine
 *                      (Disconnected)
 *                           |    ^
 *                   CONNECT |    | DISCONNECTED
 *                           V    |
 *                         (Pending)
 *                           |    ^
 *                 CONNECTED |    | CONNECT
 *                           V    |
 *                        (Connected)
 *                           |    ^
 *             CONNECT_AUDIO |    | DISCONNECT_AUDIO
 *                           V    |
 *                         (AudioOn)
 */
package com.android.bluetooth.hfp;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothAssignedNumbers;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHeadset;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothUuid;
import android.bluetooth.IBluetooth;
import android.bluetooth.IBluetoothHeadsetPhone;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.ActivityNotFoundException;
import android.media.AudioManager;
import android.net.Uri;
import android.os.IBinder;
import android.os.IDeviceIdleController;
import android.os.Message;
import android.os.ParcelUuid;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.PowerManager;
import android.os.UserHandle;
import android.os.PowerManager.WakeLock;
import android.telephony.PhoneNumberUtils;
import android.util.Log;
import com.android.bluetooth.Utils;
import com.android.bluetooth.btservice.AdapterService;
import com.android.bluetooth.btservice.ProfileService;
import com.android.internal.util.IState;
import com.android.internal.util.State;
import com.android.internal.util.StateMachine;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import android.os.SystemProperties;

final class HeadsetStateMachine extends StateMachine {
    private static final String TAG = "HeadsetStateMachine";
    private static final boolean DBG = false;
    //For Debugging only
    private static int sRefCount=0;

    private static final String HEADSET_NAME = "bt_headset_name";
    private static final String HEADSET_NREC = "bt_headset_nrec";
    private static final String HEADSET_WBS = "bt_wbs";

    static final int CONNECT = 1;
    static final int DISCONNECT = 2;
    static final int CONNECT_AUDIO = 3;
    static final int DISCONNECT_AUDIO = 4;
    static final int VOICE_RECOGNITION_START = 5;
    static final int VOICE_RECOGNITION_STOP = 6;

    // message.obj is an intent AudioManager.VOLUME_CHANGED_ACTION
    // EXTRA_VOLUME_STREAM_TYPE is STREAM_BLUETOOTH_SCO
    static final int INTENT_SCO_VOLUME_CHANGED = 7;
    static final int SET_MIC_VOLUME = 8;
    static final int CALL_STATE_CHANGED = 9;
    static final int INTENT_BATTERY_CHANGED = 10;
    static final int DEVICE_STATE_CHANGED = 11;
    static final int SEND_CCLC_RESPONSE = 12;
    static final int SEND_VENDOR_SPECIFIC_RESULT_CODE = 13;

    static final int VIRTUAL_CALL_START = 14;
    static final int VIRTUAL_CALL_STOP = 15;

    static final int ENABLE_WBS = 16;
    static final int DISABLE_WBS = 17;


    private static final int STACK_EVENT = 101;
    private static final int DIALING_OUT_TIMEOUT = 102;
    private static final int START_VR_TIMEOUT = 103;
    private static final int CLCC_RSP_TIMEOUT = 104;

    private static final int CONNECT_TIMEOUT = 201;

    private static final int DIALING_OUT_TIMEOUT_VALUE = 10000;
    private static final int START_VR_TIMEOUT_VALUE = 5000;
    private static final int CLCC_RSP_TIMEOUT_VALUE = 5000;

    // Max number of HF connections at any time
    private int max_hf_connections = 1;

    private static final int NBS_CODEC = 1;
    private static final int WBS_CODEC = 2;

    // Keys are AT commands, and values are the company IDs.
    private static final Map<String, Integer> VENDOR_SPECIFIC_AT_COMMAND_COMPANY_ID;
    // Hash for storing the Audio Parameters like NREC for connected headsets
    private HashMap<BluetoothDevice, HashMap> mHeadsetAudioParam =
                                          new HashMap<BluetoothDevice, HashMap>();
    // Hash for storing the Remotedevice BRSF
    private HashMap<BluetoothDevice, Integer> mHeadsetBrsf =
                                          new HashMap<BluetoothDevice, Integer>();

    private static final ParcelUuid[] HEADSET_UUIDS = {
        BluetoothUuid.HSP,
        BluetoothUuid.Handsfree,
    };

    private Disconnected mDisconnected;
    private Pending mPending;
    private Connected mConnected;
    private AudioOn mAudioOn;
    // Multi HFP: add new class object
    private MultiHFPending mMultiHFPending;

    private HeadsetService mService;
    private PowerManager mPowerManager;
    private boolean mVirtualCallStarted = false;
    private boolean mVoiceRecognitionStarted = false;
    private boolean mWaitingForVoiceRecognition = false;
    private WakeLock mStartVoiceRecognitionWakeLock;  // held while waiting for voice recognition

    private boolean mDialingOut = false;
    private AudioManager mAudioManager;
    private AtPhonebook mPhonebook;

    private static Intent sVoiceCommandIntent;

    private HeadsetPhoneState mPhoneState;
    private int mAudioState;
    private BluetoothAdapter mAdapter;
    private IBluetoothHeadsetPhone mPhoneProxy;
    private boolean mNativeAvailable;

    // Indicates whether audio can be routed to the device.
    private boolean mAudioRouteAllowed = true;

    // mCurrentDevice is the device connected before the state changes
    // mTargetDevice is the device to be connected
    // mIncomingDevice is the device connecting to us, valid only in Pending state
    //                when mIncomingDevice is not null, both mCurrentDevice
    //                  and mTargetDevice are null
    //                when either mCurrentDevice or mTargetDevice is not null,
    //                  mIncomingDevice is null
    // Stable states
    //   No connection, Disconnected state
    //                  both mCurrentDevice and mTargetDevice are null
    //   Connected, Connected state
    //              mCurrentDevice is not null, mTargetDevice is null
    // Interim states
    //   Connecting to a device, Pending
    //                           mCurrentDevice is null, mTargetDevice is not null
    //   Disconnecting device, Connecting to new device
    //     Pending
    //     Both mCurrentDevice and mTargetDevice are not null
    //   Disconnecting device Pending
    //                        mCurrentDevice is not null, mTargetDevice is null
    //   Incoming connections Pending
    //                        Both mCurrentDevice and mTargetDevice are null
    private BluetoothDevice mCurrentDevice = null;
    private BluetoothDevice mTargetDevice = null;
    private BluetoothDevice mIncomingDevice = null;
    private BluetoothDevice mActiveScoDevice = null;
    private BluetoothDevice mMultiDisconnectDevice = null;

    // Multi HFP: Connected devices list holds all currently connected headsets
    private ArrayList<BluetoothDevice> mConnectedDevicesList =
                                             new ArrayList<BluetoothDevice>();

    static {
        classInitNative();

        VENDOR_SPECIFIC_AT_COMMAND_COMPANY_ID = new HashMap<String, Integer>();
        VENDOR_SPECIFIC_AT_COMMAND_COMPANY_ID.put("+XEVENT", BluetoothAssignedNumbers.PLANTRONICS);
        VENDOR_SPECIFIC_AT_COMMAND_COMPANY_ID.put("+ANDROID", BluetoothAssignedNumbers.GOOGLE);
    }

    private HeadsetStateMachine(HeadsetService context) {
        super(TAG);
        mService = context;
        mVoiceRecognitionStarted = false;
        mWaitingForVoiceRecognition = false;

        mPowerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        mStartVoiceRecognitionWakeLock = mPowerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
                                                       TAG + ":VoiceRecognition");
        mStartVoiceRecognitionWakeLock.setReferenceCounted(false);

        mDialingOut = false;
        mAudioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        mPhonebook = new AtPhonebook(mService, this);
        mPhoneState = new HeadsetPhoneState(context, this);
        mAudioState = BluetoothHeadset.STATE_AUDIO_DISCONNECTED;
        mAdapter = BluetoothAdapter.getDefaultAdapter();
        Intent intent = new Intent(IBluetoothHeadsetPhone.class.getName());
        intent.setComponent(intent.resolveSystemService(context.getPackageManager(), 0));
        if (intent.getComponent() == null || !context.bindService(intent, mConnection, 0)) {
            Log.e(TAG, "Could not bind to Bluetooth Headset Phone Service");
        }

        String max_hfp_clients = SystemProperties.get("bt.max.hfpclient.connections");
        if (!max_hfp_clients.isEmpty() && (Integer.parseInt(max_hfp_clients) == 2))
            max_hf_connections = Integer.parseInt(max_hfp_clients);
        Log.d(TAG, "max_hf_connections = " + max_hf_connections);
        initializeNative(max_hf_connections);
        mNativeAvailable=true;

        mDisconnected = new Disconnected();
        mPending = new Pending();
        mConnected = new Connected();
        mAudioOn = new AudioOn();
        // Multi HFP: initialise new class variable
        mMultiHFPending = new MultiHFPending();

        if (sVoiceCommandIntent == null) {
            sVoiceCommandIntent = new Intent(Intent.ACTION_VOICE_COMMAND);
            sVoiceCommandIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        }

        addState(mDisconnected);
        addState(mPending);
        addState(mConnected);
        addState(mAudioOn);
        // Multi HFP: add State
        addState(mMultiHFPending);

        setInitialState(mDisconnected);
    }

    static HeadsetStateMachine make(HeadsetService context) {
        Log.d(TAG, "make");
        HeadsetStateMachine hssm = new HeadsetStateMachine(context);
        hssm.start();
        return hssm;
    }


    public void doQuit() {
        quitNow();
    }

    public void cleanup() {
        if (mPhoneProxy != null) {
            if (DBG) Log.d(TAG,"Unbinding service...");
            synchronized (mConnection) {
                try {
                    mPhoneProxy = null;
                    mService.unbindService(mConnection);
                } catch (Exception re) {
                    Log.e(TAG,"Error unbinding from IBluetoothHeadsetPhone",re);
                }
            }
        }
        if (mPhoneState != null) {
            mPhoneState.listenForPhoneState(false);
            mPhoneState.cleanup();
        }
        if (mPhonebook != null) {
            mPhonebook.cleanup();
        }
        if (mHeadsetAudioParam != null) {
            mHeadsetAudioParam.clear();
        }
        if (mHeadsetBrsf != null) {
            mHeadsetBrsf.clear();
        }
        if (mConnectedDevicesList != null) {
            mConnectedDevicesList.clear();
        }
        if (mNativeAvailable) {
            cleanupNative();
            mNativeAvailable = false;
        }
    }

    public void dump(StringBuilder sb) {
        ProfileService.println(sb, "mCurrentDevice: " + mCurrentDevice);
        ProfileService.println(sb, "mTargetDevice: " + mTargetDevice);
        ProfileService.println(sb, "mIncomingDevice: " + mIncomingDevice);
        ProfileService.println(sb, "mActiveScoDevice: " + mActiveScoDevice);
        ProfileService.println(sb, "mMultiDisconnectDevice: " + mMultiDisconnectDevice);
        ProfileService.println(sb, "mVirtualCallStarted: " + mVirtualCallStarted);
        ProfileService.println(sb, "mVoiceRecognitionStarted: " + mVoiceRecognitionStarted);
        ProfileService.println(sb, "mWaitingForVoiceRecognition: " + mWaitingForVoiceRecognition);
        ProfileService.println(sb, "StateMachine: " + this.toString());
        ProfileService.println(sb, "mPhoneState: " + mPhoneState);
        ProfileService.println(sb, "mAudioState: " + mAudioState);
    }

    private class Disconnected extends State {
        @Override
        public void enter() {
            log("Enter Disconnected: " + getCurrentMessage().what +
                                ", size: " + mConnectedDevicesList.size());
            mPhonebook.resetAtState();
            mPhoneState.listenForPhoneState(false);
            mVoiceRecognitionStarted = false;
            mWaitingForVoiceRecognition = false;
        }

        @Override
        public boolean processMessage(Message message) {
            log("Disconnected process message: " + message.what +
                                ", size: " + mConnectedDevicesList.size());
            if (mConnectedDevicesList.size() != 0 || mTargetDevice != null ||
                                mIncomingDevice != null) {
                Log.e(TAG, "ERROR: mConnectedDevicesList is not empty," +
                       "target, or mIncomingDevice not null in Disconnected");
                return NOT_HANDLED;
            }

            boolean retValue = HANDLED;
            switch(message.what) {
                case CONNECT:
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTING,
                                   BluetoothProfile.STATE_DISCONNECTED);

                    if (!connectHfpNative(getByteAddress(device)) ) {
                        broadcastConnectionState(device, BluetoothProfile.STATE_DISCONNECTED,
                                       BluetoothProfile.STATE_CONNECTING);
                        break;
                    }

                    synchronized (HeadsetStateMachine.this) {
                        mTargetDevice = device;
                        transitionTo(mPending);
                    }
                    // TODO(BT) remove CONNECT_TIMEOUT when the stack
                    //          sends back events consistently
                    Message m = obtainMessage(CONNECT_TIMEOUT);
                    m.obj = device;
                    sendMessageDelayed(m, 30000);
                    break;
                case DISCONNECT:
                    // ignore
                    break;
                case INTENT_BATTERY_CHANGED:
                    processIntentBatteryChanged((Intent) message.obj);
                    break;
                case CALL_STATE_CHANGED:
                    processCallState((HeadsetCallState) message.obj,
                        ((message.arg1 == 1)?true:false));
                    break;
                case STACK_EVENT:
                    StackEvent event = (StackEvent) message.obj;
                    if (DBG) {
                        log("event type: " + event.type);
                    }
                    switch (event.type) {
                        case EVENT_TYPE_CONNECTION_STATE_CHANGED:
                            processConnectionEvent(event.valueInt, event.device);
                            break;
                        default:
                            Log.e(TAG, "Unexpected stack event: " + event.type);
                            break;
                    }
                    break;
                default:
                    return NOT_HANDLED;
            }
            return retValue;
        }

        @Override
        public void exit() {
            log("Exit Disconnected: " + getCurrentMessage().what);
        }

        // in Disconnected state
        private void processConnectionEvent(int state, BluetoothDevice device) {
            Log.d(TAG, "processConnectionEvent state = " + state +
                             ", device = " + device);
            switch (state) {
            case HeadsetHalConstants.CONNECTION_STATE_DISCONNECTED:
                Log.w(TAG, "Ignore HF DISCONNECTED event, device: " + device);
                break;
            case HeadsetHalConstants.CONNECTION_STATE_CONNECTING:
                if (okToConnect(device)) {
                    Log.i(TAG,"Incoming Hf accepted");
                    broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTING,
                                             BluetoothProfile.STATE_DISCONNECTED);
                    synchronized (HeadsetStateMachine.this) {
                        mIncomingDevice = device;
                        transitionTo(mPending);
                    }
                } else {
                    Log.i(TAG,"Incoming Hf rejected. priority=" + mService.getPriority(device)+
                              " bondState=" + device.getBondState());
                    //reject the connection and stay in Disconnected state itself
                    disconnectHfpNative(getByteAddress(device));
                    // the other profile connection should be initiated
                    AdapterService adapterService = AdapterService.getAdapterService();
                    if (adapterService != null) {
                        adapterService.connectOtherProfile(device,
                                                           AdapterService.PROFILE_CONN_REJECTED);
                    }
                }
                break;
            case HeadsetHalConstants.CONNECTION_STATE_CONNECTED:
                Log.w(TAG, "HFP Connected from Disconnected state");
                if (okToConnect(device)) {
                    Log.i(TAG,"Incoming Hf accepted");
                    broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTED,
                                             BluetoothProfile.STATE_DISCONNECTED);
                    synchronized (HeadsetStateMachine.this) {
                        if (!mConnectedDevicesList.contains(device)) {
                            mConnectedDevicesList.add(device);
                            Log.d(TAG, "device " + device.getAddress() +
                                          " is adding in Disconnected state");
                        }
                        mCurrentDevice = device;
                        transitionTo(mConnected);
                    }
                    configAudioParameters(device);
                } else {
                    //reject the connection and stay in Disconnected state itself
                    Log.i(TAG,"Incoming Hf rejected. priority=" + mService.getPriority(device) +
                              " bondState=" + device.getBondState());
                    disconnectHfpNative(getByteAddress(device));
                    // the other profile connection should be initiated
                    AdapterService adapterService = AdapterService.getAdapterService();
                    if (adapterService != null) {
                        adapterService.connectOtherProfile(device,
                                                           AdapterService.PROFILE_CONN_REJECTED);
                    }
                }
                break;
            case HeadsetHalConstants.CONNECTION_STATE_DISCONNECTING:
                Log.w(TAG, "Ignore HF DISCONNECTING event, device: " + device);
                break;
            default:
                Log.e(TAG, "Incorrect state: " + state);
                break;
            }
        }
    }

    private class Pending extends State {
        @Override
        public void enter() {
            log("Enter Pending: " + getCurrentMessage().what);
        }

        @Override
        public boolean processMessage(Message message) {
            log("Pending process message: " + message.what + ", size: "
                                        + mConnectedDevicesList.size());

            boolean retValue = HANDLED;
            switch(message.what) {
                case CONNECT:
                case CONNECT_AUDIO:
                    deferMessage(message);
                    break;
                case CONNECT_TIMEOUT:
                    onConnectionStateChanged(HeadsetHalConstants.CONNECTION_STATE_DISCONNECTED,
                                             getByteAddress(mTargetDevice));
                    break;
                case DISCONNECT:
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (mCurrentDevice != null && mTargetDevice != null &&
                        mTargetDevice.equals(device) ) {
                        // cancel connection to the mTargetDevice
                        broadcastConnectionState(device, BluetoothProfile.STATE_DISCONNECTED,
                                       BluetoothProfile.STATE_CONNECTING);
                        synchronized (HeadsetStateMachine.this) {
                            mTargetDevice = null;
                        }
                    } else {
                        deferMessage(message);
                    }
                    break;
                case INTENT_BATTERY_CHANGED:
                    processIntentBatteryChanged((Intent) message.obj);
                    break;
                case CALL_STATE_CHANGED:
                    processCallState((HeadsetCallState) message.obj,
                        ((message.arg1 == 1)?true:false));
                    break;
                case STACK_EVENT:
                    StackEvent event = (StackEvent) message.obj;
                    if (DBG) {
                        log("event type: " + event.type);
                    }
                    switch (event.type) {
                        case EVENT_TYPE_CONNECTION_STATE_CHANGED:
                            BluetoothDevice device1 = getDeviceForMessage(CONNECT_TIMEOUT);
                            if (device1 != null && device1.equals(event.device)) {
                                Log.d(TAG, "remove connect timeout for device = " + device1);
                                removeMessages(CONNECT_TIMEOUT);
                            }
                            processConnectionEvent(event.valueInt, event.device);
                            break;
                        default:
                            Log.e(TAG, "Unexpected event: " + event.type);
                            break;
                    }
                    break;
                default:
                    return NOT_HANDLED;
            }
            return retValue;
        }

        // in Pending state
        private void processConnectionEvent(int state, BluetoothDevice device) {
            Log.d(TAG, "processConnectionEvent state = " + state +
                                              ", device = " + device);
            switch (state) {
                case HeadsetHalConstants.CONNECTION_STATE_DISCONNECTED:
                    if (mConnectedDevicesList.contains(device)) {

                        synchronized (HeadsetStateMachine.this) {
                            mConnectedDevicesList.remove(device);
                            mHeadsetAudioParam.remove(device);
                            mHeadsetBrsf.remove(device);
                            Log.d(TAG, "device " + device.getAddress() +
                                             " is removed in Pending state");
                        }

                        broadcastConnectionState(device,
                                                 BluetoothProfile.STATE_DISCONNECTED,
                                                 BluetoothProfile.STATE_DISCONNECTING);
                        synchronized (HeadsetStateMachine.this) {
                            mCurrentDevice = null;
                        }

                        processWBSEvent(0, device); /* disable WBS audio parameters */

                        if (mTargetDevice != null) {
                            if (!connectHfpNative(getByteAddress(mTargetDevice))) {
                                broadcastConnectionState(mTargetDevice,
                                                         BluetoothProfile.STATE_DISCONNECTED,
                                                         BluetoothProfile.STATE_CONNECTING);
                                synchronized (HeadsetStateMachine.this) {
                                    mTargetDevice = null;
                                    transitionTo(mDisconnected);
                                }
                            }
                        } else {
                            synchronized (HeadsetStateMachine.this) {
                                mIncomingDevice = null;
                                if (mConnectedDevicesList.size() == 0) {
                                    transitionTo(mDisconnected);
                                }
                                else {
                                    processMultiHFConnected(device);
                                }
                            }
                        }
                    } else if (mTargetDevice != null && mTargetDevice.equals(device)) {
                        // outgoing connection failed
                        broadcastConnectionState(mTargetDevice, BluetoothProfile.STATE_DISCONNECTED,
                                                 BluetoothProfile.STATE_CONNECTING);
                        synchronized (HeadsetStateMachine.this) {
                            mTargetDevice = null;
                            if (mConnectedDevicesList.size() == 0) {
                                transitionTo(mDisconnected);
                            }
                            else {
                                transitionTo(mConnected);
                            }

                        }
                    } else if (mIncomingDevice != null && mIncomingDevice.equals(device)) {
                        broadcastConnectionState(mIncomingDevice,
                                                 BluetoothProfile.STATE_DISCONNECTED,
                                                 BluetoothProfile.STATE_CONNECTING);
                        synchronized (HeadsetStateMachine.this) {
                            mIncomingDevice = null;
                            if (mConnectedDevicesList.size() == 0) {
                                transitionTo(mDisconnected);
                            }
                            else {
                                transitionTo(mConnected);
                            }
                        }
                    } else {
                        Log.e(TAG, "Unknown device Disconnected: " + device);
                    }
                    break;
                case HeadsetHalConstants.CONNECTION_STATE_CONNECTED:
                    if (mConnectedDevicesList.contains(device)) {
                         // disconnection failed
                         broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTED,
                                             BluetoothProfile.STATE_DISCONNECTING);
                        if (mTargetDevice != null) {
                            broadcastConnectionState(mTargetDevice,
                                                 BluetoothProfile.STATE_DISCONNECTED,
                                                 BluetoothProfile.STATE_CONNECTING);
                        }
                        synchronized (HeadsetStateMachine.this) {
                            mTargetDevice = null;
                            transitionTo(mConnected);
                        }
                    } else if (mTargetDevice != null && mTargetDevice.equals(device)) {

                        synchronized (HeadsetStateMachine.this) {
                            mCurrentDevice = device;
                            mConnectedDevicesList.add(device);
                            Log.d(TAG, "device " + device.getAddress() +
                                         " is added in Pending state");
                            mTargetDevice = null;
                            transitionTo(mConnected);
                        }
                        broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTED,
                                             BluetoothProfile.STATE_CONNECTING);
                        configAudioParameters(device);
                    } else if (mIncomingDevice != null && mIncomingDevice.equals(device)) {

                        synchronized (HeadsetStateMachine.this) {
                            mCurrentDevice = device;
                            mConnectedDevicesList.add(device);
                            Log.d(TAG, "device " + device.getAddress() +
                                             " is added in Pending state");
                            mIncomingDevice = null;
                            transitionTo(mConnected);
                        }
                        broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTED,
                                             BluetoothProfile.STATE_CONNECTING);
                        configAudioParameters(device);
                    } else {
                        Log.w(TAG, "Some other incoming HF connected in Pending state");
                        if (okToConnect(device)) {
                            Log.i(TAG,"Incoming Hf accepted");
                            broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTED,
                                                     BluetoothProfile.STATE_DISCONNECTED);
                            synchronized (HeadsetStateMachine.this) {
                                mCurrentDevice = device;
                                mConnectedDevicesList.add(device);
                                Log.d(TAG, "device " + device.getAddress() +
                                             " is added in Pending state");
                            }
                            configAudioParameters(device);
                        } else {
                            //reject the connection and stay in Pending state itself
                            Log.i(TAG,"Incoming Hf rejected. priority=" +
                                mService.getPriority(device) + " bondState=" +
                                               device.getBondState());
                            disconnectHfpNative(getByteAddress(device));
                            // the other profile connection should be initiated
                            AdapterService adapterService = AdapterService.getAdapterService();
                            if (adapterService != null) {
                                adapterService.connectOtherProfile(device,
                                         AdapterService.PROFILE_CONN_REJECTED);
                            }
                        }
                    }
                    break;
                case HeadsetHalConstants.CONNECTION_STATE_CONNECTING:
                    if ((mCurrentDevice != null) && mCurrentDevice.equals(device)) {
                        log("current device tries to connect back");
                        // TODO(BT) ignore or reject
                    } else if (mTargetDevice != null && mTargetDevice.equals(device)) {
                        // The stack is connecting to target device or
                        // there is an incoming connection from the target device at the same time
                        // we already broadcasted the intent, doing nothing here
                        if (DBG) {
                            log("Stack and target device are connecting");
                        }
                    }
                    else if (mIncomingDevice != null && mIncomingDevice.equals(device)) {
                        Log.e(TAG, "Another connecting event on the incoming device");
                    } else {
                        // We get an incoming connecting request while Pending
                        // TODO(BT) is stack handing this case? let's ignore it for now
                        log("Incoming connection while pending, ignore");
                    }
                    break;
                case HeadsetHalConstants.CONNECTION_STATE_DISCONNECTING:
                    if ((mCurrentDevice != null) && mCurrentDevice.equals(device)) {
                        // we already broadcasted the intent, doing nothing here
                        if (DBG) {
                            log("stack is disconnecting mCurrentDevice");
                        }
                    } else if (mTargetDevice != null && mTargetDevice.equals(device)) {
                        Log.e(TAG, "TargetDevice is getting disconnected");
                    } else if (mIncomingDevice != null && mIncomingDevice.equals(device)) {
                        Log.e(TAG, "IncomingDevice is getting disconnected");
                    } else {
                        Log.e(TAG, "Disconnecting unknow device: " + device);
                    }
                    break;
                default:
                    Log.e(TAG, "Incorrect state: " + state);
                    break;
            }
        }

        private void processMultiHFConnected(BluetoothDevice device) {
            log("Pending state: processMultiHFConnected");
            /* Assign the current activedevice again if the disconnected
                         device equals to the current active device*/
            if (mCurrentDevice != null && mCurrentDevice.equals(device)) {
                transitionTo(mConnected);
                int deviceSize = mConnectedDevicesList.size();
                mCurrentDevice = mConnectedDevicesList.get(deviceSize-1);
            } else {
                // The disconnected device is not current active device
                if (mAudioState == BluetoothHeadset.STATE_AUDIO_CONNECTED)
                    transitionTo(mAudioOn);
                else transitionTo(mConnected);
            }
            log("processMultiHFConnected , the latest mCurrentDevice is:"
                                             + mCurrentDevice);
            log("Pending state: processMultiHFConnected ," +
                           "fake broadcasting for mCurrentDevice");
            broadcastConnectionState(mCurrentDevice, BluetoothProfile.STATE_CONNECTED,
                                         BluetoothProfile.STATE_DISCONNECTED);
        }
    }

    private class Connected extends State {
        @Override
        public void enter() {
            // Remove pending connection attempts that were deferred during the pending
            // state. This is to prevent auto connect attempts from disconnecting
            // devices that previously successfully connected.
            // TODO: This needs to check for multiple HFP connections, once supported...
            removeDeferredMessages(CONNECT);

            log("Enter Connected: " + getCurrentMessage().what +
                           ", size: " + mConnectedDevicesList.size());
            // start phone state listener here so that the CIND response as part of SLC can be
            // responded to, correctly.
            // we may enter Connected from Disconnected/Pending/AudioOn. listenForPhoneState
            // internally handles multiple calls to start listen
            mPhoneState.listenForPhoneState(true);
        }

        @Override
        public boolean processMessage(Message message) {
            log("Connected process message: " + message.what +
                          ", size: " + mConnectedDevicesList.size());
            if (DBG) {
                if (mConnectedDevicesList.size() == 0) {
                    log("ERROR: mConnectedDevicesList is empty in Connected");
                    return NOT_HANDLED;
                }
            }

            boolean retValue = HANDLED;
            switch(message.what) {
                case CONNECT:
                {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (device == null) {
                        break;
                    }

                    if (mConnectedDevicesList.contains(device)) {
                        Log.e(TAG, "ERROR: Connect received for already connected device, Ignore");
                        break;
                    }

                   if (mConnectedDevicesList.size() >= max_hf_connections) {
                       BluetoothDevice DisconnectConnectedDevice = null;
                       IState CurrentAudioState = getCurrentState();
                       Log.d(TAG, "Reach to max size, disconnect one of them first");
                       /* TODO: Disconnect based on CoD */
                       DisconnectConnectedDevice = mConnectedDevicesList.get(0);

                       broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTING,
                                   BluetoothProfile.STATE_DISCONNECTED);

                       if (!disconnectHfpNative(getByteAddress(DisconnectConnectedDevice))) {
                           broadcastConnectionState(device, BluetoothProfile.STATE_DISCONNECTED,
                                       BluetoothProfile.STATE_CONNECTING);
                           break;
                       } else {
                           broadcastConnectionState(DisconnectConnectedDevice,
                                       BluetoothProfile.STATE_DISCONNECTING,
                                       BluetoothProfile.STATE_CONNECTED);
                       }

                       synchronized (HeadsetStateMachine.this) {
                           mTargetDevice = device;
                           if (max_hf_connections == 1) {
                               transitionTo(mPending);
                           } else {
                               mMultiDisconnectDevice = DisconnectConnectedDevice;
                               transitionTo(mMultiHFPending);
                           }
                           DisconnectConnectedDevice = null;
                       }
                    }else if (mConnectedDevicesList.size() < max_hf_connections) {
                       broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTING,
                         BluetoothProfile.STATE_DISCONNECTED);
                       if (!connectHfpNative(getByteAddress(device))) {
                           broadcastConnectionState(device, BluetoothProfile.STATE_DISCONNECTED,
                               BluetoothProfile.STATE_CONNECTING);
                           break;
                       }
                       synchronized (HeadsetStateMachine.this) {
                           mTargetDevice = device;
                           // Transtion to MultiHFPending state for Multi HF connection
                           transitionTo(mMultiHFPending);
                       }
                    }
                    Message m = obtainMessage(CONNECT_TIMEOUT);
                    m.obj = device;
                    sendMessageDelayed(m, 30000);
                }
                    break;
                case DISCONNECT:
                {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (!mConnectedDevicesList.contains(device)) {
                        break;
                    }
                    broadcastConnectionState(device, BluetoothProfile.STATE_DISCONNECTING,
                                   BluetoothProfile.STATE_CONNECTED);
                    if (!disconnectHfpNative(getByteAddress(device))) {
                        broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTED,
                                       BluetoothProfile.STATE_DISCONNECTED);
                        break;
                    }

                    if (mConnectedDevicesList.size() > 1) {
                        mMultiDisconnectDevice = device;
                        transitionTo(mMultiHFPending);
                    } else {
                        transitionTo(mPending);
                    }
                }
                    break;
                case CONNECT_AUDIO:
                {
                    BluetoothDevice device = mCurrentDevice;
                    // TODO(BT) when failure, broadcast audio connecting to disconnected intent
                    //          check if device matches mCurrentDevice
                    if (mActiveScoDevice != null) {
                        log("connectAudioNative in Connected; mActiveScoDevice is not null");
                        device = mActiveScoDevice;
                    }
                    log("connectAudioNative in Connected for device = " + device);
                    connectAudioNative(getByteAddress(device));
                }
                    break;
                case VOICE_RECOGNITION_START:
                    processLocalVrEvent(HeadsetHalConstants.VR_STATE_STARTED);
                    break;
                case VOICE_RECOGNITION_STOP:
                    processLocalVrEvent(HeadsetHalConstants.VR_STATE_STOPPED);
                    break;
                case CALL_STATE_CHANGED:
                    processCallState((HeadsetCallState) message.obj, ((message.arg1==1)?true:false));
                    break;
                case INTENT_BATTERY_CHANGED:
                    processIntentBatteryChanged((Intent) message.obj);
                    break;
                case DEVICE_STATE_CHANGED:
                    processDeviceStateChanged((HeadsetDeviceState) message.obj);
                    break;
                case SEND_CCLC_RESPONSE:
                    processSendClccResponse((HeadsetClccResponse) message.obj);
                    break;
                case CLCC_RSP_TIMEOUT:
                {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    clccResponseNative(0, 0, 0, 0, false, "", 0, getByteAddress(device));
                }
                    break;
                case SEND_VENDOR_SPECIFIC_RESULT_CODE:
                    processSendVendorSpecificResultCode(
                            (HeadsetVendorSpecificResultCode) message.obj);
                    break;
                case DIALING_OUT_TIMEOUT:
                {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (mDialingOut) {
                        mDialingOut= false;
                        atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR,
                                                   0, getByteAddress(device));
                    }
                }
                    break;
                case VIRTUAL_CALL_START:
                    initiateScoUsingVirtualVoiceCall();
                    break;
                case VIRTUAL_CALL_STOP:
                    terminateScoUsingVirtualVoiceCall();
                    break;
                case ENABLE_WBS:
                {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    configureWBSNative(getByteAddress(device),WBS_CODEC);
                }
                    break;
                case DISABLE_WBS:
                {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    configureWBSNative(getByteAddress(device),NBS_CODEC);
                }
                    break;
                case START_VR_TIMEOUT:
                {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (mWaitingForVoiceRecognition) {
                        device = (BluetoothDevice) message.obj;
                        mWaitingForVoiceRecognition = false;
                        Log.e(TAG, "Timeout waiting for voice recognition to start");
                        atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR,
                                                   0, getByteAddress(device));
                    }
                }
                    break;
                case STACK_EVENT:
                    StackEvent event = (StackEvent) message.obj;
                    if (DBG) {
                        log("event type: " + event.type + "event device : "
                                                  + event.device);
                    }
                    switch (event.type) {
                        case EVENT_TYPE_CONNECTION_STATE_CHANGED:
                            processConnectionEvent(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_AUDIO_STATE_CHANGED:
                            processAudioEvent(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_VR_STATE_CHANGED:
                            processVrEvent(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_ANSWER_CALL:
                            // TODO(BT) could answer call happen on Connected state?
                            processAnswerCall(event.device);
                            break;
                        case EVENT_TYPE_HANGUP_CALL:
                            // TODO(BT) could hangup call happen on Connected state?
                            processHangupCall(event.device);
                            break;
                        case EVENT_TYPE_VOLUME_CHANGED:
                            processVolumeEvent(event.valueInt, event.valueInt2,
                                                        event.device);
                            break;
                        case EVENT_TYPE_DIAL_CALL:
                            processDialCall(event.valueString, event.device);
                            break;
                        case EVENT_TYPE_SEND_DTMF:
                            processSendDtmf(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_NOICE_REDUCTION:
                            processNoiceReductionEvent(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_WBS:
                            Log.d(TAG, "EVENT_TYPE_WBS codec is "+event.valueInt);
                            processWBSEvent(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_AT_CHLD:
                            processAtChld(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_SUBSCRIBER_NUMBER_REQUEST:
                            processSubscriberNumberRequest(event.device);
                            break;
                        case EVENT_TYPE_AT_CIND:
                            processAtCind(event.device);
                            break;
                        case EVENT_TYPE_AT_COPS:
                            processAtCops(event.device);
                            break;
                        case EVENT_TYPE_AT_CLCC:
                            processAtClcc(event.device);
                            break;
                        case EVENT_TYPE_UNKNOWN_AT:
                            processUnknownAt(event.valueString, event.device);
                            break;
                        case EVENT_TYPE_KEY_PRESSED:
                            processKeyPressed(event.device);
                            break;
                        default:
                            Log.e(TAG, "Unknown stack event: " + event.type);
                            break;
                    }
                    break;
                default:
                    return NOT_HANDLED;
            }
            return retValue;
        }

        // in Connected state
        private void processConnectionEvent(int state, BluetoothDevice device) {
        Log.d(TAG, "processConnectionEvent state = " + state + ", device = "
                                                           + device);
            switch (state) {
                case HeadsetHalConstants.CONNECTION_STATE_DISCONNECTED:
                    if (mConnectedDevicesList.contains(device)) {
                        processWBSEvent(0, device); /* disable WBS audio parameters */
                        synchronized (HeadsetStateMachine.this) {
                            mConnectedDevicesList.remove(device);
                            mHeadsetAudioParam.remove(device);
                            mHeadsetBrsf.remove(device);
                            Log.d(TAG, "device " + device.getAddress() +
                                         " is removed in Connected state");

                            if (mConnectedDevicesList.size() == 0) {
                                mCurrentDevice = null;
                                transitionTo(mDisconnected);
                            }
                            else {
                                processMultiHFConnected(device);
                            }
                        }
                        broadcastConnectionState(device, BluetoothProfile.STATE_DISCONNECTED,
                                                 BluetoothProfile.STATE_CONNECTED);
                    } else {
                        Log.e(TAG, "Disconnected from unknown device: " + device);
                    }
                    break;
                case HeadsetHalConstants.CONNECTION_STATE_SLC_CONNECTED:
                    processSlcConnected();
                    break;
                case HeadsetHalConstants.CONNECTION_STATE_CONNECTED:
                    if (mConnectedDevicesList.contains(device)) {
                        mIncomingDevice = null;
                        mTargetDevice = null;
                        break;
                    }
                    Log.w(TAG, "HFP to be Connected in Connected state");
                    if (okToConnect(device) && (mConnectedDevicesList.size()
                                                       < max_hf_connections)) {
                        Log.i(TAG,"Incoming Hf accepted");
                        broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTED,
                                          BluetoothProfile.STATE_DISCONNECTED);
                        synchronized (HeadsetStateMachine.this) {
                            if(!mConnectedDevicesList.contains(device)) {
                                mCurrentDevice = device;
                                mConnectedDevicesList.add(device);
                                Log.d(TAG, "device " + device.getAddress() +
                                             " is added in Connected state");
                            }
                            transitionTo(mConnected);
                        }
                        configAudioParameters(device);
                    } else {
                        // reject the connection and stay in Connected state itself
                        Log.i(TAG,"Incoming Hf rejected. priority=" +
                               mService.getPriority(device) + " bondState=" +
                                        device.getBondState());
                        disconnectHfpNative(getByteAddress(device));
                        // the other profile connection should be initiated
                        AdapterService adapterService = AdapterService.getAdapterService();
                        if (adapterService != null) {
                            adapterService.connectOtherProfile(device,
                                                        AdapterService.PROFILE_CONN_REJECTED);
                        }
                    }
                    break;
                default:
                  Log.e(TAG, "Connection State Device: " + device + " bad state: " + state);
                    break;
            }
        }

        // in Connected state
        private void processAudioEvent(int state, BluetoothDevice device) {
            if (!mConnectedDevicesList.contains(device)) {
                Log.e(TAG, "Audio changed on disconnected device: " + device);
                return;
            }

            switch (state) {
                case HeadsetHalConstants.AUDIO_STATE_CONNECTED:
                    if (!isScoAcceptable()) {
                        Log.e(TAG,"Audio Connected without any listener");
                        disconnectAudioNative(getByteAddress(device));
                        break;
                    }

                    // TODO(BT) should I save the state for next broadcast as the prevState?
                    mAudioState = BluetoothHeadset.STATE_AUDIO_CONNECTED;
                    setAudioParameters(device); /*Set proper Audio Paramters.*/
                    mAudioManager.setBluetoothScoOn(true);
                    broadcastAudioState(device, BluetoothHeadset.STATE_AUDIO_CONNECTED,
                                        BluetoothHeadset.STATE_AUDIO_CONNECTING);
                    mActiveScoDevice = device;
                    transitionTo(mAudioOn);
                    break;
                case HeadsetHalConstants.AUDIO_STATE_CONNECTING:
                    mAudioState = BluetoothHeadset.STATE_AUDIO_CONNECTING;
                    broadcastAudioState(device, BluetoothHeadset.STATE_AUDIO_CONNECTING,
                                        BluetoothHeadset.STATE_AUDIO_DISCONNECTED);
                    break;
                    // TODO(BT) process other states
                default:
                    Log.e(TAG, "Audio State Device: " + device + " bad state: " + state);
                    break;
            }
        }

        private void processSlcConnected() {
            if (mPhoneProxy != null) {
                try {
                    mPhoneProxy.queryPhoneState();
                } catch (RemoteException e) {
                    Log.e(TAG, Log.getStackTraceString(new Throwable()));
                }
            } else {
                Log.e(TAG, "Handsfree phone proxy null for query phone state");
            }

        }

        private void processMultiHFConnected(BluetoothDevice device) {
            log("Connect state: processMultiHFConnected");
            if (mActiveScoDevice != null && mActiveScoDevice.equals(device)) {
                log ("mActiveScoDevice is disconnected, setting it to null");
                mActiveScoDevice = null;
            }
            /* Assign the current activedevice again if the disconnected
                         device equals to the current active device */
            if (mCurrentDevice != null && mCurrentDevice.equals(device)) {
                transitionTo(mConnected);
                int deviceSize = mConnectedDevicesList.size();
                mCurrentDevice = mConnectedDevicesList.get(deviceSize-1);
            } else {
                // The disconnected device is not current active device
                transitionTo(mConnected);
            }
            log("processMultiHFConnected , the latest mCurrentDevice is:" +
                                     mCurrentDevice);
            log("Connect state: processMultiHFConnected ," +
                       "fake broadcasting for mCurrentDevice");
            broadcastConnectionState(mCurrentDevice, BluetoothProfile.STATE_CONNECTED,
                            BluetoothProfile.STATE_DISCONNECTED);
        }
    }

    private class AudioOn extends State {

        @Override
        public void enter() {
            log("Enter AudioOn: " + getCurrentMessage().what + ", size: " +
                                  mConnectedDevicesList.size());
        }

        @Override
        public boolean processMessage(Message message) {
            log("AudioOn process message: " + message.what + ", size: " +
                                  mConnectedDevicesList.size());
            if (DBG) {
                if (mConnectedDevicesList.size() == 0) {
                    log("ERROR: mConnectedDevicesList is empty in AudioOn");
                    return NOT_HANDLED;
                }
            }

            boolean retValue = HANDLED;
            switch(message.what) {
                case CONNECT:
                {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (device == null) {
                        break;
                    }

                    if (mConnectedDevicesList.contains(device)) {
                        break;
                    }

                    if (max_hf_connections == 1) {
                        deferMessage(obtainMessage(DISCONNECT, mCurrentDevice));
                        deferMessage(obtainMessage(CONNECT, device));
                        if (disconnectAudioNative(getByteAddress(mCurrentDevice))) {
                            Log.d(TAG, "Disconnecting SCO audio for device = " + mCurrentDevice);
                        } else {
                            Log.e(TAG, "disconnectAudioNative failed");
                        }
                        break;
                    }

                    if (mConnectedDevicesList.size() >= max_hf_connections) {
                        BluetoothDevice DisconnectConnectedDevice = null;
                        IState CurrentAudioState = getCurrentState();
                        Log.d(TAG, "Reach to max size, disconnect " +
                                           "one of them first");
                        DisconnectConnectedDevice = mConnectedDevicesList.get(0);

                        if (mActiveScoDevice.equals(DisconnectConnectedDevice)) {
                           DisconnectConnectedDevice = mConnectedDevicesList.get(1);
                        }

                        broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTING,
                                   BluetoothProfile.STATE_DISCONNECTED);

                        if (!disconnectHfpNative(getByteAddress(DisconnectConnectedDevice))) {
                            broadcastConnectionState(device, BluetoothProfile.STATE_DISCONNECTED,
                                           BluetoothProfile.STATE_CONNECTING);
                            break;
                        } else {
                            broadcastConnectionState(DisconnectConnectedDevice,
                                       BluetoothProfile.STATE_DISCONNECTING,
                                       BluetoothProfile.STATE_CONNECTED);
                        }

                        synchronized (HeadsetStateMachine.this) {
                            mTargetDevice = device;
                            mMultiDisconnectDevice = DisconnectConnectedDevice;
                            transitionTo(mMultiHFPending);
                            DisconnectConnectedDevice = null;
                        }
                    } else if(mConnectedDevicesList.size() < max_hf_connections) {
                        broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTING,
                        BluetoothProfile.STATE_DISCONNECTED);
                        if (!connectHfpNative(getByteAddress(device))) {
                            broadcastConnectionState(device, BluetoothProfile.STATE_DISCONNECTED,
                                BluetoothProfile.STATE_CONNECTING);
                            break;
                        }
                        synchronized (HeadsetStateMachine.this) {
                            mTargetDevice = device;
                            // Transtion to MultilHFPending state for Multi handsfree connection
                            transitionTo(mMultiHFPending);
                        }
                    }
                    Message m = obtainMessage(CONNECT_TIMEOUT);
                    m.obj = device;
                    sendMessageDelayed(m, 30000);
                }
                break;
                case CONNECT_TIMEOUT:
                    onConnectionStateChanged(HeadsetHalConstants.CONNECTION_STATE_DISCONNECTED,
                                             getByteAddress(mTargetDevice));
                break;
                case DISCONNECT:
                {
                    BluetoothDevice device = (BluetoothDevice)message.obj;
                    if (!mConnectedDevicesList.contains(device)) {
                        break;
                    }
                    if (mActiveScoDevice != null && mActiveScoDevice.equals(device)) {
                        // The disconnected device is active SCO device
                        Log.d(TAG, "AudioOn, the disconnected device" +
                                            "is active SCO device");
                        deferMessage(obtainMessage(DISCONNECT, message.obj));
                        // Disconnect BT SCO first
                        if (disconnectAudioNative(getByteAddress(mActiveScoDevice))) {
                            log("Disconnecting SCO audio");
                        } else {
                            // if disconnect BT SCO failed, transition to mConnected state
                            transitionTo(mConnected);
                        }
                    } else {
                        /* Do not disconnect BT SCO if the disconnected
                           device is not active SCO device */
                        Log.d(TAG, "AudioOn, the disconnected device" +
                                        "is not active SCO device");
                        broadcastConnectionState(device, BluetoothProfile.STATE_DISCONNECTING,
                                   BluetoothProfile.STATE_CONNECTED);
                        // Should be still in AudioOn state
                        if (!disconnectHfpNative(getByteAddress(device))) {
                            Log.w(TAG, "AudioOn, disconnect device failed");
                            broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTED,
                                       BluetoothProfile.STATE_DISCONNECTING);
                            break;
                        }
                        /* Transtion to MultiHFPending state for Multi
                           handsfree connection */
                        if (mConnectedDevicesList.size() > 1) {
                            mMultiDisconnectDevice = device;
                            transitionTo(mMultiHFPending);
                        }
                    }
                }
                break;
                case DISCONNECT_AUDIO:
                    if (mActiveScoDevice != null) {
                        if (disconnectAudioNative(getByteAddress(mActiveScoDevice))) {
                            log("Disconnecting SCO audio for device = " +
                                                 mActiveScoDevice);
                        } else {
                            Log.e(TAG, "disconnectAudioNative failed" +
                                      "for device = " + mActiveScoDevice);
                        }
                    }
                    break;
                case VOICE_RECOGNITION_START:
                    processLocalVrEvent(HeadsetHalConstants.VR_STATE_STARTED);
                    break;
                case VOICE_RECOGNITION_STOP:
                    processLocalVrEvent(HeadsetHalConstants.VR_STATE_STOPPED);
                    break;
                case INTENT_SCO_VOLUME_CHANGED:
                    if (mActiveScoDevice != null) {
                        processIntentScoVolume((Intent) message.obj, mActiveScoDevice);
                    }
                    break;
                case CALL_STATE_CHANGED:
                    processCallState((HeadsetCallState) message.obj, ((message.arg1 == 1)?true:false));
                    break;
                case INTENT_BATTERY_CHANGED:
                    processIntentBatteryChanged((Intent) message.obj);
                    break;
                case DEVICE_STATE_CHANGED:
                    processDeviceStateChanged((HeadsetDeviceState) message.obj);
                    break;
                case SEND_CCLC_RESPONSE:
                    processSendClccResponse((HeadsetClccResponse) message.obj);
                    break;
                case CLCC_RSP_TIMEOUT:
                {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    clccResponseNative(0, 0, 0, 0, false, "", 0, getByteAddress(device));
                }
                    break;
                case SEND_VENDOR_SPECIFIC_RESULT_CODE:
                    processSendVendorSpecificResultCode(
                            (HeadsetVendorSpecificResultCode) message.obj);
                    break;

                case VIRTUAL_CALL_START:
                    initiateScoUsingVirtualVoiceCall();
                    break;
                case VIRTUAL_CALL_STOP:
                    terminateScoUsingVirtualVoiceCall();
                    break;

                case DIALING_OUT_TIMEOUT:
                {
                    if (mDialingOut) {
                        BluetoothDevice device = (BluetoothDevice)message.obj;
                        mDialingOut= false;
                        atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR,
                                               0, getByteAddress(device));
                    }
                }
                    break;
                case START_VR_TIMEOUT:
                {
                    if (mWaitingForVoiceRecognition) {
                        BluetoothDevice device = (BluetoothDevice)message.obj;
                        mWaitingForVoiceRecognition = false;
                        Log.e(TAG, "Timeout waiting for voice recognition" +
                                                     "to start");
                        atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR,
                                               0, getByteAddress(device));
                    }
                }
                    break;
                case STACK_EVENT:
                    StackEvent event = (StackEvent) message.obj;
                    if (DBG) {
                        log("event type: " + event.type);
                    }
                    switch (event.type) {
                        case EVENT_TYPE_CONNECTION_STATE_CHANGED:
                            BluetoothDevice device1 = getDeviceForMessage(CONNECT_TIMEOUT);
                            if (device1 != null && device1.equals(event.device)) {
                                Log.d(TAG, "remove connect timeout for device = " + device1);
                                removeMessages(CONNECT_TIMEOUT);
                            }
                            processConnectionEvent(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_AUDIO_STATE_CHANGED:
                            processAudioEvent(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_VR_STATE_CHANGED:
                            processVrEvent(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_ANSWER_CALL:
                            processAnswerCall(event.device);
                            break;
                        case EVENT_TYPE_HANGUP_CALL:
                            processHangupCall(event.device);
                            break;
                        case EVENT_TYPE_VOLUME_CHANGED:
                            processVolumeEvent(event.valueInt, event.valueInt2,
                                                     event.device);
                            break;
                        case EVENT_TYPE_DIAL_CALL:
                            processDialCall(event.valueString, event.device);
                            break;
                        case EVENT_TYPE_SEND_DTMF:
                            processSendDtmf(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_NOICE_REDUCTION:
                            processNoiceReductionEvent(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_AT_CHLD:
                            processAtChld(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_SUBSCRIBER_NUMBER_REQUEST:
                            processSubscriberNumberRequest(event.device);
                            break;
                        case EVENT_TYPE_AT_CIND:
                            processAtCind(event.device);
                            break;
                        case EVENT_TYPE_AT_COPS:
                            processAtCops(event.device);
                            break;
                        case EVENT_TYPE_AT_CLCC:
                            processAtClcc(event.device);
                            break;
                        case EVENT_TYPE_UNKNOWN_AT:
                            processUnknownAt(event.valueString, event.device);
                            break;
                        case EVENT_TYPE_KEY_PRESSED:
                            processKeyPressed(event.device);
                            break;
                        default:
                            Log.e(TAG, "Unknown stack event: " + event.type);
                            break;
                    }
                    break;
                default:
                    return NOT_HANDLED;
            }
            return retValue;
        }

        // in AudioOn state. Some headsets disconnect RFCOMM prior to SCO down. Handle this
        private void processConnectionEvent(int state, BluetoothDevice device) {
        Log.d(TAG, "processConnectionEvent state = " + state + ", device = " +
                                                   device);
            switch (state) {
                case HeadsetHalConstants.CONNECTION_STATE_DISCONNECTED:
                    if (mConnectedDevicesList.contains(device)) {
                        if (mActiveScoDevice != null
                            && mActiveScoDevice.equals(device)&& mAudioState
                            != BluetoothHeadset.STATE_AUDIO_DISCONNECTED) {
                            processAudioEvent(
                                HeadsetHalConstants.AUDIO_STATE_DISCONNECTED, device);
                        }

                        synchronized (HeadsetStateMachine.this) {
                            mConnectedDevicesList.remove(device);
                            mHeadsetAudioParam.remove(device);
                            mHeadsetBrsf.remove(device);
                            Log.d(TAG, "device " + device.getAddress() +
                                           " is removed in AudioOn state");
                            broadcastConnectionState(device, BluetoothProfile.STATE_DISCONNECTED,
                                                     BluetoothProfile.STATE_CONNECTED);
                            processWBSEvent(0, device); /* disable WBS audio parameters */
                            if (mConnectedDevicesList.size() == 0) {
                                transitionTo(mDisconnected);
                            }
                            else {
                                processMultiHFConnected(device);
                            }
                        }
                    } else {
                        Log.e(TAG, "Disconnected from unknown device: " + device);
                    }
                    break;
               case HeadsetHalConstants.CONNECTION_STATE_SLC_CONNECTED:
                    processSlcConnected();
                    break;
                case HeadsetHalConstants.CONNECTION_STATE_CONNECTED:
                    if (mConnectedDevicesList.contains(device)) {
                        mIncomingDevice = null;
                        mTargetDevice = null;
                        break;
                    }
                    Log.w(TAG, "HFP to be Connected in AudioOn state");
                    if (okToConnect(device) && (mConnectedDevicesList.size()
                                                      < max_hf_connections) ) {
                        Log.i(TAG,"Incoming Hf accepted");
                        broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTED,
                                          BluetoothProfile.STATE_DISCONNECTED);
                        synchronized (HeadsetStateMachine.this) {
                            if (!mConnectedDevicesList.contains(device)) {
                                mCurrentDevice = device;
                                mConnectedDevicesList.add(device);
                                Log.d(TAG, "device " + device.getAddress() +
                                              " is added in AudioOn state");
                            }
                        }
                        configAudioParameters(device);
                     } else {
                         // reject the connection and stay in Connected state itself
                         Log.i(TAG,"Incoming Hf rejected. priority="
                                      + mService.getPriority(device) +
                                       " bondState=" + device.getBondState());
                         disconnectHfpNative(getByteAddress(device));
                         // the other profile connection should be initiated
                         AdapterService adapterService = AdapterService.getAdapterService();
                         if (adapterService != null) {
                             adapterService.connectOtherProfile(device,
                                             AdapterService.PROFILE_CONN_REJECTED);
                         }
                    }
                    break;
                default:
                    Log.e(TAG, "Connection State Device: " + device + " bad state: " + state);
                    break;
            }
        }

        // in AudioOn state
        private void processAudioEvent(int state, BluetoothDevice device) {
            if (!mConnectedDevicesList.contains(device)) {
                Log.e(TAG, "Audio changed on disconnected device: " + device);
                return;
            }

            switch (state) {
                case HeadsetHalConstants.AUDIO_STATE_DISCONNECTED:
                    if (mAudioState != BluetoothHeadset.STATE_AUDIO_DISCONNECTED) {
                        mAudioState = BluetoothHeadset.STATE_AUDIO_DISCONNECTED;
                        mAudioManager.setBluetoothScoOn(false);
                        broadcastAudioState(device, BluetoothHeadset.STATE_AUDIO_DISCONNECTED,
                                            BluetoothHeadset.STATE_AUDIO_CONNECTED);
                    }
                    transitionTo(mConnected);
                    break;
                case HeadsetHalConstants.AUDIO_STATE_DISCONNECTING:
                    // TODO(BT) adding STATE_AUDIO_DISCONNECTING in BluetoothHeadset?
                    //broadcastAudioState(device, BluetoothHeadset.STATE_AUDIO_DISCONNECTING,
                    //                    BluetoothHeadset.STATE_AUDIO_CONNECTED);
                    break;
                default:
                    Log.e(TAG, "Audio State Device: " + device + " bad state: " + state);
                    break;
            }
        }

        private void processSlcConnected() {
            if (mPhoneProxy != null) {
                try {
                    mPhoneProxy.queryPhoneState();
                } catch (RemoteException e) {
                    Log.e(TAG, Log.getStackTraceString(new Throwable()));
                }
            } else {
                Log.e(TAG, "Handsfree phone proxy null for query phone state");
            }
        }

        private void processIntentScoVolume(Intent intent, BluetoothDevice device) {
            int volumeValue = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_VALUE, 0);
            if (mPhoneState.getSpeakerVolume() != volumeValue) {
                mPhoneState.setSpeakerVolume(volumeValue);
                setVolumeNative(HeadsetHalConstants.VOLUME_TYPE_SPK,
                                        volumeValue, getByteAddress(device));
            }
        }

        private void processMultiHFConnected(BluetoothDevice device) {
            log("AudioOn state: processMultiHFConnected");
            /* Assign the current activedevice again if the disconnected
                          device equals to the current active device */
            if (mCurrentDevice != null && mCurrentDevice.equals(device)) {
                int deviceSize = mConnectedDevicesList.size();
                mCurrentDevice = mConnectedDevicesList.get(deviceSize-1);
            }
            if (mAudioState != BluetoothHeadset.STATE_AUDIO_CONNECTED)
                transitionTo(mConnected);

            log("processMultiHFConnected , the latest mCurrentDevice is:"
                                      + mCurrentDevice);
            log("AudioOn state: processMultiHFConnected ," +
                       "fake broadcasting for mCurrentDevice");
            broadcastConnectionState(mCurrentDevice, BluetoothProfile.STATE_CONNECTED,
                            BluetoothProfile.STATE_DISCONNECTED);
        }
    }

    /* Add MultiHFPending state when atleast 1 HS is connected
            and disconnect/connect new HS */
    private class MultiHFPending extends State {
        @Override
        public void enter() {
            log("Enter MultiHFPending: " + getCurrentMessage().what +
                         ", size: " + mConnectedDevicesList.size());
        }

        @Override
        public boolean processMessage(Message message) {
            log("MultiHFPending process message: " + message.what +
                         ", size: " + mConnectedDevicesList.size());

            boolean retValue = HANDLED;
            switch(message.what) {
                case CONNECT:
                    deferMessage(message);
                    break;

                case CONNECT_AUDIO:
                    if (mCurrentDevice != null) {
                        connectAudioNative(getByteAddress(mCurrentDevice));
                    }
                    break;
                case CONNECT_TIMEOUT:
                    onConnectionStateChanged(HeadsetHalConstants.CONNECTION_STATE_DISCONNECTED,
                                             getByteAddress(mTargetDevice));
                    break;

                case DISCONNECT_AUDIO:
                    if (mActiveScoDevice != null) {
                        if (disconnectAudioNative(getByteAddress(mActiveScoDevice))) {
                            Log.d(TAG, "MultiHFPending, Disconnecting SCO audio for " +
                                                 mActiveScoDevice);
                        } else {
                            Log.e(TAG, "disconnectAudioNative failed" +
                                      "for device = " + mActiveScoDevice);
                        }
                    }
                    break;
                case DISCONNECT:
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (mConnectedDevicesList.contains(device) &&
                        mTargetDevice != null && mTargetDevice.equals(device)) {
                        // cancel connection to the mTargetDevice
                        broadcastConnectionState(device,
                                       BluetoothProfile.STATE_DISCONNECTED,
                                       BluetoothProfile.STATE_CONNECTING);
                        synchronized (HeadsetStateMachine.this) {
                            mTargetDevice = null;
                        }
                    } else {
                        deferMessage(message);
                    }
                    break;
                case VOICE_RECOGNITION_START:
                    device = (BluetoothDevice) message.obj;
                    if (mConnectedDevicesList.contains(device)) {
                        processLocalVrEvent(HeadsetHalConstants.VR_STATE_STARTED);
                    }
                    break;
                case VOICE_RECOGNITION_STOP:
                    device = (BluetoothDevice) message.obj;
                    if (mConnectedDevicesList.contains(device)) {
                        processLocalVrEvent(HeadsetHalConstants.VR_STATE_STOPPED);
                    }
                    break;
                case INTENT_SCO_VOLUME_CHANGED:
                    if (mActiveScoDevice != null) {
                        processIntentScoVolume((Intent) message.obj, mActiveScoDevice);
                    }
                    break;
                case INTENT_BATTERY_CHANGED:
                    processIntentBatteryChanged((Intent) message.obj);
                    break;
                case CALL_STATE_CHANGED:
                    processCallState((HeadsetCallState) message.obj,
                                      ((message.arg1 == 1)?true:false));
                    break;
                case DEVICE_STATE_CHANGED:
                    processDeviceStateChanged((HeadsetDeviceState) message.obj);
                    break;
                case SEND_CCLC_RESPONSE:
                    processSendClccResponse((HeadsetClccResponse) message.obj);
                    break;
                case CLCC_RSP_TIMEOUT:
                {
                    device = (BluetoothDevice) message.obj;
                    clccResponseNative(0, 0, 0, 0, false, "", 0, getByteAddress(device));
                }
                    break;
                case DIALING_OUT_TIMEOUT:
                    if (mDialingOut) {
                        device = (BluetoothDevice) message.obj;
                        mDialingOut= false;
                        atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR,
                                             0, getByteAddress(device));
                    }
                    break;
                case VIRTUAL_CALL_START:
                    device = (BluetoothDevice) message.obj;
                    if(mConnectedDevicesList.contains(device)) {
                        initiateScoUsingVirtualVoiceCall();
                    }
                    break;
                case VIRTUAL_CALL_STOP:
                    device = (BluetoothDevice) message.obj;
                    if (mConnectedDevicesList.contains(device)) {
                        terminateScoUsingVirtualVoiceCall();
                    }
                    break;
                case START_VR_TIMEOUT:
                    if (mWaitingForVoiceRecognition) {
                        device = (BluetoothDevice) message.obj;
                        mWaitingForVoiceRecognition = false;
                        Log.e(TAG, "Timeout waiting for voice" +
                                             "recognition to start");
                        atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR,
                                               0, getByteAddress(device));
                    }
                    break;
                case STACK_EVENT:
                    StackEvent event = (StackEvent) message.obj;
                    if (DBG) {
                        log("event type: " + event.type);
                    }
                    switch (event.type) {
                        case EVENT_TYPE_CONNECTION_STATE_CHANGED:
                            BluetoothDevice device1 = getDeviceForMessage(CONNECT_TIMEOUT);
                            if (device1 != null && device1.equals(event.device)) {
                                Log.d(TAG, "remove connect timeout for device = " + device1);
                                removeMessages(CONNECT_TIMEOUT);
                            }
                            processConnectionEvent(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_AUDIO_STATE_CHANGED:
                            processAudioEvent(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_VR_STATE_CHANGED:
                            processVrEvent(event.valueInt,event.device);
                            break;
                        case EVENT_TYPE_ANSWER_CALL:
                            //TODO(BT) could answer call happen on Connected state?
                            processAnswerCall(event.device);
                            break;
                        case EVENT_TYPE_HANGUP_CALL:
                            // TODO(BT) could hangup call happen on Connected state?
                            processHangupCall(event.device);
                            break;
                        case EVENT_TYPE_VOLUME_CHANGED:
                            processVolumeEvent(event.valueInt, event.valueInt2,
                                                    event.device);
                            break;
                        case EVENT_TYPE_DIAL_CALL:
                            processDialCall(event.valueString, event.device);
                            break;
                        case EVENT_TYPE_SEND_DTMF:
                            processSendDtmf(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_NOICE_REDUCTION:
                            processNoiceReductionEvent(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_SUBSCRIBER_NUMBER_REQUEST:
                            processSubscriberNumberRequest(event.device);
                            break;
                        case EVENT_TYPE_AT_CIND:
                            processAtCind(event.device);
                            break;
                        case EVENT_TYPE_AT_CHLD:
                            processAtChld(event.valueInt, event.device);
                            break;
                        case EVENT_TYPE_AT_COPS:
                            processAtCops(event.device);
                            break;
                        case EVENT_TYPE_AT_CLCC:
                            processAtClcc(event.device);
                            break;
                        case EVENT_TYPE_UNKNOWN_AT:
                            processUnknownAt(event.valueString,event.device);
                            break;
                        case EVENT_TYPE_KEY_PRESSED:
                            processKeyPressed(event.device);
                            break;
                        default:
                            Log.e(TAG, "Unexpected event: " + event.type);
                            break;
                    }
                    break;
                default:
                    return NOT_HANDLED;
            }
            return retValue;
        }

        // in MultiHFPending state
        private void processConnectionEvent(int state, BluetoothDevice device) {
            Log.d(TAG, "processConnectionEvent state = " + state +
                                     ", device = " + device);
            switch (state) {
                case HeadsetHalConstants.CONNECTION_STATE_DISCONNECTED:
                    if (mConnectedDevicesList.contains(device)) {
                        if (mMultiDisconnectDevice != null &&
                                mMultiDisconnectDevice.equals(device)) {
                            mMultiDisconnectDevice = null;

                          synchronized (HeadsetStateMachine.this) {
                              mConnectedDevicesList.remove(device);
                              mHeadsetAudioParam.remove(device);
                              mHeadsetBrsf.remove(device);
                              Log.d(TAG, "device " + device.getAddress() +
                                      " is removed in MultiHFPending state");
                              broadcastConnectionState(device,
                                        BluetoothProfile.STATE_DISCONNECTED,
                                        BluetoothProfile.STATE_DISCONNECTING);
                          }

                          if (mTargetDevice != null) {
                              if (!connectHfpNative(getByteAddress(mTargetDevice))) {

                                broadcastConnectionState(mTargetDevice,
                                          BluetoothProfile.STATE_DISCONNECTED,
                                          BluetoothProfile.STATE_CONNECTING);
                                  synchronized (HeadsetStateMachine.this) {
                                      mTargetDevice = null;
                                      if (mConnectedDevicesList.size() == 0) {
                                          // Should be not in this state since it has at least
                                          // one HF connected in MultiHFPending state
                                          Log.d(TAG, "Should be not in this state, error handling");
                                          transitionTo(mDisconnected);
                                      }
                                      else {
                                          processMultiHFConnected(device);
                                      }
                                  }
                              }
                          } else {
                              synchronized (HeadsetStateMachine.this) {
                                  mIncomingDevice = null;
                                  if (mConnectedDevicesList.size() == 0) {
                                      transitionTo(mDisconnected);
                                  }
                                  else {
                                      processMultiHFConnected(device);
                                  }
                              }
                           }
                        } else {
                            /* Another HF disconnected when one HF is connecting */
                            synchronized (HeadsetStateMachine.this) {
                              mConnectedDevicesList.remove(device);
                              mHeadsetAudioParam.remove(device);
                              mHeadsetBrsf.remove(device);
                              Log.d(TAG, "device " + device.getAddress() +
                                           " is removed in MultiHFPending state");
                            }
                            broadcastConnectionState(device,
                                BluetoothProfile.STATE_DISCONNECTED,
                                BluetoothProfile.STATE_CONNECTED);
                        }
                    } else if (mTargetDevice != null && mTargetDevice.equals(device)) {

                        broadcastConnectionState(mTargetDevice, BluetoothProfile.STATE_DISCONNECTED,
                                                 BluetoothProfile.STATE_CONNECTING);
                        synchronized (HeadsetStateMachine.this) {
                            mTargetDevice = null;
                            if (mConnectedDevicesList.size() == 0) {
                                transitionTo(mDisconnected);
                            }
                            else
                            {
                               if (mAudioState == BluetoothHeadset.STATE_AUDIO_CONNECTED)
                                   transitionTo(mAudioOn);
                               else transitionTo(mConnected);
                            }
                        }
                    } else {
                        Log.e(TAG, "Unknown device Disconnected: " + device);
                    }
                    break;
            case HeadsetHalConstants.CONNECTION_STATE_CONNECTED:
                /* Outgoing disconnection for device failed */
                if (mConnectedDevicesList.contains(device)) {

                    broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTED,
                                             BluetoothProfile.STATE_DISCONNECTING);
                    if (mTargetDevice != null) {
                        broadcastConnectionState(mTargetDevice, BluetoothProfile.STATE_DISCONNECTED,
                                                 BluetoothProfile.STATE_CONNECTING);
                    }
                    synchronized (HeadsetStateMachine.this) {
                        mTargetDevice = null;
                        if (mAudioState == BluetoothHeadset.STATE_AUDIO_CONNECTED)
                            transitionTo(mAudioOn);
                        else transitionTo(mConnected);
                    }
                } else if (mTargetDevice != null && mTargetDevice.equals(device)) {

                    synchronized (HeadsetStateMachine.this) {
                            mCurrentDevice = device;
                            mConnectedDevicesList.add(device);
                            Log.d(TAG, "device " + device.getAddress() +
                                      " is added in MultiHFPending state");
                            mTargetDevice = null;
                            if (mAudioState == BluetoothHeadset.STATE_AUDIO_CONNECTED)
                                transitionTo(mAudioOn);
                            else transitionTo(mConnected);
                    }

                    broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTED,
                                             BluetoothProfile.STATE_CONNECTING);
                    configAudioParameters(device);
                } else {
                    Log.w(TAG, "Some other incoming HF connected" +
                                          "in Multi Pending state");
                    if (okToConnect(device) &&
                            (mConnectedDevicesList.size() < max_hf_connections)) {
                        Log.i(TAG,"Incoming Hf accepted");
                        broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTED,
                                         BluetoothProfile.STATE_DISCONNECTED);
                        synchronized (HeadsetStateMachine.this) {
                            if (!mConnectedDevicesList.contains(device)) {
                                mCurrentDevice = device;
                                mConnectedDevicesList.add(device);
                                Log.d(TAG, "device " + device.getAddress() +
                                            " is added in MultiHFPending state");
                            }
                        }
                        configAudioParameters(device);
                    } else {
                        // reject the connection and stay in Pending state itself
                        Log.i(TAG,"Incoming Hf rejected. priority=" +
                                          mService.getPriority(device) +
                                  " bondState=" + device.getBondState());
                        disconnectHfpNative(getByteAddress(device));
                        // the other profile connection should be initiated
                        AdapterService adapterService = AdapterService.getAdapterService();
                        if (adapterService != null) {
                            adapterService.connectOtherProfile(device,
                                          AdapterService.PROFILE_CONN_REJECTED);
                        }
                    }
                }
                break;
            case HeadsetHalConstants.CONNECTION_STATE_SLC_CONNECTED:
                processSlcConnected();
                break;
            case HeadsetHalConstants.CONNECTION_STATE_CONNECTING:
                if (mConnectedDevicesList.contains(device)) {
                    Log.e(TAG, "current device tries to connect back");
                } else if (mTargetDevice != null && mTargetDevice.equals(device)) {
                    if (DBG) {
                        log("Stack and target device are connecting");
                    }
                }
                else if (mIncomingDevice != null && mIncomingDevice.equals(device)) {
                    Log.e(TAG, "Another connecting event on" +
                                              "the incoming device");
                }
                break;
            case HeadsetHalConstants.CONNECTION_STATE_DISCONNECTING:
                if (mConnectedDevicesList.contains(device)) {
                    if (DBG) {
                        log("stack is disconnecting mCurrentDevice");
                    }
                } else if (mTargetDevice != null && mTargetDevice.equals(device)) {
                    Log.e(TAG, "TargetDevice is getting disconnected");
                } else if (mIncomingDevice != null && mIncomingDevice.equals(device)) {
                    Log.e(TAG, "IncomingDevice is getting disconnected");
                } else {
                    Log.e(TAG, "Disconnecting unknow device: " + device);
                }
                break;
            default:
                Log.e(TAG, "Incorrect state: " + state);
                break;
            }
        }

        private void processAudioEvent(int state, BluetoothDevice device) {
            if (!mConnectedDevicesList.contains(device)) {
                Log.e(TAG, "Audio changed on disconnected device: " + device);
                return;
            }

            switch (state) {
                case HeadsetHalConstants.AUDIO_STATE_CONNECTED:
                    if (!isScoAcceptable()) {
                        Log.e(TAG,"Audio Connected without any listener");
                        disconnectAudioNative(getByteAddress(device));
                        break;
                    }
                    mAudioState = BluetoothHeadset.STATE_AUDIO_CONNECTED;
                    setAudioParameters(device); /* Set proper Audio Parameters. */
                    mAudioManager.setBluetoothScoOn(true);
                    mActiveScoDevice = device;
                    broadcastAudioState(device, BluetoothHeadset.STATE_AUDIO_CONNECTED,
                            BluetoothHeadset.STATE_AUDIO_CONNECTING);
                    /* The state should be still in MultiHFPending state when
                       audio connected since other device is still connecting/
                       disconnecting */
                    break;
                case HeadsetHalConstants.AUDIO_STATE_CONNECTING:
                    mAudioState = BluetoothHeadset.STATE_AUDIO_CONNECTING;
                    broadcastAudioState(device, BluetoothHeadset.STATE_AUDIO_CONNECTING,
                                        BluetoothHeadset.STATE_AUDIO_DISCONNECTED);
                    break;
                case HeadsetHalConstants.AUDIO_STATE_DISCONNECTED:
                    if (mAudioState != BluetoothHeadset.STATE_AUDIO_DISCONNECTED) {
                        mAudioState = BluetoothHeadset.STATE_AUDIO_DISCONNECTED;
                        mAudioManager.setBluetoothScoOn(false);
                        broadcastAudioState(device, BluetoothHeadset.STATE_AUDIO_DISCONNECTED,
                                            BluetoothHeadset.STATE_AUDIO_CONNECTED);
                    }
                    /* The state should be still in MultiHFPending state when audio
                       disconnected since other device is still connecting/
                       disconnecting */
                    break;

                default:
                    Log.e(TAG, "Audio State Device: " + device + " bad state: " + state);
                    break;
            }
        }

        private void processSlcConnected() {
            if (mPhoneProxy != null) {
                try {
                    mPhoneProxy.queryPhoneState();
                } catch (RemoteException e) {
                    Log.e(TAG, Log.getStackTraceString(new Throwable()));
                }
            } else {
                Log.e(TAG, "Handsfree phone proxy null for query phone state");
            }
        }


        private void processMultiHFConnected(BluetoothDevice device) {
            log("MultiHFPending state: processMultiHFConnected");
            if (mActiveScoDevice != null && mActiveScoDevice.equals(device)) {
                log ("mActiveScoDevice is disconnected, setting it to null");
                mActiveScoDevice = null;
            }
            /* Assign the current activedevice again if the disconnected
               device equals to the current active device */
            if (mCurrentDevice != null && mCurrentDevice.equals(device)) {
                int deviceSize = mConnectedDevicesList.size();
                mCurrentDevice = mConnectedDevicesList.get(deviceSize-1);
            }
            // The disconnected device is not current active device
            if (mAudioState == BluetoothHeadset.STATE_AUDIO_CONNECTED)
                transitionTo(mAudioOn);
            else transitionTo(mConnected);
            log("processMultiHFConnected , the latest mCurrentDevice is:"
                                            + mCurrentDevice);
            log("MultiHFPending state: processMultiHFConnected ," +
                         "fake broadcasting for mCurrentDevice");
            broadcastConnectionState(mCurrentDevice, BluetoothProfile.STATE_CONNECTED,
                            BluetoothProfile.STATE_DISCONNECTED);
        }

        private void processIntentScoVolume(Intent intent, BluetoothDevice device) {
            int volumeValue = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_VALUE, 0);
            if (mPhoneState.getSpeakerVolume() != volumeValue) {
                mPhoneState.setSpeakerVolume(volumeValue);
                setVolumeNative(HeadsetHalConstants.VOLUME_TYPE_SPK,
                                    volumeValue, getByteAddress(device));
            }
        }
    }


    private ServiceConnection mConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder service) {
            if (DBG) Log.d(TAG, "Proxy object connected");
            mPhoneProxy = IBluetoothHeadsetPhone.Stub.asInterface(service);
        }

        public void onServiceDisconnected(ComponentName className) {
            if (DBG) Log.d(TAG, "Proxy object disconnected");
            mPhoneProxy = null;
        }
    };

    // HFP Connection state of the device could be changed by the state machine
    // in separate thread while this method is executing.
    int getConnectionState(BluetoothDevice device) {
        if (getCurrentState() == mDisconnected) {
            if (DBG) Log.d(TAG, "currentState is Disconnected");
            return BluetoothProfile.STATE_DISCONNECTED;
        }

        synchronized (this) {
            IState currentState = getCurrentState();
            if (DBG) Log.d(TAG, "currentState = " + currentState);
            if (currentState == mPending) {
                if ((mTargetDevice != null) && mTargetDevice.equals(device)) {
                    return BluetoothProfile.STATE_CONNECTING;
                }
                if (mConnectedDevicesList.contains(device)) {
                    return BluetoothProfile.STATE_DISCONNECTING;
                }
                if ((mIncomingDevice != null) && mIncomingDevice.equals(device)) {
                    return BluetoothProfile.STATE_CONNECTING; // incoming connection
                }
                return BluetoothProfile.STATE_DISCONNECTED;
            }

            if (currentState == mMultiHFPending) {
                if ((mTargetDevice != null) && mTargetDevice.equals(device)) {
                    return BluetoothProfile.STATE_CONNECTING;
                }
                if ((mIncomingDevice != null) && mIncomingDevice.equals(device)) {
                    return BluetoothProfile.STATE_CONNECTING; // incoming connection
                }
                if (mConnectedDevicesList.contains(device)) {
                    if ((mMultiDisconnectDevice != null) &&
                            (!mMultiDisconnectDevice.equals(device))) {
                        // The device is still connected
                        return BluetoothProfile.STATE_CONNECTED;
                    }
                    return BluetoothProfile.STATE_DISCONNECTING;
                }
                return BluetoothProfile.STATE_DISCONNECTED;
            }

            if (currentState == mConnected || currentState == mAudioOn) {
                if (mConnectedDevicesList.contains(device)) {
                    return BluetoothProfile.STATE_CONNECTED;
                }
                return BluetoothProfile.STATE_DISCONNECTED;
            } else {
                Log.e(TAG, "Bad currentState: " + currentState);
                return BluetoothProfile.STATE_DISCONNECTED;
            }
        }
    }

    List<BluetoothDevice> getConnectedDevices() {
        List<BluetoothDevice> devices = new ArrayList<BluetoothDevice>();
        synchronized(this) {
            for (int i = 0; i < mConnectedDevicesList.size(); i++)
                devices.add(mConnectedDevicesList.get(i));
            }

        return devices;
    }

    boolean isAudioOn() {
        return (getCurrentState() == mAudioOn);
    }

    boolean isAudioConnected(BluetoothDevice device) {
        synchronized(this) {

            /*  Additional check for audio state included for the case when PhoneApp queries
            Bluetooth Audio state, before we receive the close event from the stack for the
            sco disconnect issued in AudioOn state. This was causing a mismatch in the
            Incall screen UI. */

            if (getCurrentState() == mAudioOn && mCurrentDevice.equals(device)
                && mAudioState != BluetoothHeadset.STATE_AUDIO_DISCONNECTED)
            {
                return true;
            }
        }
        return false;
    }

    public void setAudioRouteAllowed(boolean allowed) {
        mAudioRouteAllowed = allowed;
    }

    public boolean getAudioRouteAllowed() {
        return mAudioRouteAllowed;
    }

    int getAudioState(BluetoothDevice device) {
        synchronized(this) {
            if (mConnectedDevicesList.size() == 0) {
                return BluetoothHeadset.STATE_AUDIO_DISCONNECTED;
            }
        }
        return mAudioState;
    }

    private void processVrEvent(int state, BluetoothDevice device) {

        if(device == null) {
            Log.w(TAG, "processVrEvent device is null");
            return;
        }
        Log.d(TAG, "processVrEvent: state=" + state + " mVoiceRecognitionStarted: " +
            mVoiceRecognitionStarted + " mWaitingforVoiceRecognition: " + mWaitingForVoiceRecognition +
            " isInCall: " + isInCall());
        if (state == HeadsetHalConstants.VR_STATE_STARTED) {
            if (!isVirtualCallInProgress() && !isInCall()) {
                IDeviceIdleController dic = IDeviceIdleController.Stub.asInterface(
                        ServiceManager.getService(Context.DEVICE_IDLE_CONTROLLER));
                if (dic != null) {
                    try {
                        dic.exitIdle("voice-command");
                    } catch (RemoteException e) {
                    }
                }
                try {
                    mService.startActivity(sVoiceCommandIntent);
                } catch (ActivityNotFoundException e) {
                    atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR,
                                        0, getByteAddress(device));
                    return;
                }
                expectVoiceRecognition(device);
            } else {
                // send error response if call is ongoing
                atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR,
                        0, getByteAddress(device));
                return;
            }
        } else if (state == HeadsetHalConstants.VR_STATE_STOPPED) {
            if (mVoiceRecognitionStarted || mWaitingForVoiceRecognition)
            {
                atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_OK,
                                         0, getByteAddress(device));
                mVoiceRecognitionStarted = false;
                mWaitingForVoiceRecognition = false;
                if (!isInCall() && (mActiveScoDevice != null)) {
                    disconnectAudioNative(getByteAddress(mActiveScoDevice));
                    mAudioManager.setParameters("A2dpSuspended=false");
                }
            }
            else
            {
                atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR,
                                        0, getByteAddress(device));
            }
        } else {
            Log.e(TAG, "Bad Voice Recognition state: " + state);
        }
    }

    private void processLocalVrEvent(int state)
    {
        BluetoothDevice device = null;
        if (state == HeadsetHalConstants.VR_STATE_STARTED)
        {
            boolean needAudio = true;
            if (mVoiceRecognitionStarted || isInCall())
            {
                Log.e(TAG, "Voice recognition started when call is active. isInCall:" + isInCall() +
                    " mVoiceRecognitionStarted: " + mVoiceRecognitionStarted);
                return;
            }
            mVoiceRecognitionStarted = true;

            if (mWaitingForVoiceRecognition)
            {
                device = getDeviceForMessage(START_VR_TIMEOUT);
                if (device == null)
                    return;

                Log.d(TAG, "Voice recognition started successfully");
                mWaitingForVoiceRecognition = false;
                atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_OK,
                                        0, getByteAddress(device));
                removeMessages(START_VR_TIMEOUT);
            }
            else
            {
                Log.d(TAG, "Voice recognition started locally");
                needAudio = startVoiceRecognitionNative(getByteAddress(mCurrentDevice));
                if (mCurrentDevice != null)
                    device = mCurrentDevice;
            }

            if (needAudio && !isAudioOn())
            {
                Log.d(TAG, "Initiating audio connection for Voice Recognition");
                // At this stage, we need to be sure that AVDTP is not streaming. This is needed
                // to be compliant with the AV+HFP Whitepaper as we cannot have A2DP in
                // streaming state while a SCO connection is established.
                // This is needed for VoiceDial scenario alone and not for
                // incoming call/outgoing call scenarios as the phone enters MODE_RINGTONE
                // or MODE_IN_CALL which shall automatically suspend the AVDTP stream if needed.
                // Whereas for VoiceDial we want to activate the SCO connection but we are still
                // in MODE_NORMAL and hence the need to explicitly suspend the A2DP stream
                mAudioManager.setParameters("A2dpSuspended=true");
                if (device != null) {
                    connectAudioNative(getByteAddress(device));
                } else {
                    Log.e(TAG, "device not found for VR");
                }
            }

            if (mStartVoiceRecognitionWakeLock.isHeld()) {
                mStartVoiceRecognitionWakeLock.release();
            }
        }
        else
        {
            Log.d(TAG, "Voice Recognition stopped. mVoiceRecognitionStarted: " + mVoiceRecognitionStarted +
                " mWaitingForVoiceRecognition: " + mWaitingForVoiceRecognition);
            if (mVoiceRecognitionStarted || mWaitingForVoiceRecognition)
            {
                mVoiceRecognitionStarted = false;
                mWaitingForVoiceRecognition = false;

                if (stopVoiceRecognitionNative(getByteAddress(mCurrentDevice))
                                && !isInCall() && mActiveScoDevice != null) {
                    disconnectAudioNative(getByteAddress(mActiveScoDevice));
                    mAudioManager.setParameters("A2dpSuspended=false");
                }
            }
        }
    }

    private synchronized void expectVoiceRecognition(BluetoothDevice device) {
        mWaitingForVoiceRecognition = true;
        Message m = obtainMessage(START_VR_TIMEOUT);
        m.obj = getMatchingDevice(device);
        sendMessageDelayed(m, START_VR_TIMEOUT_VALUE);

        if (!mStartVoiceRecognitionWakeLock.isHeld()) {
            mStartVoiceRecognitionWakeLock.acquire(START_VR_TIMEOUT_VALUE);
        }
    }

    List<BluetoothDevice> getDevicesMatchingConnectionStates(int[] states) {
        List<BluetoothDevice> deviceList = new ArrayList<BluetoothDevice>();
        Set<BluetoothDevice> bondedDevices = mAdapter.getBondedDevices();
        int connectionState;
        synchronized (this) {
            for (BluetoothDevice device : bondedDevices) {
                ParcelUuid[] featureUuids = device.getUuids();
                if (!BluetoothUuid.containsAnyUuid(featureUuids, HEADSET_UUIDS)) {
                    continue;
                }
                connectionState = getConnectionState(device);
                for(int i = 0; i < states.length; i++) {
                    if (connectionState == states[i]) {
                        deviceList.add(device);
                    }
                }
            }
        }
        return deviceList;
    }

    private BluetoothDevice getDeviceForMessage(int what)
    {
        if (what == CONNECT_TIMEOUT) {
            log("getDeviceForMessage: returning mTargetDevice for what=" + what);
            return mTargetDevice;
        }
        if (mConnectedDevicesList.size() == 0) {
            log("getDeviceForMessage: No connected device. what=" + what);
            return null;
        }
        for (BluetoothDevice device : mConnectedDevicesList)
        {
            if (getHandler().hasMessages(what, device))
            {
                log("getDeviceForMessage: returning " + device);
                return device;
            }
        }
        log("getDeviceForMessage: No matching device for " + what + ". Returning null");
        return null;
    }

    private BluetoothDevice getMatchingDevice(BluetoothDevice device)
    {
        for (BluetoothDevice matchingDevice : mConnectedDevicesList)
        {
            if (matchingDevice.equals(device))
            {
                return matchingDevice;
            }
        }
        return null;
    }

    // This method does not check for error conditon (newState == prevState)
    private void broadcastConnectionState(BluetoothDevice device, int newState, int prevState) {
        log("Connection state " + device + ": " + prevState + "->" + newState);
        if(prevState == BluetoothProfile.STATE_CONNECTED) {
            // Headset is disconnecting, stop Virtual call if active.
            terminateScoUsingVirtualVoiceCall();
        }

        /* Notifying the connection state change of the profile before sending the intent for
           connection state change, as it was causing a race condition, with the UI not being
           updated with the correct connection state. */
        mService.notifyProfileConnectionStateChanged(device, BluetoothProfile.HEADSET,
                                                     newState, prevState);
        Intent intent = new Intent(BluetoothHeadset.ACTION_CONNECTION_STATE_CHANGED);
        intent.putExtra(BluetoothProfile.EXTRA_PREVIOUS_STATE, prevState);
        intent.putExtra(BluetoothProfile.EXTRA_STATE, newState);
        intent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
        mService.sendBroadcastAsUser(intent, UserHandle.ALL,
                HeadsetService.BLUETOOTH_PERM);
    }

    private void broadcastAudioState(BluetoothDevice device, int newState, int prevState) {
        if(prevState == BluetoothHeadset.STATE_AUDIO_CONNECTED) {
            // When SCO gets disconnected during call transfer, Virtual call
            //needs to be cleaned up.So call terminateScoUsingVirtualVoiceCall.
            terminateScoUsingVirtualVoiceCall();
        }
        Intent intent = new Intent(BluetoothHeadset.ACTION_AUDIO_STATE_CHANGED);
        intent.putExtra(BluetoothProfile.EXTRA_PREVIOUS_STATE, prevState);
        intent.putExtra(BluetoothProfile.EXTRA_STATE, newState);
        intent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
        mService.sendBroadcastAsUser(intent, UserHandle.ALL,
                HeadsetService.BLUETOOTH_PERM);
        log("Audio state " + device + ": " + prevState + "->" + newState);
    }

    /*
     * Put the AT command, company ID, arguments, and device in an Intent and broadcast it.
     */
    private void broadcastVendorSpecificEventIntent(String command,
                                                    int companyId,
                                                    int commandType,
                                                    Object[] arguments,
                                                    BluetoothDevice device) {
        log("broadcastVendorSpecificEventIntent(" + command + ")");
        Intent intent =
                new Intent(BluetoothHeadset.ACTION_VENDOR_SPECIFIC_HEADSET_EVENT);
        intent.putExtra(BluetoothHeadset.EXTRA_VENDOR_SPECIFIC_HEADSET_EVENT_CMD, command);
        intent.putExtra(BluetoothHeadset.EXTRA_VENDOR_SPECIFIC_HEADSET_EVENT_CMD_TYPE,
                        commandType);
        // assert: all elements of args are Serializable
        intent.putExtra(BluetoothHeadset.EXTRA_VENDOR_SPECIFIC_HEADSET_EVENT_ARGS, arguments);
        intent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);

        intent.addCategory(BluetoothHeadset.VENDOR_SPECIFIC_HEADSET_EVENT_COMPANY_ID_CATEGORY
            + "." + Integer.toString(companyId));

        mService.sendBroadcastAsUser(intent, UserHandle.ALL,
                HeadsetService.BLUETOOTH_PERM);
    }

    private void configAudioParameters(BluetoothDevice device)
    {
        // Reset NREC on connect event. Headset will override later
        HashMap<String, Integer> AudioParamConfig = new HashMap<String, Integer>();
        AudioParamConfig.put("NREC", 1);
        mHeadsetAudioParam.put(device, AudioParamConfig);
        mAudioManager.setParameters(HEADSET_NAME + "=" + getCurrentDeviceName(device) + ";" +
                                    HEADSET_NREC + "=on");
        Log.d(TAG, "configAudioParameters for device:" + device + " are: nrec = " +
                      AudioParamConfig.get("NREC"));
    }

    private void setAudioParameters(BluetoothDevice device)
    {
        // 1. update nrec value
        // 2. update headset name
        int mNrec = 0;
        HashMap<String, Integer> AudioParam = mHeadsetAudioParam.get(device);
        if (AudioParam != null && !AudioParam.isEmpty()) {
            mNrec = AudioParam.get("NREC");
        } else {
            Log.e(TAG,"setAudioParameters: AudioParam not found");
        }

        if (mNrec == 1) {
            Log.d(TAG, "Set NREC: 1 for device:" + device);
            mAudioManager.setParameters(HEADSET_NREC + "=on");
        } else {
            Log.d(TAG, "Set NREC: 0 for device:" + device);
            mAudioManager.setParameters(HEADSET_NREC + "=off");
        }
        mAudioManager.setParameters(HEADSET_NAME + "=" + getCurrentDeviceName(device));
    }

    private String parseUnknownAt(String atString)
    {
        StringBuilder atCommand = new StringBuilder(atString.length());
        String result = null;

        for (int i = 0; i < atString.length(); i++) {
            char c = atString.charAt(i);
            if (c == '"') {
                int j = atString.indexOf('"', i + 1 );  // search for closing "
                if (j == -1) {  // unmatched ", insert one.
                    atCommand.append(atString.substring(i, atString.length()));
                    atCommand.append('"');
                    break;
                }
                atCommand.append(atString.substring(i, j + 1));
                i = j;
            } else if (c != ' ') {
                atCommand.append(Character.toUpperCase(c));
            }
        }
        result = atCommand.toString();
        return result;
    }

    private int getAtCommandType(String atCommand)
    {
        int commandType = mPhonebook.TYPE_UNKNOWN;
        String atString = null;
        atCommand = atCommand.trim();
        if (atCommand.length() > 5)
        {
            atString = atCommand.substring(5);
            if (atString.startsWith("?"))     // Read
                commandType = mPhonebook.TYPE_READ;
            else if (atString.startsWith("=?"))   // Test
                commandType = mPhonebook.TYPE_TEST;
            else if (atString.startsWith("="))   // Set
                commandType = mPhonebook.TYPE_SET;
            else
                commandType = mPhonebook.TYPE_UNKNOWN;
        }
        return commandType;
    }

    /* Method to check if Virtual Call in Progress */
    private boolean isVirtualCallInProgress() {
        return mVirtualCallStarted;
    }

    void setVirtualCallInProgress(boolean state) {
        mVirtualCallStarted = state;
    }

    /* NOTE: Currently the VirtualCall API does not support handling of
    call transfers. If it is initiated from the handsfree device,
    HeadsetStateMachine will end the virtual call by calling
    terminateScoUsingVirtualVoiceCall() in broadcastAudioState() */
    synchronized boolean initiateScoUsingVirtualVoiceCall() {
        if (DBG) log("initiateScoUsingVirtualVoiceCall: Received");
        // 1. Check if the SCO state is idle
        if (isInCall() || mVoiceRecognitionStarted) {
            Log.e(TAG, "initiateScoUsingVirtualVoiceCall: Call in progress.");
            return false;
        }

        // 2. Send virtual phone state changed to initialize SCO
        processCallState(new HeadsetCallState(0, 0,
            HeadsetHalConstants.CALL_STATE_DIALING, "", 0), true);
        processCallState(new HeadsetCallState(0, 0,
            HeadsetHalConstants.CALL_STATE_ALERTING, "", 0), true);
        processCallState(new HeadsetCallState(1, 0,
            HeadsetHalConstants.CALL_STATE_IDLE, "", 0), true);
        setVirtualCallInProgress(true);
        // Done
        if (DBG) log("initiateScoUsingVirtualVoiceCall: Done");
        return true;
    }

    synchronized boolean terminateScoUsingVirtualVoiceCall() {
        if (DBG) log("terminateScoUsingVirtualVoiceCall: Received");

        if (!isVirtualCallInProgress()) {
            Log.e(TAG, "terminateScoUsingVirtualVoiceCall:"+
                "No present call to terminate");
            return false;
        }

        // 2. Send virtual phone state changed to close SCO
        processCallState(new HeadsetCallState(0, 0,
            HeadsetHalConstants.CALL_STATE_IDLE, "", 0), true);
        setVirtualCallInProgress(false);
        // Done
        if (DBG) log("terminateScoUsingVirtualVoiceCall: Done");
        return true;
    }

    private void processAnswerCall(BluetoothDevice device) {
        if(device == null) {
            Log.w(TAG, "processAnswerCall device is null");
            return;
        }

        if (mPhoneProxy != null) {
            try {
                mPhoneProxy.answerCall();
            } catch (RemoteException e) {
                Log.e(TAG, Log.getStackTraceString(new Throwable()));
            }
        } else {
            Log.e(TAG, "Handsfree phone proxy null for answering call");
        }
    }

    private void processHangupCall(BluetoothDevice device) {
        if(device == null) {
            Log.w(TAG, "processHangupCall device is null");
            return;
        }
        // Close the virtual call if active. Virtual call should be
        // terminated for CHUP callback event
        if (isVirtualCallInProgress()) {
            terminateScoUsingVirtualVoiceCall();
        } else {
            if (mPhoneProxy != null) {
                try {
                    mPhoneProxy.hangupCall();
                } catch (RemoteException e) {
                    Log.e(TAG, Log.getStackTraceString(new Throwable()));
                }
            } else {
                Log.e(TAG, "Handsfree phone proxy null for hanging up call");
            }
        }
    }

    private void processDialCall(String number, BluetoothDevice device) {
        if(device == null) {
            Log.w(TAG, "processDialCall device is null");
            return;
        }

        String dialNumber;
        if (mDialingOut) {
            if (DBG) log("processDialCall, already dialling");
            atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR, 0,
                                       getByteAddress(device));
            return;
        }
        if ((number == null) || (number.length() == 0)) {
            dialNumber = mPhonebook.getLastDialledNumber();
            if (dialNumber == null) {
                if (DBG) log("processDialCall, last dial number null");
                atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR, 0,
                                       getByteAddress(device));
                return;
            }
        } else if (number.charAt(0) == '>') {
            // Yuck - memory dialling requested.
            // Just dial last number for now
            if (number.startsWith(">9999")) {   // for PTS test
                atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR, 0,
                                       getByteAddress(device));
                return;
            }
            if (DBG) log("processDialCall, memory dial do last dial for now");
            dialNumber = mPhonebook.getLastDialledNumber();
            if (dialNumber == null) {
                if (DBG) log("processDialCall, last dial number null");
                atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR, 0,
                                       getByteAddress(device));
                return;
            }
        } else {
            // Remove trailing ';'
            if (number.charAt(number.length() - 1) == ';') {
                number = number.substring(0, number.length() - 1);
            }

            dialNumber = PhoneNumberUtils.convertPreDial(number);
        }
        // Check for virtual call to terminate before sending Call Intent
        terminateScoUsingVirtualVoiceCall();

        Intent intent = new Intent(Intent.ACTION_CALL_PRIVILEGED,
                                   Uri.fromParts(SCHEME_TEL, dialNumber, null));
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mService.startActivity(intent);
        // TODO(BT) continue send OK reults code after call starts
        //          hold wait lock, start a timer, set wait call flag
        //          Get call started indication from bluetooth phone
        mDialingOut = true;
        Message m = obtainMessage(DIALING_OUT_TIMEOUT);
        m.obj = getMatchingDevice(device);
        sendMessageDelayed(m, DIALING_OUT_TIMEOUT_VALUE);
    }

    private void processVolumeEvent(int volumeType, int volume, BluetoothDevice device) {
        if(device != null && !device.equals(mActiveScoDevice) && mPhoneState.isInCall()) {
            Log.w(TAG, "ignore processVolumeEvent");
            return;
        }

        if (volumeType == HeadsetHalConstants.VOLUME_TYPE_SPK) {
            mPhoneState.setSpeakerVolume(volume);
            int flag = (getCurrentState() == mAudioOn) ? AudioManager.FLAG_SHOW_UI : 0;
            mAudioManager.setStreamVolume(AudioManager.STREAM_BLUETOOTH_SCO, volume, flag);
        } else if (volumeType == HeadsetHalConstants.VOLUME_TYPE_MIC) {
            mPhoneState.setMicVolume(volume);
        } else {
            Log.e(TAG, "Bad voluem type: " + volumeType);
        }
    }

    private void processSendDtmf(int dtmf, BluetoothDevice device) {
        if(device == null) {
            Log.w(TAG, "processSendDtmf device is null");
            return;
        }

        if (mPhoneProxy != null) {
            try {
                mPhoneProxy.sendDtmf(dtmf);
            } catch (RemoteException e) {
                Log.e(TAG, Log.getStackTraceString(new Throwable()));
            }
        } else {
            Log.e(TAG, "Handsfree phone proxy null for sending DTMF");
        }
    }

    private void processCallState(HeadsetCallState callState) {
        processCallState(callState, false);
    }

    private void processCallState(HeadsetCallState callState,
        boolean isVirtualCall) {
        mPhoneState.setNumActiveCall(callState.mNumActive);
        mPhoneState.setNumHeldCall(callState.mNumHeld);
        mPhoneState.setCallState(callState.mCallState);
        if (mDialingOut) {
            if (callState.mCallState ==
                HeadsetHalConstants.CALL_STATE_DIALING) {
                BluetoothDevice device = getDeviceForMessage(DIALING_OUT_TIMEOUT);
                if (device == null) {
                    return;
                }
                atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_OK,
                                                       0, getByteAddress(device));
                removeMessages(DIALING_OUT_TIMEOUT);
            } else if (callState.mCallState ==
                HeadsetHalConstants.CALL_STATE_ACTIVE || callState.mCallState
                == HeadsetHalConstants.CALL_STATE_IDLE) {
                mDialingOut = false;
            }
        }

        /* Set ActiveScoDevice to null when call ends */
        if ((mActiveScoDevice != null) && !isInCall() &&
                callState.mCallState == HeadsetHalConstants.CALL_STATE_IDLE)
            mActiveScoDevice = null;

        log("mNumActive: " + callState.mNumActive + " mNumHeld: " +
            callState.mNumHeld +" mCallState: " + callState.mCallState);
        log("mNumber: " + callState.mNumber + " mType: " + callState.mType);

        if (isVirtualCall) {
            // virtual call state update
            if (getCurrentState() != mDisconnected) {
                phoneStateChangeNative(callState.mNumActive, callState.mNumHeld,
                    callState.mCallState, callState.mNumber, callState.mType);
            }
        } else {
            // circuit-switch voice call update
            // stop virtual voice call if there is a CSV call ongoing
            if (callState.mNumActive > 0 || callState.mNumHeld > 0
                    || callState.mCallState != HeadsetHalConstants.CALL_STATE_IDLE) {
                terminateScoUsingVirtualVoiceCall();
            }

            // Specific handling for case of starting MO/MT call while VOIP
            // ongoing, terminateScoUsingVirtualVoiceCall() resets callState
            // INCOMING/DIALING to IDLE. Some HS send AT+CIND? to read call
            // and get wrong value of callsetup. This case is hit only
            // SCO for VOIP call is not terminated via SDK API call.
            if (mPhoneState.getCallState() != callState.mCallState) {
                mPhoneState.setCallState(callState.mCallState);
            }

            // at this step: if there is virtual call ongoing, it means there is no CSV call
            // let virtual call continue and skip phone state update
            if (!isVirtualCallInProgress()) {
                if (getCurrentState() != mDisconnected) {
                    phoneStateChangeNative(callState.mNumActive, callState.mNumHeld,
                        callState.mCallState, callState.mNumber, callState.mType);
                }
            }
        }
    }

    // 1 enable noice reduction
    // 0 disable noice reduction
    private void processNoiceReductionEvent(int enable, BluetoothDevice device) {
        HashMap<String, Integer> AudioParamNrec = mHeadsetAudioParam.get(device);
        if (AudioParamNrec != null && !AudioParamNrec.isEmpty()) {
            if (enable == 1)
                AudioParamNrec.put("NREC", 1);
            else
                AudioParamNrec.put("NREC", 0);
            log("NREC value for device :" + device + " is: " +
                    AudioParamNrec.get("NREC"));
        } else {
            Log.e(TAG,"processNoiceReductionEvent: AudioParamNrec is null ");
        }

        if (mActiveScoDevice != null && mActiveScoDevice.equals(device)
                && mAudioState == BluetoothHeadset.STATE_AUDIO_CONNECTED) {
            setAudioParameters(device);
        }
    }

    // 2 - WBS on
    // 1 - NBS on
    private void processWBSEvent(int enable, BluetoothDevice device) {
        if (enable == 2) {
            Log.d(TAG, "AudioManager.setParameters bt_wbs=on for " +
                        device.getName() + " - " + device.getAddress());
            mAudioManager.setParameters(HEADSET_WBS + "=on");
        } else {
            Log.d(TAG, "AudioManager.setParameters bt_wbs=off for " +
                        device.getName() + " - " + device.getAddress());
            mAudioManager.setParameters(HEADSET_WBS + "=off");
        }
    }

    private void processAtChld(int chld, BluetoothDevice device) {
        if(device == null) {
            Log.w(TAG, "processAtChld device is null");
            return;
        }

        if (mPhoneProxy != null) {
            try {
                if (mPhoneProxy.processChld(chld)) {
                    atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_OK,
                                               0, getByteAddress(device));
                } else {
                    atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR,
                                               0, getByteAddress(device));
                }
            } catch (RemoteException e) {
                Log.e(TAG, Log.getStackTraceString(new Throwable()));
                atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR,
                                               0, getByteAddress(device));
            }
        } else {
            Log.e(TAG, "Handsfree phone proxy null for At+Chld");
            atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR,
                                               0, getByteAddress(device));
        }
    }

    private void processSubscriberNumberRequest(BluetoothDevice device) {
        if(device == null) {
            Log.w(TAG, "processSubscriberNumberRequest device is null");
            return;
        }

        if (mPhoneProxy != null) {
            try {
                String number = mPhoneProxy.getSubscriberNumber();
                if (number != null) {
                    atResponseStringNative("+CNUM: ,\"" + number + "\"," +
                                                PhoneNumberUtils.toaFromString(number) +
                                                ",,4", getByteAddress(device));
                    atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_OK,
                                                0, getByteAddress(device));
                } else {
                    Log.e(TAG, "getSubscriberNumber returns null");
                    atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR,
                                                0, getByteAddress(device));
                }
            } catch (RemoteException e) {
                Log.e(TAG, Log.getStackTraceString(new Throwable()));
                atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR,
                                                 0, getByteAddress(device));
            }
        } else {
            Log.e(TAG, "Handsfree phone proxy null for At+CNUM");
        }
    }

    private void processAtCind(BluetoothDevice device) {
        int call, call_setup;

        if(device == null) {
            Log.w(TAG, "processAtCind device is null");
            return;
        }

        /* Handsfree carkits expect that +CIND is properly responded to
         Hence we ensure that a proper response is sent
         for the virtual call too.*/
        if (isVirtualCallInProgress()) {
            call = 1;
            call_setup = 0;
        } else {
            // regular phone call
            call = mPhoneState.getNumActiveCall();
            call_setup = mPhoneState.getNumHeldCall();
        }

        cindResponseNative(mPhoneState.getService(), call,
                           call_setup, mPhoneState.getCallState(),
                           mPhoneState.getSignal(), mPhoneState.getRoam(),
                           mPhoneState.getBatteryCharge(), getByteAddress(device));
    }

    private void processAtCops(BluetoothDevice device) {
        if(device == null) {
            Log.w(TAG, "processAtCops device is null");
            return;
        }

        if (mPhoneProxy != null) {
            try {
                String operatorName = mPhoneProxy.getNetworkOperator();
                if (operatorName == null) {
                    operatorName = "";
                }
                copsResponseNative(operatorName, getByteAddress(device));
            } catch (RemoteException e) {
                Log.e(TAG, Log.getStackTraceString(new Throwable()));
                copsResponseNative("", getByteAddress(device));
            }
        } else {
            Log.e(TAG, "Handsfree phone proxy null for At+COPS");
            copsResponseNative("", getByteAddress(device));
        }
    }

    private void processAtClcc(BluetoothDevice device) {
        if(device == null) {
            Log.w(TAG, "processAtClcc device is null");
            return;
        }

        if (mPhoneProxy != null) {
            try {
                if(isVirtualCallInProgress()) {
                    String phoneNumber = "";
                    int type = PhoneNumberUtils.TOA_Unknown;
                    try {
                        phoneNumber = mPhoneProxy.getSubscriberNumber();
                        type = PhoneNumberUtils.toaFromString(phoneNumber);
                    } catch (RemoteException ee) {
                        Log.e(TAG, "Unable to retrieve phone number"+
                            "using IBluetoothHeadsetPhone proxy");
                        phoneNumber = "";
                    }
                    clccResponseNative(1, 0, 0, 0, false, phoneNumber, type,
                                                       getByteAddress(device));
                    clccResponseNative(0, 0, 0, 0, false, "", 0, getByteAddress(device));
                }
                else if (!mPhoneProxy.listCurrentCalls()) {
                    clccResponseNative(0, 0, 0, 0, false, "", 0,
                                                       getByteAddress(device));
                }
                else
                {
                    Log.d(TAG, "Starting CLCC response timeout for device: "
                                                                     + device);
                    Message m = obtainMessage(CLCC_RSP_TIMEOUT);
                    m.obj = getMatchingDevice(device);
                    sendMessageDelayed(m, CLCC_RSP_TIMEOUT_VALUE);
                }
            } catch (RemoteException e) {
                Log.e(TAG, Log.getStackTraceString(new Throwable()));
                clccResponseNative(0, 0, 0, 0, false, "", 0, getByteAddress(device));
            }
        } else {
            Log.e(TAG, "Handsfree phone proxy null for At+CLCC");
            clccResponseNative(0, 0, 0, 0, false, "", 0, getByteAddress(device));
        }
    }

    private void processAtCscs(String atString, int type, BluetoothDevice device) {
        log("processAtCscs - atString = "+ atString);
        if(mPhonebook != null) {
            mPhonebook.handleCscsCommand(atString, type, device);
        }
        else {
            Log.e(TAG, "Phonebook handle null for At+CSCS");
            atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR, 0, getByteAddress(device));
        }
    }

    private void processAtCpbs(String atString, int type, BluetoothDevice device) {
        log("processAtCpbs - atString = "+ atString);
        if(mPhonebook != null) {
            mPhonebook.handleCpbsCommand(atString, type, device);
        }
        else {
            Log.e(TAG, "Phonebook handle null for At+CPBS");
            atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR, 0, getByteAddress(device));
        }
    }

    private void processAtCpbr(String atString, int type, BluetoothDevice device) {
        log("processAtCpbr - atString = "+ atString);
        if(mPhonebook != null) {
            mPhonebook.handleCpbrCommand(atString, type, device);
        }
        else {
            Log.e(TAG, "Phonebook handle null for At+CPBR");
            atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR, 0, getByteAddress(device));
        }
    }

    /**
     * Find a character ch, ignoring quoted sections.
     * Return input.length() if not found.
     */
    static private int findChar(char ch, String input, int fromIndex) {
        for (int i = fromIndex; i < input.length(); i++) {
            char c = input.charAt(i);
            if (c == '"') {
                i = input.indexOf('"', i + 1);
                if (i == -1) {
                    return input.length();
                }
            } else if (c == ch) {
                return i;
            }
        }
        return input.length();
    }

    /**
     * Break an argument string into individual arguments (comma delimited).
     * Integer arguments are turned into Integer objects. Otherwise a String
     * object is used.
     */
    static private Object[] generateArgs(String input) {
        int i = 0;
        int j;
        ArrayList<Object> out = new ArrayList<Object>();
        while (i <= input.length()) {
            j = findChar(',', input, i);

            String arg = input.substring(i, j);
            try {
                out.add(new Integer(arg));
            } catch (NumberFormatException e) {
                out.add(arg);
            }

            i = j + 1; // move past comma
        }
        return out.toArray();
    }

    /**
     * @return {@code true} if the given string is a valid vendor-specific AT command.
     */
    private boolean processVendorSpecificAt(String atString) {
        log("processVendorSpecificAt - atString = " + atString);

        // Currently we accept only SET type commands.
        int indexOfEqual = atString.indexOf("=");
        if (indexOfEqual == -1) {
            Log.e(TAG, "processVendorSpecificAt: command type error in " + atString);
            return false;
        }

        String command = atString.substring(0, indexOfEqual);
        Integer companyId = VENDOR_SPECIFIC_AT_COMMAND_COMPANY_ID.get(command);
        if (companyId == null) {
            Log.e(TAG, "processVendorSpecificAt: unsupported command: " + atString);
            return false;
        }

        String arg = atString.substring(indexOfEqual + 1);
        if (arg.startsWith("?")) {
            Log.e(TAG, "processVendorSpecificAt: command type error in " + atString);
            return false;
        }

        Object[] args = generateArgs(arg);
        broadcastVendorSpecificEventIntent(command,
                                           companyId,
                                           BluetoothHeadset.AT_CMD_TYPE_SET,
                                           args,
                                           mCurrentDevice);
        atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_OK, 0, getByteAddress(mCurrentDevice));
        return true;
    }

    private void processUnknownAt(String atString, BluetoothDevice device) {
        if(device == null) {
            Log.w(TAG, "processUnknownAt device is null");
            return;
        }

        // TODO (BT)
        log("processUnknownAt - atString = "+ atString);
        String atCommand = parseUnknownAt(atString);
        int commandType = getAtCommandType(atCommand);
        if (atCommand.startsWith("+CSCS"))
            processAtCscs(atCommand.substring(5), commandType, device);
        else if (atCommand.startsWith("+CPBS"))
            processAtCpbs(atCommand.substring(5), commandType, device);
        else if (atCommand.startsWith("+CPBR"))
            processAtCpbr(atCommand.substring(5), commandType, device);
        else if (!processVendorSpecificAt(atCommand))
            atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR, 0, getByteAddress(device));
    }

    private void processKeyPressed(BluetoothDevice device) {
        if(device == null) {
            Log.w(TAG, "processKeyPressed device is null");
            return;
        }

        if (mPhoneState.getCallState() == HeadsetHalConstants.CALL_STATE_INCOMING) {
            if (mPhoneProxy != null) {
                try {
                    mPhoneProxy.answerCall();
                } catch (RemoteException e) {
                    Log.e(TAG, Log.getStackTraceString(new Throwable()));
                }
            } else {
                Log.e(TAG, "Handsfree phone proxy null for answering call");
            }
        } else if (mPhoneState.getNumActiveCall() > 0) {
            if (!isAudioOn())
            {
                connectAudioNative(getByteAddress(mCurrentDevice));
            }
            else
            {
                if (mPhoneProxy != null) {
                    try {
                        mPhoneProxy.hangupCall();
                    } catch (RemoteException e) {
                        Log.e(TAG, Log.getStackTraceString(new Throwable()));
                    }
                } else {
                    Log.e(TAG, "Handsfree phone proxy null for hangup call");
                }
            }
        } else {
            String dialNumber = mPhonebook.getLastDialledNumber();
            if (dialNumber == null) {
                if (DBG) log("processKeyPressed, last dial number null");
                return;
            }
            Intent intent = new Intent(Intent.ACTION_CALL_PRIVILEGED,
                                       Uri.fromParts(SCHEME_TEL, dialNumber, null));
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            mService.startActivity(intent);
        }
    }

    private void onConnectionStateChanged(int state, byte[] address) {
        StackEvent event = new StackEvent(EVENT_TYPE_CONNECTION_STATE_CHANGED);
        event.valueInt = state;
        event.device = getDevice(address);
        sendMessage(STACK_EVENT, event);
    }

    private void onAudioStateChanged(int state, byte[] address) {
        StackEvent event = new StackEvent(EVENT_TYPE_AUDIO_STATE_CHANGED);
        event.valueInt = state;
        event.device = getDevice(address);
        sendMessage(STACK_EVENT, event);
    }

    private void onVrStateChanged(int state, byte[] address) {
        StackEvent event = new StackEvent(EVENT_TYPE_VR_STATE_CHANGED);
        event.valueInt = state;
        event.device = getDevice(address);
        sendMessage(STACK_EVENT, event);
    }

    private void onAnswerCall(byte[] address) {
        StackEvent event = new StackEvent(EVENT_TYPE_ANSWER_CALL);
        event.device = getDevice(address);
        sendMessage(STACK_EVENT, event);
    }

    private void onHangupCall(byte[] address) {
        StackEvent event = new StackEvent(EVENT_TYPE_HANGUP_CALL);
        event.device = getDevice(address);
        sendMessage(STACK_EVENT, event);
    }

    private void onVolumeChanged(int type, int volume, byte[] address) {
        StackEvent event = new StackEvent(EVENT_TYPE_VOLUME_CHANGED);
        event.valueInt = type;
        event.valueInt2 = volume;
        event.device = getDevice(address);
        sendMessage(STACK_EVENT, event);
    }

    private void onDialCall(String number, byte[] address) {
        StackEvent event = new StackEvent(EVENT_TYPE_DIAL_CALL);
        event.valueString = number;
        event.device = getDevice(address);
        sendMessage(STACK_EVENT, event);
    }

    private void onSendDtmf(int dtmf, byte[] address) {
        StackEvent event = new StackEvent(EVENT_TYPE_SEND_DTMF);
        event.valueInt = dtmf;
        event.device = getDevice(address);
        sendMessage(STACK_EVENT, event);
    }

    private void onNoiceReductionEnable(boolean enable,  byte[] address) {
        StackEvent event = new StackEvent(EVENT_TYPE_NOICE_REDUCTION);
        event.valueInt = enable ? 1 : 0;
        event.device = getDevice(address);
        sendMessage(STACK_EVENT, event);
    }

    private void onWBS(int codec, byte[] address) {
        StackEvent event = new StackEvent(EVENT_TYPE_WBS);
        event.valueInt = codec;
        event.device = getDevice(address);
        sendMessage(STACK_EVENT, event);
    }

    private void onAtChld(int chld, byte[] address) {
        StackEvent event = new StackEvent(EVENT_TYPE_AT_CHLD);
        event.valueInt = chld;
        event.device = getDevice(address);
        sendMessage(STACK_EVENT, event);
    }

    private void onAtCnum(byte[] address) {
        StackEvent event = new StackEvent(EVENT_TYPE_SUBSCRIBER_NUMBER_REQUEST);
        event.device = getDevice(address);
        sendMessage(STACK_EVENT, event);
    }

    private void onAtCind(byte[] address) {
        StackEvent event = new StackEvent(EVENT_TYPE_AT_CIND);
        event.device = getDevice(address);
        sendMessage(STACK_EVENT, event);
    }

    private void onAtCops(byte[] address) {
        StackEvent event = new StackEvent(EVENT_TYPE_AT_COPS);
        event.device = getDevice(address);
        sendMessage(STACK_EVENT, event);
    }

    private void onAtClcc(byte[] address) {
        StackEvent event = new StackEvent(EVENT_TYPE_AT_CLCC);
        event.device = getDevice(address);
        sendMessage(STACK_EVENT, event);
    }

    private void onUnknownAt(String atString, byte[] address) {
        StackEvent event = new StackEvent(EVENT_TYPE_UNKNOWN_AT);
        event.valueString = atString;
        event.device = getDevice(address);
        sendMessage(STACK_EVENT, event);
    }

    private void onKeyPressed(byte[] address) {
        StackEvent event = new StackEvent(EVENT_TYPE_KEY_PRESSED);
        event.device = getDevice(address);
        sendMessage(STACK_EVENT, event);
    }

    private void processIntentBatteryChanged(Intent intent) {
        int batteryLevel = intent.getIntExtra("level", -1);
        int scale = intent.getIntExtra("scale", -1);
        if (batteryLevel == -1 || scale == -1 || scale == 0) {
            Log.e(TAG, "Bad Battery Changed intent: " + batteryLevel + "," + scale);
            return;
        }
        batteryLevel = batteryLevel * 5 / scale;
        mPhoneState.setBatteryCharge(batteryLevel);
    }

    private void processDeviceStateChanged(HeadsetDeviceState deviceState) {
        notifyDeviceStatusNative(deviceState.mService, deviceState.mRoam, deviceState.mSignal,
                                 deviceState.mBatteryCharge);
    }

    private void processSendClccResponse(HeadsetClccResponse clcc) {
        BluetoothDevice device = getDeviceForMessage(CLCC_RSP_TIMEOUT);
        if (device == null) {
            return;
        }
        if (clcc.mIndex == 0) {
            removeMessages(CLCC_RSP_TIMEOUT);
        }
        clccResponseNative(clcc.mIndex, clcc.mDirection, clcc.mStatus, clcc.mMode, clcc.mMpty,
                           clcc.mNumber, clcc.mType, getByteAddress(device));
    }

    private void processSendVendorSpecificResultCode(HeadsetVendorSpecificResultCode resultCode) {
        String stringToSend = resultCode.mCommand + ": ";
        if (resultCode.mArg != null) {
            stringToSend += resultCode.mArg;
        }
        atResponseStringNative(stringToSend, getByteAddress(resultCode.mDevice));
    }

    private String getCurrentDeviceName(BluetoothDevice device) {
        String defaultName = "<unknown>";

        if(device == null) {
            return defaultName;
        }

        String deviceName = device.getName();
        if (deviceName == null) {
            return defaultName;
        }
        return deviceName;
    }

    private byte[] getByteAddress(BluetoothDevice device) {
        return Utils.getBytesFromAddress(device.getAddress());
    }

    private BluetoothDevice getDevice(byte[] address) {
        return mAdapter.getRemoteDevice(Utils.getAddressStringFromByte(address));
    }

    private boolean isInCall() {
        return ((mPhoneState.getNumActiveCall() > 0) || (mPhoneState.getNumHeldCall() > 0) ||
                (mPhoneState.getCallState() != HeadsetHalConstants.CALL_STATE_IDLE));
    }

    // Accept incoming SCO only when there is active call, VR activated,
    // active VOIP call
    private boolean isScoAcceptable() {
        return mAudioRouteAllowed && (mVoiceRecognitionStarted || isInCall());
    }

    boolean isConnected() {
        IState currentState = getCurrentState();
        return (currentState == mConnected || currentState == mAudioOn);
    }

    boolean okToConnect(BluetoothDevice device) {
        AdapterService adapterService = AdapterService.getAdapterService();
        int priority = mService.getPriority(device);
        boolean ret = false;
        //check if this is an incoming connection in Quiet mode.
        if((adapterService == null) ||
           ((adapterService.isQuietModeEnabled() == true) &&
           (mTargetDevice == null))){
            ret = false;
        }
        // check priority and accept or reject the connection. if priority is undefined
        // it is likely that our SDP has not completed and peer is initiating the
        // connection. Allow this connection, provided the device is bonded
        else if((BluetoothProfile.PRIORITY_OFF < priority) ||
                ((BluetoothProfile.PRIORITY_UNDEFINED == priority) &&
                (device.getBondState() != BluetoothDevice.BOND_NONE))){
            ret= true;
        }
        return ret;
    }

    @Override
    protected void log(String msg) {
        if (DBG) {
            super.log(msg);
        }
    }

    public void handleAccessPermissionResult(Intent intent) {
        log("handleAccessPermissionResult");
        BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
        if (mPhonebook != null) {
            if (!mPhonebook.getCheckingAccessPermission()) {
                return;
            }
            int atCommandResult = 0;
            int atCommandErrorCode = 0;
            //HeadsetBase headset = mHandsfree.getHeadset();
            // ASSERT: (headset != null) && headSet.isConnected()
            // REASON: mCheckingAccessPermission is true, otherwise resetAtState
            // has set mCheckingAccessPermission to false
            if (intent.getAction().equals(BluetoothDevice.ACTION_CONNECTION_ACCESS_REPLY)) {
                if (intent.getIntExtra(BluetoothDevice.EXTRA_CONNECTION_ACCESS_RESULT,
                                       BluetoothDevice.CONNECTION_ACCESS_NO)
                        == BluetoothDevice.CONNECTION_ACCESS_YES) {
                    if (intent.getBooleanExtra(BluetoothDevice.EXTRA_ALWAYS_ALLOWED, false)) {
                        mCurrentDevice.setPhonebookAccessPermission(BluetoothDevice.ACCESS_ALLOWED);
                    }
                    atCommandResult = mPhonebook.processCpbrCommand(device);
                } else {
                    if (intent.getBooleanExtra(BluetoothDevice.EXTRA_ALWAYS_ALLOWED, false)) {
                        mCurrentDevice.setPhonebookAccessPermission(
                                BluetoothDevice.ACCESS_REJECTED);
                    }
                }
            }
            mPhonebook.setCpbrIndex(-1);
            mPhonebook.setCheckingAccessPermission(false);

            if (atCommandResult >= 0) {
                atResponseCodeNative(atCommandResult, atCommandErrorCode, getByteAddress(device));
            } else {
                log("handleAccessPermissionResult - RESULT_NONE");
            }
        } else {
            Log.e(TAG, "Phonebook handle null");
            if (device != null) {
                atResponseCodeNative(HeadsetHalConstants.AT_RESPONSE_ERROR, 0,
                                     getByteAddress(device));
            }
        }
    }

    private static final String SCHEME_TEL = "tel";

    // Event types for STACK_EVENT message
    final private static int EVENT_TYPE_NONE = 0;
    final private static int EVENT_TYPE_CONNECTION_STATE_CHANGED = 1;
    final private static int EVENT_TYPE_AUDIO_STATE_CHANGED = 2;
    final private static int EVENT_TYPE_VR_STATE_CHANGED = 3;
    final private static int EVENT_TYPE_ANSWER_CALL = 4;
    final private static int EVENT_TYPE_HANGUP_CALL = 5;
    final private static int EVENT_TYPE_VOLUME_CHANGED = 6;
    final private static int EVENT_TYPE_DIAL_CALL = 7;
    final private static int EVENT_TYPE_SEND_DTMF = 8;
    final private static int EVENT_TYPE_NOICE_REDUCTION = 9;
    final private static int EVENT_TYPE_AT_CHLD = 10;
    final private static int EVENT_TYPE_SUBSCRIBER_NUMBER_REQUEST = 11;
    final private static int EVENT_TYPE_AT_CIND = 12;
    final private static int EVENT_TYPE_AT_COPS = 13;
    final private static int EVENT_TYPE_AT_CLCC = 14;
    final private static int EVENT_TYPE_UNKNOWN_AT = 15;
    final private static int EVENT_TYPE_KEY_PRESSED = 16;
    final private static int EVENT_TYPE_WBS = 17;

    private class StackEvent {
        int type = EVENT_TYPE_NONE;
        int valueInt = 0;
        int valueInt2 = 0;
        String valueString = null;
        BluetoothDevice device = null;

        private StackEvent(int type) {
            this.type = type;
        }
    }

    /*package*/native boolean atResponseCodeNative(int responseCode, int errorCode,
                                                                          byte[] address);
    /*package*/ native boolean atResponseStringNative(String responseString, byte[] address);

    private native static void classInitNative();
    private native void initializeNative(int max_hf_clients);
    private native void cleanupNative();
    private native boolean connectHfpNative(byte[] address);
    private native boolean disconnectHfpNative(byte[] address);
    private native boolean connectAudioNative(byte[] address);
    private native boolean disconnectAudioNative(byte[] address);
    private native boolean startVoiceRecognitionNative(byte[] address);
    private native boolean stopVoiceRecognitionNative(byte[] address);
    private native boolean setVolumeNative(int volumeType, int volume, byte[] address);
    private native boolean cindResponseNative(int service, int numActive, int numHeld,
                                              int callState, int signal, int roam,
                                              int batteryCharge, byte[] address);
    private native boolean notifyDeviceStatusNative(int networkState, int serviceType, int signal,
                                                    int batteryCharge);

    private native boolean clccResponseNative(int index, int dir, int status, int mode,
                                              boolean mpty, String number, int type,
                                                                           byte[] address);
    private native boolean copsResponseNative(String operatorName, byte[] address);

    private native boolean phoneStateChangeNative(int numActive, int numHeld, int callState,
                                                  String number, int type);
    private native boolean configureWBSNative(byte[] address,int condec_config);
}
