package com.android.bluetooth.tests;

import java.io.IOException;
import java.util.Arrays;
import java.util.concurrent.CountDownLatch;

import javax.obex.HeaderSet;
import javax.obex.Operation;
import javax.obex.ResponseCodes;
import javax.obex.ServerRequestHandler;

import junit.framework.Assert;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothUuid;
import android.bluetooth.SdpMasRecord;
import android.bluetooth.SdpMnsRecord;
import android.bluetooth.SdpOppOpsRecord;
import android.bluetooth.SdpPseRecord;
import android.bluetooth.SdpSapsRecord;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.ParcelUuid;
import android.util.Log;

import com.android.bluetooth.btservice.AbstractionLayer;
import com.android.bluetooth.sdp.SdpManager;

/**
 * We use an OBEX server to execute SDP search operations, and validate results.
 * @author cbonde
 *
 */
public class SdpManagerTestServer extends ServerRequestHandler {

    private static final String TAG = "SdpManagerTestServer";
    private static final boolean V = true;

    int mOperationIndex = 0;
    int mResult = ResponseCodes.OBEX_HTTP_OK;

    final Context mContext;
    final CountDownLatch mStopLatch;
    final BluetoothDevice mDevice;

    public SdpManagerTestServer(CountDownLatch stopLatch, Context context,
            BluetoothDevice device) {
        super();
        mStopLatch = stopLatch;
        mContext = context;
        mDevice = device;
        Log.i(TAG, "created.");
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
        } catch (IOException e) {
            Log.e(TAG, "Exception in onConnect - aborting...");
            result = ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
            // A read from null will produce exception to end the test.
        }
        return result;
    }

    @Override
    public void onDisconnect(HeaderSet request, HeaderSet reply) {
        Log.i(TAG,"onDisconnect()");
        int index;
        int result = ResponseCodes.OBEX_HTTP_OK;
        try {
            index = ((Long)request.getHeader(TestSequencer.STEP_INDEX_HEADER)).intValue();
            mOperationIndex = index;
        } catch (IOException e) {
            Log.e(TAG, "Exception in onDisconnect...");
            result = ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
            // A read from null will produce exception to end the test.
        }
        if(mOperationIndex == 0) {
            /* End of test, signal test runner thread */
            Log.i(TAG, "Sending latch close signal...");
            mStopLatch.countDown();
        } else {
            Log.i(TAG, "Got disconnect with mOperationCounter = " + mOperationIndex);
        }
        reply.responseCode = result;
    }

    /**
     * Currently not used
     */
    @Override
    public int onPut(Operation operation) {
        Log.i(TAG,"onPut()");
        int result = ResponseCodes.OBEX_HTTP_NOT_IMPLEMENTED;
        return result;
    }

    /**
     * Used to execute SDP search operations.
     */
    @Override
    public int onGet(Operation operation) {
        Log.i(TAG,"onGet()");
        /* - Use the name header to transfer a ';' separated list of UUID's to search for.
         * - For each UUID:
         *    - start a search
         *    - validate each result received
         *    - ensure all records gets received (use CountDownLatch)
         * */
        mResult = ResponseCodes.OBEX_HTTP_OK;
        try{
            HeaderSet reqHeaders = operation.getReceivedHeader();
            int index = ((Long)reqHeaders.getHeader(TestSequencer.STEP_INDEX_HEADER)).intValue();
            mOperationIndex = index;
            /* Get the expected number of records to read. */
            int count = ((Long)reqHeaders.getHeader(HeaderSet.COUNT)).intValue();
            String name = (String)reqHeaders.getHeader(HeaderSet.NAME);
            String[] uuids = name.split(";");

            // Initiate search operations, Wait for results and validate
            searchAwaitAndValidate(uuids, mDevice, count);
        } catch (IOException e) {
            Log.e(TAG, "Exception in onPut - aborting...");
            mResult = ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
        } finally {
        }
        if(mResult == ResponseCodes.OBEX_HTTP_OK) {
            Log.i(TAG, "OBEX-HANDLER: operation complete success");
        } else {
            Log.e(TAG, "OBEX-HANDLER: operation complete FAILED!");
        }
        return mResult;
    }


    class SdpBroadcastReceiver extends BroadcastReceiver {

        boolean hasMas = false;
        boolean hasMns = false;
        boolean hasOppServer = false;
        boolean hasSapServer = false;
        boolean hasPse = false;
        final CountDownLatch mLatch;

        public SdpBroadcastReceiver(String[] uuids, CountDownLatch latch) {
            for(String uuid : uuids) {
                if(uuid.toString().equals(BluetoothUuid.MAS.toString()))
                    hasMas = true;
                if(uuid.toString().equals(BluetoothUuid.MNS.toString()))
                    hasMns = true;
                if(uuid.toString().equals(BluetoothUuid.PBAP_PSE.toString()))
                    hasPse = true;
                if(uuid.toString().equals(BluetoothUuid.ObexObjectPush.toString()))
                    hasOppServer = true;
                if(uuid.toString().equals(BluetoothUuid.SAP.toString()))
                    hasSapServer = true;
            }
            mLatch = latch;
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "onReceive");
            String action = intent.getAction();
            if (action.equals(BluetoothDevice.ACTION_SDP_RECORD)){
                Log.v(TAG, "Received ACTION_SDP_RECORD.");
                ParcelUuid uuid = intent.getParcelableExtra(BluetoothDevice.EXTRA_UUID);
                Log.v(TAG, "Received UUID: " + uuid.toString());
                if(hasMas && uuid.toString().equals(BluetoothUuid.MAS.toString())) {
                    Log.v(TAG, " -> MAS UUID in result.");
                    int status = intent.getIntExtra(BluetoothDevice.EXTRA_SDP_SEARCH_STATUS, -1);
                    Assert.assertEquals(AbstractionLayer.BT_STATUS_SUCCESS, status); /* BT_STATUS_SUCCESS == 0 - but status is not documented... */
                    Log.v(TAG, " -> status: "+status);
                    SdpMasRecord record = intent.getParcelableExtra(BluetoothDevice.EXTRA_SDP_RECORD);
                    Assert.assertNotNull(record);
                    Log.v(TAG, " -> Record: " + record);
                    /* As the normal profiles are also running, we filter out these records */
                    if(record.getServiceName().equals(SdpManagerTest.SDP_SERVER_NAME)) {
                        Assert.assertEquals(((long)record.getSupportedFeatures())&0xffffffffL, SdpManagerTest.SDP_FEATURES);
                        Assert.assertEquals(record.getSupportedMessageTypes(), SdpManagerTest.SDP_MSG_TYPES);
                        Assert.assertEquals(record.getProfileVersion(), SdpManagerTest.SDP_VERSION);
                        Assert.assertEquals(record.getServiceName(), SdpManagerTest.SDP_SERVER_NAME);
                        Assert.assertEquals(record.getMasInstanceId(), SdpManagerTest.SDP_MAS_ID);
                        int rfcommChannel = record.getRfcommCannelNumber();
                        int l2capPsm = record.getL2capPsm();
                        /* We set RFCOMM-channel to record_id and the l2cap PSM to iteration*record_id */
                        Assert.assertEquals(mOperationIndex+rfcommChannel,l2capPsm);
                        mLatch.countDown();
                    } else {
                        Log.i(TAG, "Wrong service name (" + record.getServiceName()
                                + ") received, still waiting...");
                    }
                } else if(hasMns && uuid.toString().equals(BluetoothUuid.MNS.toString())) {
                    Log.v(TAG, " -> MAP_MNS UUID in result.");
                    int status = intent.getIntExtra(BluetoothDevice.EXTRA_SDP_SEARCH_STATUS, -1);
                    Assert.assertEquals(0, status); /* BT_STATUS_SUCCESS == 0 - but status is not documented... */
                    Log.v(TAG, " -> status: "+status);
                    SdpMnsRecord record = intent.getParcelableExtra(BluetoothDevice.EXTRA_SDP_RECORD);
                    Assert.assertNotNull(record);
                    Log.v(TAG, " -> Record: " + record);
                    /* As the normal profiles are also running, we filter out these records */
                    if(record.getServiceName().equals(SdpManagerTest.SDP_SERVER_NAME)) {
                        Assert.assertEquals(((long)record.getSupportedFeatures())&0xffffffffL, SdpManagerTest.SDP_FEATURES);
                        Assert.assertEquals(record.getProfileVersion(), SdpManagerTest.SDP_VERSION);
                        Assert.assertEquals(record.getServiceName(), SdpManagerTest.SDP_SERVER_NAME);
                        int rfcommChannel = record.getRfcommChannelNumber();
                        int l2capPsm = record.getL2capPsm();
                        /* We set RFCOMM-channel to record_id and the l2cap PSM to iteration*record_id */
                        Assert.assertEquals(mOperationIndex+rfcommChannel,l2capPsm);
                        mLatch.countDown();
                    } else {
                        Log.i(TAG, "Wrong service name (" + record.getServiceName()
                                + ") received, still waiting...");
                    }
                } else if(hasPse && uuid.toString().equals(BluetoothUuid.PBAP_PSE.toString())) {
                    Log.v(TAG, " -> PBAP_PSE UUID in result.");
                    int status = intent.getIntExtra(BluetoothDevice.EXTRA_SDP_SEARCH_STATUS, -1);
                    Assert.assertEquals(0, status); /* BT_STATUS_SUCCESS == 0 - but status is not documented... */
                    Log.v(TAG, " -> status: "+status);
                    SdpPseRecord record = intent.getParcelableExtra(BluetoothDevice.EXTRA_SDP_RECORD);
                    Assert.assertNotNull(record);
                    Log.v(TAG, " -> Record: " + record);
                    /* As the normal profiles are also running, we filter out these records */
                    if(record.getServiceName().equals(SdpManagerTest.SDP_SERVER_NAME)) {
                        Assert.assertEquals(((long)record.getSupportedFeatures())&0xffffffffL, SdpManagerTest.SDP_FEATURES);
                        Assert.assertEquals(((long)record.getSupportedRepositories())&0xffffffffL, SdpManagerTest.SDP_REPOS);
                        Assert.assertEquals(record.getProfileVersion(), SdpManagerTest.SDP_VERSION);
                        Assert.assertEquals(record.getServiceName(), SdpManagerTest.SDP_SERVER_NAME);
                        int rfcommChannel = record.getRfcommChannelNumber();
                        int l2capPsm = record.getL2capPsm();
                        /* We set RFCOMM-channel to record_id and the l2cap PSM to iteration*record_id */
                        Assert.assertEquals(mOperationIndex+rfcommChannel,l2capPsm);
                        mLatch.countDown();
                    } else {
                        Log.i(TAG, "Wrong service name (" + record.getServiceName()
                                + ") received, still waiting...");
                    }
                } else if(hasOppServer && uuid.toString().equals(BluetoothUuid.ObexObjectPush.toString())) {
                    Log.v(TAG, " -> OPP Server UUID in result.");
                    int status = intent.getIntExtra(BluetoothDevice.EXTRA_SDP_SEARCH_STATUS, -1);
                    Assert.assertEquals(0, status); /* BT_STATUS_SUCCESS == 0 - but status is not documented... */
                    Log.v(TAG, " -> status: "+status);
                    SdpOppOpsRecord record = intent.getParcelableExtra(BluetoothDevice.EXTRA_SDP_RECORD);
                    Assert.assertNotNull(record);
                    Log.v(TAG, " -> Record: " + record);
                    /* As the normal profiles are also running, we filter out these records */
                    if(record.getServiceName().equals(SdpManagerTest.SDP_SERVER_NAME)) {
                        Assert.assertEquals(record.getProfileVersion(), SdpManagerTest.SDP_VERSION);
                        Assert.assertEquals(record.getServiceName(), SdpManagerTest.SDP_SERVER_NAME);
                        Assert.assertTrue(Arrays.equals(record.getFormatsList(),SdpManager.OPP_FORMAT_ALL));
                        int rfcommChannel = record.getRfcommChannel();
                        int l2capPsm = record.getL2capPsm();
                        /* We set RFCOMM-channel to record_id and the l2cap PSM to iteration*record_id */
                        Assert.assertEquals(mOperationIndex+rfcommChannel,l2capPsm);
                        mLatch.countDown();
                    } else {
                        Log.i(TAG, "Wrong service name (" + record.getServiceName()
                                + ") received, still waiting...");
                    }
                } else if (hasSapServer && uuid.toString().equals(BluetoothUuid.SAP.toString())) {
                    Log.v(TAG, " -> SAP Server UUID in result.");
                    int status = intent.getIntExtra(BluetoothDevice.EXTRA_SDP_SEARCH_STATUS, -1);
                    Assert.assertEquals(AbstractionLayer.BT_STATUS_SUCCESS, status); /* BT_STATUS_SUCCESS == 0 - but status is not documented... */
                    Log.v(TAG, " -> status: "+status);
                    SdpSapsRecord record = intent.getParcelableExtra(BluetoothDevice.EXTRA_SDP_RECORD);
                    Assert.assertNotNull(record);
                    Log.v(TAG, " -> Record: " + record);
                    /* As the normal profiles are also running, we filter out these records */
                    if (record.getServiceName().equals(SdpManagerTest.SDP_SERVER_NAME)) {
                        Assert.assertEquals(record.getProfileVersion(), SdpManagerTest.SDP_VERSION);
                        Assert.assertEquals(record.getServiceName(), SdpManagerTest.SDP_SERVER_NAME);
                        int rfcommChannel = record.getRfcommCannelNumber();
                        /* We set RFCOMM-channel to record_id and the l2cap PSM to
                         * iteration*record_id.
                         * As SAP does not carry a L2CAP PSM, we cannot validate the RFCOMM value
                        Assert.assertEquals(mOperationIndex+rfcommChannel, l2capPsm); */
                        mLatch.countDown();
                    } else {
                        Log.i(TAG, "Wrong service name (" + record.getServiceName()
                                + ") received, still waiting...");
                    }
                } else {
                    Log.i(TAG, "Wrong UUID received, still waiting...");
                }
            } else {
                Assert.fail("Unexpected intent received???");
            }
        }
    };

    private void searchAwaitAndValidate(final String[] uuids, BluetoothDevice serverDevice, int count) {
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_SDP_RECORD);
        final CountDownLatch latch = new CountDownLatch(count);
        SdpBroadcastReceiver broadcastReceiver = new SdpBroadcastReceiver(uuids, latch);

        // Register receiver
        mContext.registerReceiver(broadcastReceiver, filter);

        // Initiate searches
        for(String uuid : uuids) {
            if(uuid.toString().equals(BluetoothUuid.MAS.toString()))
                serverDevice.sdpSearch(BluetoothUuid.MAS);
            if(uuid.toString().equals(BluetoothUuid.MNS.toString()))
                serverDevice.sdpSearch(BluetoothUuid.MNS);
            if(uuid.toString().equals(BluetoothUuid.PBAP_PSE.toString()))
                serverDevice.sdpSearch(BluetoothUuid.PBAP_PSE);
            if(uuid.toString().equals(BluetoothUuid.ObexObjectPush.toString()))
                serverDevice.sdpSearch(BluetoothUuid.ObexObjectPush);
            if(uuid.toString().equals(BluetoothUuid.SAP.toString()))
                serverDevice.sdpSearch(BluetoothUuid.SAP);
        }

        // Await results
        boolean waiting = true;
        while(waiting == true) {
            try {
                Log.i(TAG, "SDP Search requested - awaiting result...");
                latch.await();
                Log.i(TAG, "SDP Search reresult received - continueing.");
                waiting = false;
            } catch (InterruptedException e) {
                Log.w(TAG, "Interrupted witle waiting - keep waiting.", e);
                waiting = true;
            }
        }
        mContext.unregisterReceiver(broadcastReceiver);
    }

}
