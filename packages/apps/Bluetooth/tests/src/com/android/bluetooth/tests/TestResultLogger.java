package com.android.bluetooth.tests;

import java.util.ArrayList;

/**
 * Class for collecting test results, and presenting them in different formats.
 * @author cbonde
 *
 */
public class TestResultLogger implements IResultLogger {

    private ArrayList<Result> mResults;

    private class Result {
        public long timeStamp; // ms precision Unix Time UTC.
        public long receivedBytes;
        public Result(long t, long b) {
            timeStamp = t;
            receivedBytes = b;
        }
    }

    private TestResultLogger() {
        mResults = new ArrayList<Result>(1000);
    }

    public static IResultLogger createLogger(){
        return new TestResultLogger();
    }

    @Override
    public void addResult(long bytesTransfered) {
        mResults.add(new Result(System.currentTimeMillis(), bytesTransfered));
    }

    @Override
    public int getAverageSpeed() {
        if(mResults.size() < 1){
            return 0;
        }
        Result first = mResults.get(0);
        Result last = mResults.get(mResults.size()-1);
        // Multiply by 1000 to convert from ms to sec without loss
        // of precision.
        return (int) ((1000*(last.receivedBytes + first.receivedBytes))/
                (last.timeStamp - first.timeStamp+1));
    }

    /**
     * Optimized to perform best when period is closest to the last
     * result entry.
     * If the period does not match a log entry, an estimate will be made
     * to compensate.
     * If the result log does not contain data to cover the entire period
     * the resulting value will represent the average speed of the content
     * in the log.
     */
    @Override
    public int getAverageSpeed(long period) {
        Result preFirst = null;
        Result first = mResults.get(0);
        int i = mResults.size()-1;
        Result last = mResults.get(i--);
        long firstTimeStamp = last.timeStamp - period;
        if(first.timeStamp > firstTimeStamp || mResults.size() < 3) {
            // Not enough data, use total average
            return getAverageSpeed();
        }
        for(; i >= 0 ; i--) {
            preFirst = mResults.get(i);
            if(preFirst.timeStamp < firstTimeStamp) {
                first = mResults.get(i+1);
                break;
            }
        }
        long timeError = period - (last.timeStamp-first.timeStamp);
        long errorBytes = 0;
        if(timeError > 0) {
            // Find the amount of bytes to use for correction
            errorBytes = (timeError*(preFirst.receivedBytes - first.receivedBytes))
                            /(preFirst.timeStamp - first.timeStamp+1);
        }
        // Calculate the result
        return (int) ((1000*(errorBytes+last.receivedBytes-first.receivedBytes))/period);
    }


}
