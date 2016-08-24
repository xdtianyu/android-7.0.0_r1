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

import android.car.CarAppContextManager;
import android.car.IAppContext;
import android.car.IAppContextListener;
import android.content.Context;
import android.os.Binder;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.util.Log;

import com.android.car.hal.VehicleHal;

import java.io.PrintWriter;
import java.util.HashMap;

public class AppContextService extends IAppContext.Stub implements CarServiceBase,
        BinderInterfaceContainer.BinderEventHandler<IAppContextListener> {
    private static final boolean DBG = true;
    private static final boolean DBG_EVENT = false;

    private final ClientHolder mAllClients;
    /** K: context flag, V: client owning it */
    private final HashMap<Integer, ClientInfo> mContextOwners = new HashMap<>();
    private int mActiveContexts;

    private final HandlerThread mHandlerThread;
    private final DispatchHandler mDispatchHandler;

    public AppContextService(Context context) {
        mAllClients = new ClientHolder(this);
        mHandlerThread = new HandlerThread(AppContextService.class.getSimpleName());
        mHandlerThread.start();
        mDispatchHandler = new DispatchHandler(mHandlerThread.getLooper());
    }

    @Override
    public void registerContextListener(IAppContextListener listener, int filter) {
        synchronized (this) {
            ClientInfo info = (ClientInfo) mAllClients.getBinderInterface(listener);
            if (info == null) {
                info = new ClientInfo(mAllClients, listener, Binder.getCallingUid(),
                        Binder.getCallingPid(), filter);
                mAllClients.addBinderInterface(info);
            } else {
                info.setFilter(filter);
            }
        }
    }

    @Override
    public void unregisterContextListener(IAppContextListener listener) {
        synchronized (this) {
            ClientInfo info = (ClientInfo) mAllClients.getBinderInterface(listener);
            if (info == null) {
                return;
            }
            resetActiveContexts(listener, info.getOwnedContexts());
            mAllClients.removeBinder(listener);
        }
    }

    @Override
    public int getActiveAppContexts() {
        synchronized (this) {
            return mActiveContexts;
        }
    }

    @Override
    public boolean isOwningContext(IAppContextListener listener, int context) {
        synchronized (this) {
            ClientInfo info = (ClientInfo) mAllClients.getBinderInterface(listener);
            if (info == null) {
                return false;
            }
            int ownedContexts = info.getOwnedContexts();
            return (ownedContexts & context) == context;
        }
    }

    @Override
    public void setActiveContexts(IAppContextListener listener, int contexts) {
        synchronized (this) {
            ClientInfo info = (ClientInfo) mAllClients.getBinderInterface(listener);
            if (info == null) {
                throw new IllegalStateException("listener not registered");
            }
            int alreadyOwnedContexts = info.getOwnedContexts();
            int addedContexts = 0;
            for (int c = CarAppContextManager.APP_CONTEXT_START_FLAG;
                    c <= CarAppContextManager.APP_CONTEXT_END_FLAG; c = (c << 1)) {
                if ((c & contexts) != 0 && (c & alreadyOwnedContexts) == 0) {
                    ClientInfo ownerInfo = mContextOwners.get(c);
                    if (ownerInfo != null && ownerInfo != info) {
                        //TODO check if current owner is having fore-ground activity. If yes,
                        //reject request. Always grant if requestor is fore-ground activity.
                        ownerInfo.setOwnedContexts(ownerInfo.getOwnedContexts() & ~c);
                        mDispatchHandler.requestAppContextOwnershipLossDispatch(
                                ownerInfo.binderInterface, c);
                        if (DBG) {
                            Log.i(CarLog.TAG_APP_CONTEXT, "losing context " +
                                    Integer.toHexString(c) + "," + ownerInfo.toString());
                        }
                    } else {
                        addedContexts |= c;
                    }
                    mContextOwners.put(c, info);
                }
            }
            info.setOwnedContexts(alreadyOwnedContexts | contexts);
            mActiveContexts |= addedContexts;
            if (addedContexts != 0) {
                if (DBG) {
                    Log.i(CarLog.TAG_APP_CONTEXT, "setting context " +
                            Integer.toHexString(addedContexts) + "," + info.toString());
                }
                for (BinderInterfaceContainer.BinderInterface<IAppContextListener> client :
                    mAllClients.getInterfaces()) {
                    ClientInfo clientInfo = (ClientInfo) client;
                    // dispatch events only when there is change after filter and the listener
                    // is not coming from the current caller.
                    int clientFilter = clientInfo.getFilter();
                    if ((addedContexts & clientFilter) != 0 && clientInfo != info) {
                        mDispatchHandler.requestAppContextChangeDispatch(clientInfo.binderInterface,
                                mActiveContexts & clientFilter);
                    }
                }
            }
        }
    }

    @Override
    public void resetActiveContexts(IAppContextListener listener, int contexts) {
        synchronized (this) {
            ClientInfo info = (ClientInfo) mAllClients.getBinderInterface(listener);
            if (info == null) {
                // ignore as this client cannot have owned anything.
                return;
            }
            if ((contexts & mActiveContexts) == 0) {
                // ignore as none of them are active;
                return;
            }
            int removedContexts = 0;
            int currentlyOwnedContexts = info.getOwnedContexts();
            for (int c = CarAppContextManager.APP_CONTEXT_START_FLAG;
                    c <= CarAppContextManager.APP_CONTEXT_END_FLAG; c = (c << 1)) {
                if ((c & contexts) != 0 && (c & currentlyOwnedContexts) != 0) {
                    removedContexts |= c;
                    mContextOwners.remove(c);
                }
            }
            if (removedContexts != 0) {
                mActiveContexts &= ~removedContexts;
                info.setOwnedContexts(currentlyOwnedContexts & ~removedContexts);
                if (DBG) {
                    Log.i(CarLog.TAG_APP_CONTEXT, "resetting context " +
                            Integer.toHexString(removedContexts) + "," + info.toString());
                }
                for (BinderInterfaceContainer.BinderInterface<IAppContextListener> client :
                    mAllClients.getInterfaces()) {
                    ClientInfo clientInfo = (ClientInfo) client;
                    int clientFilter = clientInfo.getFilter();
                    if ((removedContexts & clientFilter) != 0 && clientInfo != info) {
                        mDispatchHandler.requestAppContextChangeDispatch(clientInfo.binderInterface,
                                mActiveContexts & clientFilter);
                    }
                }
            }
        }
    }

    @Override
    public void init() {
        // nothing to do
    }

    @Override
    public void release() {
        synchronized (this) {
            mAllClients.clear();
            mContextOwners.clear();
            mActiveContexts = 0;
        }
    }

    @Override
    public void onBinderDeath(
            BinderInterfaceContainer.BinderInterface<IAppContextListener> bInterface) {
        ClientInfo info = (ClientInfo) bInterface;
        int ownedContexts = info.getOwnedContexts();
        if (ownedContexts != 0) {
            resetActiveContexts(bInterface.binderInterface, ownedContexts);
        }
    }

    @Override
    public void dump(PrintWriter writer) {
        writer.println("**AppContextService**");
        synchronized (this) {
            writer.println("mActiveContexts:" + Integer.toHexString(mActiveContexts));
            for (BinderInterfaceContainer.BinderInterface<IAppContextListener> client :
                mAllClients.getInterfaces()) {
                ClientInfo clientInfo = (ClientInfo) client;
                writer.println(clientInfo.toString());
            }
        }
    }

    /**
     * Returns true if process with given uid and pid owns provided context.
     */
    public boolean isContextOwner(int uid, int pid, int context) {
        synchronized (this) {
            if (mContextOwners.containsKey(context)) {
                ClientInfo clientInfo = mContextOwners.get(context);
                return clientInfo.uid == uid && clientInfo.pid == pid;
            }
        }
        return false;
    }

    private void dispatchAppContextOwnershipLoss(IAppContextListener listener, int contexts) {
        try {
            listener.onAppContextOwnershipLoss(contexts);
        } catch (RemoteException e) {
        }
    }

    private void dispatchAppContextChange(IAppContextListener listener, int contexts) {
        try {
            listener.onAppContextChange(contexts);
        } catch (RemoteException e) {
        }
    }

    private static class ClientHolder extends BinderInterfaceContainer<IAppContextListener> {
        private ClientHolder(AppContextService service) {
            super(service);
        }
    }

    private static class ClientInfo extends
            BinderInterfaceContainer.BinderInterface<IAppContextListener> {
        private final int uid;
        private final int pid;
        private int mFilter;
        /** contexts owned by this client */
        private int mOwnedContexts;

        private ClientInfo(ClientHolder holder, IAppContextListener binder, int uid, int pid,
                int filter) {
            super(holder, binder);
            this.uid = uid;
            this.pid = pid;
            this.mFilter = filter;
        }

        private synchronized int getFilter() {
            return mFilter;
        }

        private synchronized void setFilter(int filter) {
            mFilter = filter;
        }

        private synchronized int getOwnedContexts() {
            if (DBG_EVENT) {
                Log.i(CarLog.TAG_APP_CONTEXT, "getOwnedContexts " +
                        Integer.toHexString(mOwnedContexts));
            }
            return mOwnedContexts;
        }

        private synchronized void setOwnedContexts(int contexts) {
            if (DBG_EVENT) {
                Log.i(CarLog.TAG_APP_CONTEXT, "setOwnedContexts " + Integer.toHexString(contexts));
            }
            mOwnedContexts = contexts;
        }

        @Override
        public String toString() {
            synchronized (this) {
                return "ClientInfo{uid=" + uid + ",pid=" + pid +
                        ",filter=" + Integer.toHexString(mFilter) +
                        ",owned=" + Integer.toHexString(mOwnedContexts) + "}";
            }
        }
    }

    private class DispatchHandler extends Handler {
        private static final int MSG_DISPATCH_OWNERSHIP_LOSS = 0;
        private static final int MSG_DISPATCH_CONTEXT_CHANGE = 1;

        private DispatchHandler(Looper looper) {
            super(looper);
        }

        private void requestAppContextOwnershipLossDispatch(IAppContextListener listener,
                int contexts) {
            Message msg = obtainMessage(MSG_DISPATCH_OWNERSHIP_LOSS, contexts, 0, listener);
            sendMessage(msg);
        }

        private void requestAppContextChangeDispatch(IAppContextListener listener, int contexts) {
            Message msg = obtainMessage(MSG_DISPATCH_CONTEXT_CHANGE, contexts, 0, listener);
            sendMessage(msg);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_DISPATCH_OWNERSHIP_LOSS:
                    dispatchAppContextOwnershipLoss((IAppContextListener) msg.obj, msg.arg1);
                    break;
                case MSG_DISPATCH_CONTEXT_CHANGE:
                    dispatchAppContextChange((IAppContextListener) msg.obj, msg.arg1);
                    break;
            }
        }
    }
}
