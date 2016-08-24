package com.android.bluetooth.tests;

import java.util.ArrayList;
import java.util.concurrent.CountDownLatch;

import javax.obex.HeaderSet;
import javax.obex.Operation;
import javax.obex.ResponseCodes;

import junit.framework.Assert;
import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import android.os.Handler;
import android.os.RemoteException;
import android.util.Log;

import com.android.bluetooth.map.BluetoothMapAccountItem;
import com.android.bluetooth.map.BluetoothMapContentObserver;
import com.android.bluetooth.map.BluetoothMapMasInstance;
import com.android.bluetooth.map.BluetoothMapObexServer;
import com.android.bluetooth.map.BluetoothMapUtils;
import com.android.bluetooth.map.BluetoothMnsObexClient;

public class MapObexTestServer extends BluetoothMapObexServer {

    private static final String TAG = "MapObexTestServer";
    private static final boolean V = true;

    ArrayList<SeqStep> mSequence;
    CountDownLatch mStopLatch;

    ObexTestDataHandler mDataHandler;
    int mOperationIndex = 0;

    /* This needs to be static, as calling the super-constructor must be the first step.
     * Alternatively add the account as constructor parameter, and create a builder
     * function - factory pattern. */
//    private static BluetoothMapAccountItem mAccountMock = new BluetoothMapAccountItem("1",
//           "TestAccount",
//           "do.not.exist.package.name.and.never.used.anyway:-)",
//           "info.guardianproject.otr.app.im.provider.bluetoothprovider",
//           null,
//           BluetoothMapUtils.TYPE.IM,
//           null,
//           null);
    private static BluetoothMapAccountItem mAccountMock = null;

    /* MAP Specific instance variables
    private final BluetoothMapContentObserver mObserver = null;
    private final BluetoothMnsObexClient mMnsClient = null;*/

    /* Test values, consider gathering somewhere else */
    private static final int MAS_ID = 0;
    private static final int REMOTE_FEATURE_MASK = 0x07FFFFFF;
    private static final BluetoothMapMasInstance mMasInstance =
            new MockMasInstance(MAS_ID, REMOTE_FEATURE_MASK);

    public MapObexTestServer(final Context context, ArrayList<SeqStep> sequence,
            CountDownLatch stopLatch) throws RemoteException {

        super(null, context,
                new  BluetoothMapContentObserver(context,
                        new BluetoothMnsObexClient(
                                BluetoothAdapter.getDefaultAdapter().
                                getRemoteDevice("12:23:34:45:56:67"), null, null),
                                /* TODO: this will not work for single device test... */
                        mMasInstance,
                        mAccountMock, /* Account */
                        true) /* Enable SMS/MMS*/,
                mMasInstance,
                mAccountMock /* Account */,
                true /* SMS/MMS enabled*/);
        mSequence = sequence;
        mDataHandler = new ObexTestDataHandler("(Server)");
        mStopLatch = stopLatch;
    }

    /* OBEX operation handlers */
    @Override
    public int onConnect(HeaderSet request, HeaderSet reply) {
        Log.i(TAG,"onConnect()");
        int index;
        int result = ResponseCodes.OBEX_HTTP_OK;
        try {
            index = ((Long)request.getHeader(TestSequencer.STEP_INDEX_HEADER)).intValue();
            mOperationIndex = index;
            SeqStep step = mSequence.get(mOperationIndex);
            Assert.assertNotNull("invalid step index!", step);
            if(step.mServerPreAction != null) {
                step.mServerPreAction.execute(step, request, null);
            }
            result = super.onConnect(request, reply);
        } catch (Exception e) {
            Log.e(TAG, "Exception in onConnect - aborting...", e);
            result = ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
            // A read from null will produce exception to end the test.
        }
        return result;
    }

    @Override
    public void onDisconnect(HeaderSet request, HeaderSet reply) {
        Log.i(TAG,"onDisconnect()");
        /* TODO: validate request headers, and set response headers */
        int index;
        int result = ResponseCodes.OBEX_HTTP_OK;
        try {
            index = ((Long)request.getHeader(TestSequencer.STEP_INDEX_HEADER)).intValue();
            mOperationIndex = index;
            SeqStep step = mSequence.get(mOperationIndex);
            Assert.assertNotNull("invalid step index!", step);
            if(step.mServerPreAction != null) {
                step.mServerPreAction.execute(step, request, null);
            }
            super.onDisconnect(request, reply);
        } catch (Exception e) {
            Log.e(TAG, "Exception in onDisconnect - aborting...", e);
            result = ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
            // A read from null will produce exception to end the test.
        }
        if(mOperationIndex >= (mSequence.size()-1)) {
            /* End of test, signal test runner thread */
            Log.i(TAG, "Sending latch close signal...");
            mStopLatch.countDown();
        } else {
            Log.i(TAG, "Got disconnect with mOperationCounter = " + mOperationIndex);
        }
        reply.responseCode = result;
    }

    @Override
    public int onPut(Operation operation) {
        Log.i(TAG,"onPut()");
        int result = ResponseCodes.OBEX_HTTP_OK;
        try{
            HeaderSet reqHeaders = operation.getReceivedHeader();
            int index = ((Long)reqHeaders.getHeader(TestSequencer.STEP_INDEX_HEADER)).intValue();
            mOperationIndex = index;
            SeqStep step = mSequence.get(mOperationIndex);
            Assert.assertNotNull("invalid step index!", step);
            if(step.mServerPreAction != null) {
                step.mServerPreAction.execute(step, reqHeaders, operation);
            }
            super.onPut(operation);
        } catch (Exception e) {
            Log.e(TAG, "Exception in onPut - aborting...", e);
            result = ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
            // A read from null will produce exception to end the test.
        }
        if(result == ResponseCodes.OBEX_HTTP_OK) {
            Log.i(TAG, "OBEX-HANDLER: operation complete success");
        } else {
            Log.e(TAG, "OBEX-HANDLER: operation complete FAILED!");
        }
        return result;
    }

    @Override
    public int onGet(Operation operation) {
        Log.i(TAG,"onGet()");
        int result = ResponseCodes.OBEX_HTTP_OK;
        try{
            HeaderSet reqHeaders = operation.getReceivedHeader();
            int index = ((Long)reqHeaders.getHeader(TestSequencer.STEP_INDEX_HEADER)).intValue();
            mOperationIndex = index;
            SeqStep step = mSequence.get(mOperationIndex);
            Assert.assertNotNull("invalid step index!", step);
            if(step.mServerPreAction != null) {
                step.mServerPreAction.execute(step, reqHeaders, operation);
            }
            super.onGet(operation);
        } catch (Exception e) {
            Log.e(TAG, "Exception in onGet - aborting...", e);
            result = ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
            // A read from null will produce exception to end the test.
        }
        if(result == ResponseCodes.OBEX_HTTP_OK) {
            Log.i(TAG, "OBEX-HANDLER: operation complete success");
        } else {
            Log.e(TAG, "OBEX-HANDLER: operation complete FAILED!");
        }
        return result;
    }



}

