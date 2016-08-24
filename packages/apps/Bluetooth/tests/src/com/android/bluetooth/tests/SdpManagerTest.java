package com.android.bluetooth.tests;

import java.io.IOException;
import java.util.concurrent.CountDownLatch;

import javax.obex.ClientSession;
import javax.obex.HeaderSet;
import javax.obex.Operation;
import javax.obex.ResponseCodes;
import javax.obex.ServerSession;

import com.android.bluetooth.BluetoothObexTransport;
import com.android.bluetooth.sdp.SdpManager;

import android.annotation.TargetApi;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import android.bluetooth.BluetoothUuid;
import android.graphics.Paint.Join;
import android.os.Build;
import android.test.AndroidTestCase;
import android.util.Log;

import junit.framework.Assert;

@TargetApi(Build.VERSION_CODES.KITKAT)
public class SdpManagerTest extends AndroidTestCase {

    protected static String TAG = "SdpManagerTest";
    protected static final boolean D = true;

    public static final int SDP_RECORD_COUNT = 12; /* Maximum number of records to create */
    public static final int SDP_ITERATIONS = 2000;

    public static final String SDP_SERVER_NAME = "SDP test server";
    public static final String SDP_CLIENT_NAME = "SDP test client";

    public static final long SDP_FEATURES   = 0x87654321L;  /* 32 bit */
    public static final int  SDP_MSG_TYPES  = 0xf1;         /*  8 bit */
    public static final int  SDP_MAS_ID     = 0xCA;         /*  8 bit */
    public static final int  SDP_VERSION    = 0xF0C0;       /* 16 bit */
    public static final int  SDP_REPOS      = 0xCf;         /*  8 bit */

    SdpManager mManager = null;

    public void testSdpRemove() {
        BluetoothAdapter bt = BluetoothAdapter.getDefaultAdapter();
        if(bt == null) {
            Log.e(TAG,"No Bluetooth Device!");
            assertTrue(false);
        }
        BluetoothTestUtils.enableBt(bt);
        mManager = SdpManager.getDefaultManager();
        addRemoveRecords(SDP_RECORD_COUNT);
    }

    public void testSdpAdd() {
        BluetoothAdapter bt = BluetoothAdapter.getDefaultAdapter();
        if(bt == null) {
            Log.e(TAG,"No Bluetooth Device!");
            assertTrue(false);
        }
        BluetoothTestUtils.enableBt(bt);
        mManager = SdpManager.getDefaultManager();

        int handles[] = new int[SDP_RECORD_COUNT];
        addRecords(handles, 1);

        try {
            Log.i(TAG, "\n\n\nRecords added - waiting 5 minutes...\n\n\n");
            Thread.sleep(300000);
        } catch (InterruptedException e) {
            Log.e(TAG, "Interrupted", e);
        }
        Log.i(TAG, "All done - over and out!;-)");
    }


    private void addRecords(int handles[], int iteration) {
        /* Create the records */
        int record_id = -1; /* first index is 0 */
        int count = handles.length-1; // Break condition
        for(int c = 0; ; c++) {
            Log.i(TAG, "Create c=" + c);
            handles[++record_id] = mManager.createMapMasRecord(SDP_SERVER_NAME,
                    SDP_MAS_ID, record_id, record_id+iteration, SDP_VERSION,
                    SDP_MSG_TYPES, (int)SDP_FEATURES);
            Log.i(TAG, "  Added record_handle=" + handles[record_id]);
            assertTrue(handles[record_id]>=0);
            if(record_id == count) break;

            handles[++record_id] = mManager.createMapMnsRecord(SDP_SERVER_NAME,
                    record_id, record_id+iteration, SDP_VERSION,
                    (int)SDP_FEATURES);
            Log.i(TAG, "  Added record_handle=" + handles[record_id]);
            assertTrue(handles[record_id]>=0);
            if(record_id == count) break;

            handles[++record_id] = mManager.createOppOpsRecord(SDP_SERVER_NAME,
                    record_id, record_id+iteration, SDP_VERSION, SdpManager.OPP_FORMAT_ALL);
            Log.i(TAG, "  Added record_handle=" + handles[record_id]);
            assertTrue(handles[record_id]>=0);
            if(record_id == count) break;

            handles[++record_id] = mManager.createPbapPseRecord(SDP_SERVER_NAME,
                    record_id, record_id+iteration, SDP_VERSION, SDP_REPOS,
                    (int)SDP_FEATURES);
            Log.i(TAG, "  Added record_handle=" + handles[record_id]);
            assertTrue(handles[record_id]>=0);
            if(record_id == count) break;

            handles[++record_id] = mManager.createSapsRecord(SDP_SERVER_NAME,
                    record_id, SDP_VERSION);
            Log.i(TAG, "  Added record_handle=" + handles[record_id]);
            assertTrue(handles[record_id]>=0);
            if (record_id == count) break;
        }
    }

    void removeRecords(int handles[], int record_count) {
        int record_id;
        /* Remove the records */
        for(record_id = 0; record_id < record_count; record_id++) {
            Log.i(TAG, "remove id=" + record_id);
            assertTrue(mManager.removeSdpRecord(handles[record_id]));
        }
    }

    private void addRemoveRecords(int count) {
        int record_count = count;
        int handles[] = new int[record_count];
        int iteration;
        for(iteration = 0; iteration < SDP_ITERATIONS; iteration++) {

            addRecords(handles, iteration);

            try {
                Thread.sleep(500);
            } catch (InterruptedException e) {
                Log.e(TAG, "Interrupted", e);
            }

            removeRecords(handles, record_count);
        }
    }

    /**
     * Client side of SdpSearch test
     * This test will:
     *  1) Create a connection to a test server
     *  2) Create a number of SDP records
     *  3) Request the test server to read the records
     *  4) Remove the records
     *  5) Iterate over 2) to 4) SDP_ITERATIONS number of times
     */
    public void testSdpSearchClient() {
        int count = SDP_RECORD_COUNT;
        int record_count = count;
        int handles[] = new int[record_count];
        int iteration;
        final BluetoothSocket clientSock;
        final ClientSession mClientSession;
        final String[] uuids = {BluetoothUuid.MAS.toString(),
                                BluetoothUuid.MNS.toString(),
                                BluetoothUuid.PBAP_PSE.toString(),
                                BluetoothUuid.ObexObjectPush.toString(),
                                BluetoothUuid.SAP.toString()};
        final String uuids_str;
        final StringBuilder sb = new StringBuilder(uuids.length*2-1);
        for(String str : uuids) {
            sb.append(str).append(";");
        }
        uuids_str = sb.toString();

        try {
            /* This will turn on BT and connect */
            clientSock = ObexTest.connectClientSocket(BluetoothSocket.TYPE_L2CAP, true, mContext);
            mManager = SdpManager.getDefaultManager();
            BluetoothObexTransport clientTransport = new BluetoothObexTransport(clientSock);
            mClientSession = new ClientSession(clientTransport);
            { // Connect
                HeaderSet reqHeaders = new HeaderSet();
                reqHeaders.setHeader(TestSequencer.STEP_INDEX_HEADER, (long)0);
                HeaderSet response = mClientSession.connect(reqHeaders);
                assertEquals(response.responseCode, ResponseCodes.OBEX_HTTP_OK);
            }

            for(iteration = 0; iteration < SDP_ITERATIONS; iteration++) {
                // Add the records
                addRecords(handles, iteration);

                { // get operation to trigger SDP search on peer device
                    HeaderSet reqHeaders = new HeaderSet();
                    reqHeaders.setHeader(TestSequencer.STEP_INDEX_HEADER, (long)iteration);
                    reqHeaders.setHeader(HeaderSet.COUNT, (long)count);
                    reqHeaders.setHeader(HeaderSet.NAME, uuids_str);
                    Operation op = mClientSession.get(reqHeaders);
                    op.noBodyHeader();
                    int response = op.getResponseCode();
                    op.close();
                    assertEquals(response, ResponseCodes.OBEX_HTTP_OK);
                }

                // Cleanup
                removeRecords(handles, record_count);
            }
            { // disconnect to end test
                HeaderSet reqHeaders = new HeaderSet();
                reqHeaders.setHeader(TestSequencer.STEP_INDEX_HEADER, 0L); // signals end of test
                HeaderSet response = mClientSession.disconnect(reqHeaders);
                assertEquals(response.responseCode, ResponseCodes.OBEX_HTTP_OK);
            }
        } catch (IOException e) {
            Log.e(TAG,"IOException in testSdpSearch",e);
        }

    }

    /**
     * Server side of SdpSearch test
     * This test will start a
     *  1) Create a connection to a test server
     *  2) Create a number of SDP records
     *  3) Request the test server to read the records
     *  4) Remove the records
     *  5) Iterate over 2) to 4) SDP_ITERATIONS number of times
     */
    public void testSdpSearchServer() {
        mManager = SdpManager.getDefaultManager();
        try {
            CountDownLatch stopLatch = new CountDownLatch(1);
            BluetoothDevice clientDevice;
            /* This will turn on BT and create a server socket on which accept can be called. */
            BluetoothServerSocket serverSocket=ObexTest.createServerSocket(BluetoothSocket.TYPE_L2CAP, true);
            mManager = SdpManager.getDefaultManager();

            Log.i(TAG, "Waiting for client to connect...");
            BluetoothSocket socket = serverSocket.accept();
            Log.i(TAG, "Client connected...");

            BluetoothObexTransport serverTransport = new BluetoothObexTransport(socket);
            clientDevice = socket.getRemoteDevice();
            ServerSession serverSession = new ServerSession(serverTransport,
                    new SdpManagerTestServer(stopLatch, mContext, clientDevice), null);

            boolean interrupted = false;
            do {
                try {
                    interrupted = false;
                    Log.i(TAG,"Waiting for stopLatch signal...");
                    stopLatch.await();
                } catch (InterruptedException e) {
                    Log.w(TAG,e);
                    interrupted = true;
                }
            } while (interrupted == true);
            Log.i(TAG,"stopLatch signal received closing down...");
            /* Give a little time to transfer the disconnect response before closing the socket */
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {}

            // Cleanup
            serverSession.close();
            socket.close();
            serverSocket.close();
        } catch (IOException e) {
            Log.e(TAG, "IOException", e);
        }
        Log.i(TAG, "\n\n\nTest done - please fetch logs within 30 seconds...\n\n\n");
        try {
            Thread.sleep(30000);
        } catch (InterruptedException e) {}
        Log.i(TAG, "Test done.");
}


/*
 * Tests we need:
 * - Single threaded test:
 *      * Add a large number of records and remove them again.
 * - Multi-threaded rests:
 *      * Let two or more threads perform the test above, each tasking a n-threads fraction of the RECORD_COUNT
 *
 * - Client/server
 *      * Create a control connection - it might be easiest to use OBEX.
 *      1) Add a number of records
 *      2) Trigger read of the records
 *      3) Remove the records
 *      4) Validate they are gone (if they are not cached)
 *      5) Multi thread the test on both sides?
 *  */

}
