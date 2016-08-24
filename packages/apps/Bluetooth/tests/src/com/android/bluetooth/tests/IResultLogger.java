package com.android.bluetooth.tests;

/**
 * The interface for results - makes it easy to replace the result
 * logger implementation at a later point if needed.
 * @author cbonde
 *
 */
public interface IResultLogger {
    /**
     * Add an entry to the result log.
     * To make the first entry count, add a result of 0 bytes
     * transfered then starting the test.
     * Or add a result with 1 byte when e.g. the first byte is received.
     * @param bytesTransfered The amount of bytes transfered
     */
    void addResult(long bytesTransfered);

    /**
     * Get the current average speed of the transfer.
     * (based on the last entry in the log, and not the current time)
     * @return the average speed in bytes/sec
     */
    int getAverageSpeed();

    /**
     * Get the current average speed of the last period of the transfer.
     * (based on the last entry in the log, and not the current time)
     * @param period the period over which the average is taken.
     * @return the average speed in bytes/sec
     */
    int getAverageSpeed(long period);

}
