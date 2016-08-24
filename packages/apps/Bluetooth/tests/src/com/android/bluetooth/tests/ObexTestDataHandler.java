package com.android.bluetooth.tests;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.util.Log;

public class ObexTestDataHandler {

    final String TAG;
    String TAG_BASE = "ObexTestDataHandler";
    static final boolean V = true;

    private static final long PROGRESS_INTERVAL_MS = 1000;
    int mBufferSize = 0;
    int mThrottle = 0;
    long mBytesToTransfer = 0;
    long mBytesTransfered = 0;
    long mStartTime = 0;
    long mLastReport = 0;
    IResultLogger mResults;

    public ObexTestDataHandler(String tag) {
        TAG = TAG_BASE + tag;
    }

    /**
     * Call after a sleep to calculate the number of buffers to
     * send to match the throttle value.
     *
     * @param bufferSize
     * @param throttle
     * @return the number of buffers to send, to match the throttle value
     */
    private int getNumberOfBuffers() {
        if(mThrottle == 0) {
            return 1;
        }
        long deltaT = System.currentTimeMillis() - mStartTime;
        long deltaB = deltaT*mThrottle/1000; // the amount of bytes we should have sent
        long bytesMissing = deltaB-mBytesTransfered;
        return (int)((bytesMissing+(mBufferSize>>1))/mBufferSize); // Round result
    }

    private void publishProgressIfNeeded() {
        long now = System.currentTimeMillis();
        if((now-mLastReport) > PROGRESS_INTERVAL_MS) {
            mLastReport = now;
            String result = "Avg: " + mResults.getAverageSpeed()/1024
                    + " Avg(1s): " + mResults.getAverageSpeed(1000)/1024 +
                    " mBytesTransfered: " + mBytesTransfered + "\n";
            if(V) Log.v(TAG,result);
        }
    }

    public void readData(InputStream inStream, ObexTestParams params) {
        /* TODO: parse in the step params
         * Consider introducing a testStep prepare and wait for completion interface?
         * in stead of using OBEX headers to carry the index... */

        mBufferSize = params.packageSize;
        mThrottle = params.throttle;
        mBytesToTransfer = params.bytesToSend;
        mBytesTransfered = 0;
        mResults = TestResultLogger.createLogger();
        mStartTime = System.currentTimeMillis();

        byte[] buffer = new byte[params.packageSize];
        if(V) Log.v(TAG, "readData() started data to read: " + params.bytesToSend);
        try {
            while(mBytesTransfered < mBytesToTransfer) {
                int nRx = getNumberOfBuffers();
                for(; nRx > 0 ; nRx--) {
                    if(V) Log.v(TAG, "Read()");
                    int count = inStream.read(buffer);
                    if(V) Log.v(TAG, "Read() done - count=" + count);
                    if(count == -1) {
                        throw new IOException("EOF reached too early mBytesTransfered=" + mBytesTransfered);
                    }
                    mBytesTransfered += count;
                    if(mBytesTransfered >= mBytesToTransfer) {
                        nRx=0; // break
                    }
                    mResults.addResult(mBytesTransfered);
                    publishProgressIfNeeded();
                }
                if(mThrottle != 0) {
                    // Sleep one package of time.
                    try {
                        long sleepTime = (1000*mBufferSize)/mThrottle;
                        if(V) Log.v(TAG, "Throttle Sleep():" + sleepTime);
                        Thread.sleep(sleepTime);
                    } catch (InterruptedException e) {
                        // Just ignore as the getNumberOfBuffersToSend will compensate
                        // TODO: Handle Abort
                    }
                }
            }
        }
        catch(IOException e) {
            Log.e(TAG, "Error in readData():",e);
            } finally {
        }
    }

    public void writeData(OutputStream outStream, ObexTestParams params) {
        mBufferSize = params.packageSize;
        mThrottle = params.throttle;
        mBytesToTransfer= params.bytesToSend;
        mBytesTransfered = 0;
        mResults = TestResultLogger.createLogger();
        mStartTime = System.currentTimeMillis();

        byte[] buffer = new byte[params.packageSize];
        if(V) Log.v(TAG, "writeData() started data to write: " + params.bytesToSend);
        try {
            while(mBytesTransfered < mBytesToTransfer) {
                int nTx = getNumberOfBuffers();
                if(V) Log.v(TAG, "Write nTx " + nTx + " packets");
                for(; nTx > 0 ; nTx--) {
                    if(V) Log.v(TAG, "Write()");
                    if((mBytesTransfered + mBufferSize) < mBytesToTransfer) {
                        outStream.write(buffer);
                        mBytesTransfered += mBufferSize;
                    } else {
                        outStream.write(buffer, 0, (int) (mBytesToTransfer-mBytesTransfered));
                        mBytesTransfered += mBytesToTransfer-mBytesTransfered;
                        nTx = 0;
                    }
                    mResults.addResult(mBytesTransfered);
                    publishProgressIfNeeded();
                    if(V) Log.v(TAG, "Write mBytesTransfered: " + mBytesTransfered);
                }
                if(mThrottle != 0) {
                    // Sleep one package of time.
                    try {
                        long sleepTime = (1000*mBufferSize)/mThrottle;
                        if(V) Log.v(TAG, "Throttle Sleep():" + sleepTime);
                        Thread.sleep(sleepTime);
                    } catch (InterruptedException e) {
                        // Just ignore as the getNumberOfBuffersToSend will compensate
                        // TODO: Handle Abort
                    }

                }

            }
        }
        catch(IOException e) {
            Log.e(TAG, "Error in ListenTask:",e);
        }
    }
}
