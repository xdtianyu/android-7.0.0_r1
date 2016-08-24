package com.android.bluetooth.tests;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.concurrent.CountDownLatch;

import javax.obex.HeaderSet;
import javax.obex.Operation;
import javax.obex.ResponseCodes;
import javax.obex.ServerRequestHandler;

import android.util.Log;

public class ObexTestServer extends ServerRequestHandler {

    private static final String TAG = "ObexTestServer";
    private static final boolean V = true;

    ArrayList<SeqStep> mSequence;
    CountDownLatch mStopLatch;

    ObexTestDataHandler mDataHandler;
    int mOperationIndex = 0;

    public ObexTestServer(ArrayList<SeqStep> sequence, CountDownLatch stopLatch) {
        super();
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
        } catch (IOException e) {
            Log.e(TAG, "Exception in onConnect - aborting...");
            result = ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
            // A read from null will produce exception to end the test.
        }
        /* TODO: validate request headers, and set response headers */
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
        } catch (IOException e) {
            Log.e(TAG, "Exception in onDisconnect...");
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
        /* TODO: validate request headers, and set response headers
         *       Also handle pause/abort */
        // 1) Validate request
        // 2) Open the output stream.
        // 3) Receive the data
        // 4) Send response OK
        InputStream inStream;
        int result = ResponseCodes.OBEX_HTTP_OK;
        try{
            inStream = operation.openInputStream();
            HeaderSet reqHeaders = operation.getReceivedHeader();
            int index = ((Long)reqHeaders.getHeader(TestSequencer.STEP_INDEX_HEADER)).intValue();
            mOperationIndex = index;
            mDataHandler.readData(inStream, mSequence.get(index).mParams);
        } catch (IOException e) {
            Log.e(TAG, "Exception in onPut - aborting...");
            inStream = null;
            result = ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
            // A read from null will produce exception to end the test.
        } finally {
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
        /* TODO: validate request headers, and set response headers
         *       Also handle pause/abort */
        // 1) Validate request
        // 2) Open the output stream.
        // 3) Receive the data
        // 4) Send response OK
        OutputStream outStream;
        int result = ResponseCodes.OBEX_HTTP_OK;
        try{
            outStream = operation.openOutputStream();
            HeaderSet reqHeaders = operation.getReceivedHeader();
            int index = ((Long)reqHeaders.getHeader(TestSequencer.STEP_INDEX_HEADER)).intValue();
            mOperationIndex = index;
            mDataHandler.writeData(outStream, mSequence.get(index).mParams);
        } catch (IOException e) {
            Log.e(TAG, "Exception in onGet - aborting...");
            outStream = null;
            result = ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
            // A read from null will produce exception to end the test.
        } finally {
        }
        if(result == ResponseCodes.OBEX_HTTP_OK) {
            Log.i(TAG, "OBEX-HANDLER: operation complete success");
        } else {
            Log.e(TAG, "OBEX-HANDLER: operation complete FAILED!");
        }
        return result;
    }



}
