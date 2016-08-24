/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.bluetooth.gatt;

import android.bluetooth.le.ScanSettings;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;
import java.util.List;

import com.android.bluetooth.btservice.BluetoothProto;
/**
 * ScanStats class helps keep track of information about scans
 * on a per application basis.
 * @hide
 */
/*package*/ class AppScanStats {
    static final DateFormat dateFormat = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss");

    /* ContextMap here is needed to grab Apps and Connections */
    ContextMap contextMap;

    /* GattService is needed to add scan event protos to be dumped later */
    GattService gattService;

    class LastScan {
        long duration;
        long timestamp;
        boolean opportunistic;
        boolean timeout;
        boolean background;
        int results;

        public LastScan(long timestamp, long duration,
                        boolean opportunistic, boolean background) {
            this.duration = duration;
            this.timestamp = timestamp;
            this.opportunistic = opportunistic;
            this.background = background;
            this.results = 0;
        }
    }

    static final int NUM_SCAN_DURATIONS_KEPT = 5;

    // This constant defines the time window an app can scan multiple times.
    // Any single app can scan up to |NUM_SCAN_DURATIONS_KEPT| times during
    // this window. Once they reach this limit, they must wait until their
    // earliest recorded scan exits this window.
    static final long EXCESSIVE_SCANNING_PERIOD_MS = 30 * 1000;

    String appName;
    int scansStarted = 0;
    int scansStopped = 0;
    boolean isScanning = false;
    boolean isRegistered = false;
    long minScanTime = Long.MAX_VALUE;
    long maxScanTime = 0;
    long totalScanTime = 0;
    List<LastScan> lastScans = new ArrayList<LastScan>(NUM_SCAN_DURATIONS_KEPT + 1);
    long startTime = 0;
    long stopTime = 0;
    int results = 0;

    public AppScanStats(String name, ContextMap map, GattService service) {
        appName = name;
        contextMap = map;
        gattService = service;
    }

    synchronized void addResult() {
        if (!lastScans.isEmpty())
            lastScans.get(lastScans.size() - 1).results++;

        results++;
    }

    synchronized void recordScanStart(ScanSettings settings) {
        if (isScanning)
            return;

        this.scansStarted++;
        isScanning = true;
        startTime = System.currentTimeMillis();

        LastScan scan = new LastScan(startTime, 0, false, false);
        if (settings != null) {
          scan.opportunistic = settings.getScanMode() == ScanSettings.SCAN_MODE_OPPORTUNISTIC;
          scan.background = (settings.getCallbackType() & ScanSettings.CALLBACK_TYPE_FIRST_MATCH) != 0;
        }
        lastScans.add(scan);

        BluetoothProto.ScanEvent scanEvent = new BluetoothProto.ScanEvent();
        scanEvent.setScanEventType(BluetoothProto.ScanEvent.SCAN_EVENT_START);
        scanEvent.setScanTechnologyType(BluetoothProto.ScanEvent.SCAN_TECH_TYPE_LE);
        scanEvent.setEventTimeMillis(System.currentTimeMillis());
        scanEvent.setInitiator(truncateAppName(appName));
        gattService.addScanEvent(scanEvent);
    }

    synchronized void recordScanStop() {
        if (!isScanning)
          return;

        this.scansStopped++;
        isScanning = false;
        stopTime = System.currentTimeMillis();
        long scanDuration = stopTime - startTime;

        minScanTime = Math.min(scanDuration, minScanTime);
        maxScanTime = Math.max(scanDuration, maxScanTime);
        totalScanTime += scanDuration;

        LastScan curr = lastScans.get(lastScans.size() - 1);
        curr.duration = scanDuration;

        if (lastScans.size() > NUM_SCAN_DURATIONS_KEPT) {
            lastScans.remove(0);
        }

        BluetoothProto.ScanEvent scanEvent = new BluetoothProto.ScanEvent();
        scanEvent.setScanEventType(BluetoothProto.ScanEvent.SCAN_EVENT_STOP);
        scanEvent.setScanTechnologyType(BluetoothProto.ScanEvent.SCAN_TECH_TYPE_LE);
        scanEvent.setEventTimeMillis(System.currentTimeMillis());
        scanEvent.setInitiator(truncateAppName(appName));
        gattService.addScanEvent(scanEvent);
    }

    synchronized void setScanTimeout() {
        if (!isScanning)
          return;

        if (!lastScans.isEmpty()) {
            LastScan curr = lastScans.get(lastScans.size() - 1);
            curr.timeout = true;
        }
    }

    synchronized boolean isScanningTooFrequently() {
        if (lastScans.size() < NUM_SCAN_DURATIONS_KEPT) {
            return false;
        }

        return (System.currentTimeMillis() - lastScans.get(0).timestamp) <
            EXCESSIVE_SCANNING_PERIOD_MS;
    }

    // This function truncates the app name for privacy reasons. Apps with
    // four part package names or more get truncated to three parts, and apps
    // with three part package names names get truncated to two. Apps with two
    // or less package names names are untouched.
    // Examples: one.two.three.four => one.two.three
    //           one.two.three => one.two
    private String truncateAppName(String name) {
        String initiator = name;
        String[] nameSplit = initiator.split("\\.");
        if (nameSplit.length > 3) {
            initiator = nameSplit[0] + "." +
                        nameSplit[1] + "." +
                        nameSplit[2];
        } else if (nameSplit.length == 3) {
            initiator = nameSplit[0] + "." + nameSplit[1];
        }

        return initiator;
    }

    synchronized void dumpToString(StringBuilder sb) {
        long currTime = System.currentTimeMillis();
        long maxScan = maxScanTime;
        long minScan = minScanTime;
        long scanDuration = 0;

        if (lastScans.isEmpty())
            return;

        if (isScanning) {
            scanDuration = currTime - startTime;
            minScan = Math.min(scanDuration, minScan);
            maxScan = Math.max(scanDuration, maxScan);
        }

        if (minScan == Long.MAX_VALUE) {
            minScan = 0;
        }

        long avgScan = 0;
        if (scansStarted > 0) {
            avgScan = (totalScanTime + scanDuration) / scansStarted;
        }

        LastScan lastScan = lastScans.get(lastScans.size() - 1);
        sb.append("  " + appName);
        if (isRegistered) sb.append(" (Registered)");
        if (lastScan.opportunistic) sb.append(" (Opportunistic)");
        if (lastScan.background) sb.append(" (Background)");
        if (lastScan.timeout) sb.append(" (Forced-Opportunistic)");
        sb.append("\n");

        sb.append("  LE scans (started/stopped)         : " +
                  scansStarted + " / " +
                  scansStopped + "\n");
        sb.append("  Scan time in ms (min/max/avg/total): " +
                  minScan + " / " +
                  maxScan + " / " +
                  avgScan + " / " +
                  totalScanTime + "\n");
        sb.append("  Total number of results            : " +
                  results + "\n");

        if (lastScans.size() != 0) {
            int lastScansSize = scansStopped < NUM_SCAN_DURATIONS_KEPT ?
                                scansStopped : NUM_SCAN_DURATIONS_KEPT;
            sb.append("  Last " + lastScansSize +
                      " scans                       :\n");

            for (int i = 0; i < lastScansSize; i++) {
                LastScan scan = lastScans.get(i);
                Date timestamp = new Date(scan.timestamp);
                sb.append("    " + dateFormat.format(timestamp) + " - ");
                sb.append(scan.duration + "ms ");
                if (scan.opportunistic) sb.append("Opp ");
                if (scan.background) sb.append("Back ");
                if (scan.timeout) sb.append("Forced ");
                sb.append(scan.results + " results");
                sb.append("\n");
            }
        }

        ContextMap.App appEntry = contextMap.getByName(appName);
        if (appEntry != null && isRegistered) {
            sb.append("  Application ID                     : " +
                      appEntry.id + "\n");
            sb.append("  UUID                               : " +
                      appEntry.uuid + "\n");

            if (isScanning) {
                sb.append("  Current scan duration in ms        : " +
                          scanDuration + "\n");
            }

            List<ContextMap.Connection> connections =
              contextMap.getConnectionByApp(appEntry.id);

            sb.append("  Connections: " + connections.size() + "\n");

            Iterator<ContextMap.Connection> ii = connections.iterator();
            while(ii.hasNext()) {
                ContextMap.Connection connection = ii.next();
                long connectionTime = System.currentTimeMillis() - connection.startTime;
                sb.append("    " + connection.connId + ": " +
                          connection.address + " " + connectionTime + "ms\n");
            }
        }
        sb.append("\n");
    }
}
