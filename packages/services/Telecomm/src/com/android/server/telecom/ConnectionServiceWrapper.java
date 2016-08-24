/*
 * Copyright 2014, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.server.telecom;

import android.content.ComponentName;
import android.content.Context;
import android.net.Uri;
import android.os.Binder;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.UserHandle;
import android.telecom.CallAudioState;
import android.telecom.Connection;
import android.telecom.ConnectionRequest;
import android.telecom.ConnectionService;
import android.telecom.DisconnectCause;
import android.telecom.GatewayInfo;
import android.telecom.ParcelableConference;
import android.telecom.ParcelableConnection;
import android.telecom.PhoneAccountHandle;
import android.telecom.StatusHints;
import android.telecom.TelecomManager;
import android.telecom.VideoProfile;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.telecom.IConnectionService;
import com.android.internal.telecom.IConnectionServiceAdapter;
import com.android.internal.telecom.IVideoProvider;
import com.android.internal.telecom.RemoteServiceCallback;
import com.android.internal.util.Preconditions;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Wrapper for {@link IConnectionService}s, handles binding to {@link IConnectionService} and keeps
 * track of when the object can safely be unbound. Other classes should not use
 * {@link IConnectionService} directly and instead should use this class to invoke methods of
 * {@link IConnectionService}.
 */
@VisibleForTesting
public class ConnectionServiceWrapper extends ServiceBinder {

    private final class Adapter extends IConnectionServiceAdapter.Stub {

        @Override
        public void handleCreateConnectionComplete(String callId, ConnectionRequest request,
                ParcelableConnection connection) {
            Log.startSession("CSW.hCCC");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("handleCreateConnectionComplete %s", callId);
                    ConnectionServiceWrapper.this
                            .handleCreateConnectionComplete(callId, request, connection);
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void setActive(String callId) {
            Log.startSession("CSW.sA");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("setActive %s", callId);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        mCallsManager.markCallAsActive(call);
                    } else {
                        // Log.w(this, "setActive, unknown call id: %s", msg.obj);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void setRinging(String callId) {
            Log.startSession("CSW.sR");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("setRinging %s", callId);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        mCallsManager.markCallAsRinging(call);
                    } else {
                        // Log.w(this, "setRinging, unknown call id: %s", msg.obj);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void setVideoProvider(String callId, IVideoProvider videoProvider) {
            Log.startSession("CSW.sVP");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("setVideoProvider %s", callId);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        call.setVideoProvider(videoProvider);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void setDialing(String callId) {
            Log.startSession("CSW.sD");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("setDialing %s", callId);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        mCallsManager.markCallAsDialing(call);
                    } else {
                        // Log.w(this, "setDialing, unknown call id: %s", msg.obj);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void setDisconnected(String callId, DisconnectCause disconnectCause) {
            Log.startSession("CSW.sD");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("setDisconnected %s %s", callId, disconnectCause);
                    Call call = mCallIdMapper.getCall(callId);
                    Log.d(this, "disconnect call %s %s", disconnectCause, call);
                    if (call != null) {
                        mCallsManager.markCallAsDisconnected(call, disconnectCause);
                    } else {
                        // Log.w(this, "setDisconnected, unknown call id: %s", args.arg1);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void setOnHold(String callId) {
            Log.startSession("CSW.sOH");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("setOnHold %s", callId);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        mCallsManager.markCallAsOnHold(call);
                    } else {
                        // Log.w(this, "setOnHold, unknown call id: %s", msg.obj);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void setRingbackRequested(String callId, boolean ringback) {
            Log.startSession("CSW.SRR");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("setRingbackRequested %s %b", callId, ringback);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        call.setRingbackRequested(ringback);
                    } else {
                        // Log.w(this, "setRingback, unknown call id: %s", args.arg1);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void removeCall(String callId) {
            Log.startSession("CSW.rC");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("removeCall %s", callId);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        if (call.isAlive()) {
                            mCallsManager.markCallAsDisconnected(
                                    call, new DisconnectCause(DisconnectCause.REMOTE));
                        } else {
                            mCallsManager.markCallAsRemoved(call);
                        }
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void setConnectionCapabilities(String callId, int connectionCapabilities) {
            Log.startSession("CSW.sCC");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("setConnectionCapabilities %s %d", callId, connectionCapabilities);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        call.setConnectionCapabilities(connectionCapabilities);
                    } else {
                        // Log.w(ConnectionServiceWrapper.this,
                        // "setConnectionCapabilities, unknown call id: %s", msg.obj);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void setConnectionProperties(String callId, int connectionProperties) {
            Log.startSession("CSW.sCP");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("setConnectionProperties %s %d", callId, connectionProperties);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        call.setConnectionProperties(connectionProperties);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void setIsConferenced(String callId, String conferenceCallId) {
            Log.startSession("CSW.sIC");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("setIsConferenced %s %s", callId, conferenceCallId);
                    Call childCall = mCallIdMapper.getCall(callId);
                    if (childCall != null) {
                        if (conferenceCallId == null) {
                            Log.d(this, "unsetting parent: %s", conferenceCallId);
                            childCall.setParentCall(null);
                        } else {
                            Call conferenceCall = mCallIdMapper.getCall(conferenceCallId);
                            childCall.setParentCall(conferenceCall);
                        }
                    } else {
                        // Log.w(this, "setIsConferenced, unknown call id: %s", args.arg1);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void setConferenceMergeFailed(String callId) {
            Log.startSession("CSW.sCMF");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("setConferenceMergeFailed %s", callId);
                    // TODO: we should move the UI for indication a merge failure here
                    // from CallNotifier.onSuppServiceFailed(). This way the InCallUI can
                    // deliver the message anyway that they want. b/20530631.
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        // Just refresh the connection capabilities so that the UI
                        // is forced to reenable the merge button as the capability
                        // is still on the connection. Note when b/20530631 is fixed, we need
                        // to revisit this fix to remove this hacky way of unhiding the merge
                        // button (side effect of reprocessing the capabilities) and plumb
                        // the failure event all the way to InCallUI instead of stopping
                        // it here. That way we can also handle the UI of notifying that
                        // the merged has failed.
                        call.setConnectionCapabilities(call.getConnectionCapabilities(), true);
                    } else {
                        Log.w(this, "setConferenceMergeFailed, unknown call id: %s", callId);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void addConferenceCall(String callId, ParcelableConference parcelableConference) {
            Log.startSession("CSW.aCC");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    if (mCallIdMapper.getCall(callId) != null) {
                        Log.w(this, "Attempting to add a conference call using an existing " +
                                "call id %s", callId);
                        return;
                    }

                    // Make sure that there's at least one valid call. For remote connections
                    // we'll get a add conference msg from both the remote connection service
                    // and from the real connection service.
                    boolean hasValidCalls = false;
                    for (String connId : parcelableConference.getConnectionIds()) {
                        if (mCallIdMapper.getCall(connId) != null) {
                            hasValidCalls = true;
                        }
                    }
                    // But don't bail out if the connection count is 0, because that is a valid
                    // IMS conference state.
                    if (!hasValidCalls && parcelableConference.getConnectionIds().size() > 0) {
                        Log.d(this, "Attempting to add a conference with no valid calls");
                        return;
                    }

                    // need to create a new Call
                    PhoneAccountHandle phAcc = null;
                    if (parcelableConference != null &&
                            parcelableConference.getPhoneAccount() != null) {
                        phAcc = parcelableConference.getPhoneAccount();
                    }
                    Call conferenceCall = mCallsManager.createConferenceCall(callId,
                            phAcc, parcelableConference);
                    mCallIdMapper.addCall(conferenceCall, callId);
                    conferenceCall.setConnectionService(ConnectionServiceWrapper.this);

                    Log.d(this, "adding children to conference %s phAcc %s",
                            parcelableConference.getConnectionIds(), phAcc);
                    for (String connId : parcelableConference.getConnectionIds()) {
                        Call childCall = mCallIdMapper.getCall(connId);
                        Log.d(this, "found child: %s", connId);
                        if (childCall != null) {
                            childCall.setParentCall(conferenceCall);
                        }
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void onPostDialWait(String callId, String remaining) throws RemoteException {
            Log.startSession("CSW.oPDW");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("onPostDialWait %s %s", callId, remaining);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        call.onPostDialWait(remaining);
                    } else {
                        // Log.w(this, "onPostDialWait, unknown call id: %s", args.arg1);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void onPostDialChar(String callId, char nextChar) throws RemoteException {
            Log.startSession("CSW.oPDC");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("onPostDialChar %s %s", callId, nextChar);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        call.onPostDialChar(nextChar);
                    } else {
                        // Log.w(this, "onPostDialChar, unknown call id: %s", args.arg1);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void queryRemoteConnectionServices(RemoteServiceCallback callback) {
            final UserHandle callingUserHandle = Binder.getCallingUserHandle();
            Log.startSession("CSW.qRCS");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("queryRemoteConnectionServices %s", callback);
                    ConnectionServiceWrapper.this
                            .queryRemoteConnectionServices(callingUserHandle, callback);
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void setVideoState(String callId, int videoState) {
            Log.startSession("CSW.sVS");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("setVideoState %s %d", callId, videoState);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        call.setVideoState(videoState);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void setIsVoipAudioMode(String callId, boolean isVoip) {
            Log.startSession("CSW.sIVAM");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("setIsVoipAudioMode %s %b", callId, isVoip);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        call.setIsVoipAudioMode(isVoip);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void setStatusHints(String callId, StatusHints statusHints) {
            Log.startSession("CSW.sSH");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("setStatusHints %s %s", callId, statusHints);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        call.setStatusHints(statusHints);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void putExtras(String callId, Bundle extras) {
            Log.startSession("CSW.pE");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    Bundle.setDefusable(extras, true);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        call.putExtras(Call.SOURCE_CONNECTION_SERVICE, extras);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void removeExtras(String callId, List<String> keys) {
            Log.startSession("CSW.rE");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("removeExtra %s %s", callId, keys);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        call.removeExtras(Call.SOURCE_CONNECTION_SERVICE, keys);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void setAddress(String callId, Uri address, int presentation) {
            Log.startSession("CSW.sA");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("setAddress %s %s %d", callId, address, presentation);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        call.setHandle(address, presentation);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void setCallerDisplayName(
                String callId, String callerDisplayName, int presentation) {
            Log.startSession("CSW.sCDN");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("setCallerDisplayName %s %s %d", callId, callerDisplayName,
                            presentation);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        call.setCallerDisplayName(callerDisplayName, presentation);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void setConferenceableConnections(
                String callId, List<String> conferenceableCallIds) {
            Log.startSession("CSW.sCC");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("setConferenceableConnections %s %s", callId,
                            conferenceableCallIds);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        List<Call> conferenceableCalls =
                                new ArrayList<>(conferenceableCallIds.size());
                        for (String otherId : conferenceableCallIds) {
                            Call otherCall = mCallIdMapper.getCall(otherId);
                            if (otherCall != null && otherCall != call) {
                                conferenceableCalls.add(otherCall);
                            }
                        }
                        call.setConferenceableCalls(conferenceableCalls);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void addExistingConnection(String callId, ParcelableConnection connection) {
            Log.startSession("CSW.aEC");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    logIncoming("addExistingConnection  %s %s", callId, connection);
                    Call existingCall = mCallsManager
                            .createCallForExistingConnection(callId, connection);
                    mCallIdMapper.addCall(existingCall, callId);
                    existingCall.setConnectionService(ConnectionServiceWrapper.this);
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }

        @Override
        public void onConnectionEvent(String callId, String event, Bundle extras) {
            Log.startSession("CSW.oCE");
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (mLock) {
                    Bundle.setDefusable(extras, true);
                    Call call = mCallIdMapper.getCall(callId);
                    if (call != null) {
                        call.onConnectionEvent(event, extras);
                    }
                }
            } finally {
                Binder.restoreCallingIdentity(token);
                Log.endSession();
            }
        }
    }

    private final Adapter mAdapter = new Adapter();
    private final CallIdMapper mCallIdMapper = new CallIdMapper();
    private final Map<String, CreateConnectionResponse> mPendingResponses = new HashMap<>();

    private Binder2 mBinder = new Binder2();
    private IConnectionService mServiceInterface;
    private final ConnectionServiceRepository mConnectionServiceRepository;
    private final PhoneAccountRegistrar mPhoneAccountRegistrar;
    private final CallsManager mCallsManager;

    /**
     * Creates a connection service.
     *
     * @param componentName The component name of the service with which to bind.
     * @param connectionServiceRepository Connection service repository.
     * @param phoneAccountRegistrar Phone account registrar
     * @param callsManager Calls manager
     * @param context The context.
     * @param userHandle The {@link UserHandle} to use when binding.
     */
    ConnectionServiceWrapper(
            ComponentName componentName,
            ConnectionServiceRepository connectionServiceRepository,
            PhoneAccountRegistrar phoneAccountRegistrar,
            CallsManager callsManager,
            Context context,
            TelecomSystem.SyncRoot lock,
            UserHandle userHandle) {
        super(ConnectionService.SERVICE_INTERFACE, componentName, context, lock, userHandle);
        mConnectionServiceRepository = connectionServiceRepository;
        phoneAccountRegistrar.addListener(new PhoneAccountRegistrar.Listener() {
            // TODO -- Upon changes to PhoneAccountRegistrar, need to re-wire connections
            // To do this, we must proxy remote ConnectionService objects
        });
        mPhoneAccountRegistrar = phoneAccountRegistrar;
        mCallsManager = callsManager;
    }

    /** See {@link IConnectionService#addConnectionServiceAdapter}. */
    private void addConnectionServiceAdapter(IConnectionServiceAdapter adapter) {
        if (isServiceValid("addConnectionServiceAdapter")) {
            try {
                logOutgoing("addConnectionServiceAdapter %s", adapter);
                mServiceInterface.addConnectionServiceAdapter(adapter);
            } catch (RemoteException e) {
            }
        }
    }

    /**
     * Creates a new connection for a new outgoing call or to attach to an existing incoming call.
     */
    @VisibleForTesting
    public void createConnection(final Call call, final CreateConnectionResponse response) {
        Log.d(this, "createConnection(%s) via %s.", call, getComponentName());
        BindCallback callback = new BindCallback() {
            @Override
            public void onSuccess() {
                String callId = mCallIdMapper.getCallId(call);
                mPendingResponses.put(callId, response);

                GatewayInfo gatewayInfo = call.getGatewayInfo();
                Bundle extras = call.getIntentExtras();
                if (gatewayInfo != null && gatewayInfo.getGatewayProviderPackageName() != null &&
                        gatewayInfo.getOriginalAddress() != null) {
                    extras = (Bundle) extras.clone();
                    extras.putString(
                            TelecomManager.GATEWAY_PROVIDER_PACKAGE,
                            gatewayInfo.getGatewayProviderPackageName());
                    extras.putParcelable(
                            TelecomManager.GATEWAY_ORIGINAL_ADDRESS,
                            gatewayInfo.getOriginalAddress());
                }

                Log.event(call, Log.Events.START_CONNECTION, Log.piiHandle(call.getHandle()));
                try {
                    mServiceInterface.createConnection(
                            call.getConnectionManagerPhoneAccount(),
                            callId,
                            new ConnectionRequest(
                                    call.getTargetPhoneAccount(),
                                    call.getHandle(),
                                    extras,
                                    call.getVideoState(),
                                    callId),
                            call.shouldAttachToExistingConnection(),
                            call.isUnknown());
                } catch (RemoteException e) {
                    Log.e(this, e, "Failure to createConnection -- %s", getComponentName());
                    mPendingResponses.remove(callId).handleCreateConnectionFailure(
                            new DisconnectCause(DisconnectCause.ERROR, e.toString()));
                }
            }

            @Override
            public void onFailure() {
                Log.e(this, new Exception(), "Failure to call %s", getComponentName());
                response.handleCreateConnectionFailure(new DisconnectCause(DisconnectCause.ERROR));
            }
        };

        mBinder.bind(callback, call);
    }

    /** @see IConnectionService#abort(String) */
    void abort(Call call) {
        // Clear out any pending outgoing call data
        final String callId = mCallIdMapper.getCallId(call);

        // If still bound, tell the connection service to abort.
        if (callId != null && isServiceValid("abort")) {
            try {
                logOutgoing("abort %s", callId);
                mServiceInterface.abort(callId);
            } catch (RemoteException e) {
            }
        }

        removeCall(call, new DisconnectCause(DisconnectCause.LOCAL));
    }

    /** @see IConnectionService#silence(String) */
    void silence(Call call) {
        final String callId = mCallIdMapper.getCallId(call);
        if (callId != null && isServiceValid("silence")) {
            try {
                logOutgoing("silence %s", callId);
                mServiceInterface.silence(callId);
            } catch (RemoteException e) {
            }
        }
    }

    /** @see IConnectionService#hold(String) */
    void hold(Call call) {
        final String callId = mCallIdMapper.getCallId(call);
        if (callId != null && isServiceValid("hold")) {
            try {
                logOutgoing("hold %s", callId);
                mServiceInterface.hold(callId);
            } catch (RemoteException e) {
            }
        }
    }

    /** @see IConnectionService#unhold(String) */
    void unhold(Call call) {
        final String callId = mCallIdMapper.getCallId(call);
        if (callId != null && isServiceValid("unhold")) {
            try {
                logOutgoing("unhold %s", callId);
                mServiceInterface.unhold(callId);
            } catch (RemoteException e) {
            }
        }
    }

    /** @see IConnectionService#onCallAudioStateChanged(String, CallAudioState) */
    @VisibleForTesting
    public void onCallAudioStateChanged(Call activeCall, CallAudioState audioState) {
        final String callId = mCallIdMapper.getCallId(activeCall);
        if (callId != null && isServiceValid("onCallAudioStateChanged")) {
            try {
                logOutgoing("onCallAudioStateChanged %s %s", callId, audioState);
                mServiceInterface.onCallAudioStateChanged(callId, audioState);
            } catch (RemoteException e) {
            }
        }
    }

    /** @see IConnectionService#disconnect(String) */
    void disconnect(Call call) {
        final String callId = mCallIdMapper.getCallId(call);
        if (callId != null && isServiceValid("disconnect")) {
            try {
                logOutgoing("disconnect %s", callId);
                mServiceInterface.disconnect(callId);
            } catch (RemoteException e) {
            }
        }
    }

    /** @see IConnectionService#answer(String) */
    void answer(Call call, int videoState) {
        final String callId = mCallIdMapper.getCallId(call);
        if (callId != null && isServiceValid("answer")) {
            try {
                logOutgoing("answer %s %d", callId, videoState);
                if (VideoProfile.isAudioOnly(videoState)) {
                    mServiceInterface.answer(callId);
                } else {
                    mServiceInterface.answerVideo(callId, videoState);
                }
            } catch (RemoteException e) {
            }
        }
    }

    /** @see IConnectionService#reject(String) */
    void reject(Call call, boolean rejectWithMessage, String message) {
        final String callId = mCallIdMapper.getCallId(call);
        if (callId != null && isServiceValid("reject")) {
            try {
                logOutgoing("reject %s", callId);

                if (rejectWithMessage && call.can(
                        Connection.CAPABILITY_CAN_SEND_RESPONSE_VIA_CONNECTION)) {
                    mServiceInterface.rejectWithMessage(callId, message);
                } else {
                    mServiceInterface.reject(callId);
                }
            } catch (RemoteException e) {
            }
        }
    }

    /** @see IConnectionService#playDtmfTone(String, char) */
    void playDtmfTone(Call call, char digit) {
        final String callId = mCallIdMapper.getCallId(call);
        if (callId != null && isServiceValid("playDtmfTone")) {
            try {
                logOutgoing("playDtmfTone %s %c", callId, digit);
                mServiceInterface.playDtmfTone(callId, digit);
            } catch (RemoteException e) {
            }
        }
    }

    /** @see IConnectionService#stopDtmfTone(String) */
    void stopDtmfTone(Call call) {
        final String callId = mCallIdMapper.getCallId(call);
        if (callId != null && isServiceValid("stopDtmfTone")) {
            try {
                logOutgoing("stopDtmfTone %s", callId);
                mServiceInterface.stopDtmfTone(callId);
            } catch (RemoteException e) {
            }
        }
    }

    void addCall(Call call) {
        if (mCallIdMapper.getCallId(call) == null) {
            mCallIdMapper.addCall(call);
        }
    }

    /**
     * Associates newCall with this connection service by replacing callToReplace.
     */
    void replaceCall(Call newCall, Call callToReplace) {
        Preconditions.checkState(callToReplace.getConnectionService() == this);
        mCallIdMapper.replaceCall(newCall, callToReplace);
    }

    void removeCall(Call call) {
        removeCall(call, new DisconnectCause(DisconnectCause.ERROR));
    }

    void removeCall(String callId, DisconnectCause disconnectCause) {
        CreateConnectionResponse response = mPendingResponses.remove(callId);
        if (response != null) {
            response.handleCreateConnectionFailure(disconnectCause);
        }

        mCallIdMapper.removeCall(callId);
    }

    void removeCall(Call call, DisconnectCause disconnectCause) {
        CreateConnectionResponse response = mPendingResponses.remove(mCallIdMapper.getCallId(call));
        if (response != null) {
            response.handleCreateConnectionFailure(disconnectCause);
        }

        mCallIdMapper.removeCall(call);
    }

    void onPostDialContinue(Call call, boolean proceed) {
        final String callId = mCallIdMapper.getCallId(call);
        if (callId != null && isServiceValid("onPostDialContinue")) {
            try {
                logOutgoing("onPostDialContinue %s %b", callId, proceed);
                mServiceInterface.onPostDialContinue(callId, proceed);
            } catch (RemoteException ignored) {
            }
        }
    }

    void conference(final Call call, Call otherCall) {
        final String callId = mCallIdMapper.getCallId(call);
        final String otherCallId = mCallIdMapper.getCallId(otherCall);
        if (callId != null && otherCallId != null && isServiceValid("conference")) {
            try {
                logOutgoing("conference %s %s", callId, otherCallId);
                mServiceInterface.conference(callId, otherCallId);
            } catch (RemoteException ignored) {
            }
        }
    }

    void splitFromConference(Call call) {
        final String callId = mCallIdMapper.getCallId(call);
        if (callId != null && isServiceValid("splitFromConference")) {
            try {
                logOutgoing("splitFromConference %s", callId);
                mServiceInterface.splitFromConference(callId);
            } catch (RemoteException ignored) {
            }
        }
    }

    void mergeConference(Call call) {
        final String callId = mCallIdMapper.getCallId(call);
        if (callId != null && isServiceValid("mergeConference")) {
            try {
                logOutgoing("mergeConference %s", callId);
                mServiceInterface.mergeConference(callId);
            } catch (RemoteException ignored) {
            }
        }
    }

    void swapConference(Call call) {
        final String callId = mCallIdMapper.getCallId(call);
        if (callId != null && isServiceValid("swapConference")) {
            try {
                logOutgoing("swapConference %s", callId);
                mServiceInterface.swapConference(callId);
            } catch (RemoteException ignored) {
            }
        }
    }

    void pullExternalCall(Call call) {
        final String callId = mCallIdMapper.getCallId(call);
        if (callId != null && isServiceValid("pullExternalCall")) {
            try {
                logOutgoing("pullExternalCall %s", callId);
                mServiceInterface.pullExternalCall(callId);
            } catch (RemoteException ignored) {
            }
        }
    }

    void sendCallEvent(Call call, String event, Bundle extras) {
        final String callId = mCallIdMapper.getCallId(call);
        if (callId != null && isServiceValid("sendCallEvent")) {
            try {
                logOutgoing("sendCallEvent %s %s", callId, event);
                mServiceInterface.sendCallEvent(callId, event, extras);
            } catch (RemoteException ignored) {
            }
        }
    }

    void onExtrasChanged(Call call, Bundle extras) {
        final String callId = mCallIdMapper.getCallId(call);
        if (callId != null && isServiceValid("onExtrasChanged")) {
            try {
                logOutgoing("onExtrasChanged %s %s", callId, extras);
                mServiceInterface.onExtrasChanged(callId, extras);
            } catch (RemoteException ignored) {
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    protected void setServiceInterface(IBinder binder) {
        if (binder == null) {
            // We have lost our service connection. Notify the world that this service is done.
            // We must notify the adapter before CallsManager. The adapter will force any pending
            // outgoing calls to try the next service. This needs to happen before CallsManager
            // tries to clean up any calls still associated with this service.
            handleConnectionServiceDeath();
            mCallsManager.handleConnectionServiceDeath(this);
            mServiceInterface = null;
        } else {
            mServiceInterface = IConnectionService.Stub.asInterface(binder);
            addConnectionServiceAdapter(mAdapter);
        }
    }

    private void handleCreateConnectionComplete(
            String callId,
            ConnectionRequest request,
            ParcelableConnection connection) {
        // TODO: Note we are not using parameter "request", which is a side effect of our tacit
        // assumption that we have at most one outgoing connection attempt per ConnectionService.
        // This may not continue to be the case.
        if (connection.getState() == Connection.STATE_DISCONNECTED) {
            // A connection that begins in the DISCONNECTED state is an indication of
            // failure to connect; we handle all failures uniformly
            removeCall(callId, connection.getDisconnectCause());
        } else {
            // Successful connection
            if (mPendingResponses.containsKey(callId)) {
                mPendingResponses.remove(callId)
                        .handleCreateConnectionSuccess(mCallIdMapper, connection);
            }
        }
    }

    /**
     * Called when the associated connection service dies.
     */
    private void handleConnectionServiceDeath() {
        if (!mPendingResponses.isEmpty()) {
            CreateConnectionResponse[] responses = mPendingResponses.values().toArray(
                    new CreateConnectionResponse[mPendingResponses.values().size()]);
            mPendingResponses.clear();
            for (int i = 0; i < responses.length; i++) {
                responses[i].handleCreateConnectionFailure(
                        new DisconnectCause(DisconnectCause.ERROR));
            }
        }
        mCallIdMapper.clear();
    }

    private void logIncoming(String msg, Object... params) {
        Log.d(this, "ConnectionService -> Telecom: " + msg, params);
    }

    private void logOutgoing(String msg, Object... params) {
        Log.d(this, "Telecom -> ConnectionService: " + msg, params);
    }

    private void queryRemoteConnectionServices(final UserHandle userHandle,
            final RemoteServiceCallback callback) {
        // Only give remote connection services to this connection service if it is listed as
        // the connection manager.
        PhoneAccountHandle simCallManager = mPhoneAccountRegistrar.getSimCallManager(userHandle);
        Log.d(this, "queryRemoteConnectionServices finds simCallManager = %s", simCallManager);
        if (simCallManager == null ||
                !simCallManager.getComponentName().equals(getComponentName())) {
            noRemoteServices(callback);
            return;
        }

        // Make a list of ConnectionServices that are listed as being associated with SIM accounts
        final Set<ConnectionServiceWrapper> simServices = Collections.newSetFromMap(
                new ConcurrentHashMap<ConnectionServiceWrapper, Boolean>(8, 0.9f, 1));
        for (PhoneAccountHandle handle : mPhoneAccountRegistrar.getSimPhoneAccounts(userHandle)) {
            ConnectionServiceWrapper service = mConnectionServiceRepository.getService(
                    handle.getComponentName(), handle.getUserHandle());
            if (service != null) {
                simServices.add(service);
            }
        }

        final List<ComponentName> simServiceComponentNames = new ArrayList<>();
        final List<IBinder> simServiceBinders = new ArrayList<>();

        Log.v(this, "queryRemoteConnectionServices, simServices = %s", simServices);

        for (ConnectionServiceWrapper simService : simServices) {
            if (simService == this) {
                // Only happens in the unlikely case that a SIM service is also a SIM call manager
                continue;
            }

            final ConnectionServiceWrapper currentSimService = simService;

            currentSimService.mBinder.bind(new BindCallback() {
                @Override
                public void onSuccess() {
                    Log.d(this, "Adding simService %s", currentSimService.getComponentName());
                    simServiceComponentNames.add(currentSimService.getComponentName());
                    simServiceBinders.add(currentSimService.mServiceInterface.asBinder());
                    maybeComplete();
                }

                @Override
                public void onFailure() {
                    Log.d(this, "Failed simService %s", currentSimService.getComponentName());
                    // We know maybeComplete() will always be a no-op from now on, so go ahead and
                    // signal failure of the entire request
                    noRemoteServices(callback);
                }

                private void maybeComplete() {
                    if (simServiceComponentNames.size() == simServices.size()) {
                        setRemoteServices(callback, simServiceComponentNames, simServiceBinders);
                    }
                }
            }, null);
        }
    }

    private void setRemoteServices(
            RemoteServiceCallback callback,
            List<ComponentName> componentNames,
            List<IBinder> binders) {
        try {
            callback.onResult(componentNames, binders);
        } catch (RemoteException e) {
            Log.e(this, e, "Contacting ConnectionService %s",
                    ConnectionServiceWrapper.this.getComponentName());
        }
    }

    private void noRemoteServices(RemoteServiceCallback callback) {
        setRemoteServices(callback, Collections.EMPTY_LIST, Collections.EMPTY_LIST);
    }
}
