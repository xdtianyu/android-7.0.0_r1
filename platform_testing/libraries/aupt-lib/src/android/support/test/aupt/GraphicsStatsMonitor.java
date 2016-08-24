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

package android.support.test.aupt;

import android.app.UiAutomation;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;

/**
 * GraphicsStatsMonitor is an internal monitor for AUPT to poll and track the information coming out
 * of the shell command, "dumpsys graphicsstats." In particular, the purpose of this is to see jank
 * statistics across the lengthy duration of an AUPT run.
 * <p>
 * To use the monitor, simply specify the options: trackJank true and jankInterval n, where n is
 * the polling interval in milliseconds. The default is 5 minutes. Also, it should be noted that
 * the trackJank option is unnecessary and this comment should be removed at the same time as it.
 * <p>
 * This graphics service monitors jank levels grouped by foreground process. Even when the process
 * is killed, the monitor will continue to track information, unless the buffer runs out of space.
 * This should only occur when too many foreground processes have been killed and the service
 * decides to clear itself. When pulling the information out of the monitor, these separate images
 * are combined to provide a single image as output. The linear information is preserved by simply
 * adding the values together. However, certain information such as the jank percentiles are
 * approximated using a weighted average.
 */
public class GraphicsStatsMonitor {
    private static final String TAG = "GraphicsStatsMonitor";

    public static final int MS_IN_SECS = 1000;
    public static final int SECS_IN_MIN = 60;
    public static final long DEFAULT_INTERVAL_RATE = 5 * SECS_IN_MIN * MS_IN_SECS;

    private Timer mIntervalTimer;
    private TimerTask mIntervalTask;
    private long mIntervalRate;
    private boolean mIsRunning;

    private Map<String, List<JankStat>> mGraphicsStatsRecords;


    public GraphicsStatsMonitor () {
        mIntervalTask = new TimerTask() {
            @Override
            public void run () {
                if (mIsRunning) {
                    grabStatsImage();
                }
            }
        };
        mIntervalRate = DEFAULT_INTERVAL_RATE;
        mIsRunning = false;
    }

    /**
     * Sets the monitoring interval rate if the monitor isn't currently running
     */
    public void setIntervalRate (long intervalRate) {
        if (mIsRunning) {
            Log.e(TAG, "Can't set interval rate for monitor that is already running");
        } else {
            mIntervalRate = intervalRate;
            Log.v(TAG, String.format("Set jank monitor interval rate to %d", intervalRate));
        }
    }

    /**
     * Starts to monitor graphics stats on the interval timer after clearing the stats
     */
    public void startMonitoring () {
        if (mGraphicsStatsRecords == null) {
            mGraphicsStatsRecords = new HashMap<>();
        }

        clearGraphicsStats();

        // Schedule a daemon timer to grab stats periodically
        mIntervalTimer = new Timer(true);
        mIntervalTimer.schedule(mIntervalTask, 0, mIntervalRate);

        mIsRunning = true;
        Log.d(TAG, "Started monitoring graphics stats");
    }

    /**
     * Stops monitoring graphics stats by canceling the interval timer
     */
    public void stopMonitoring () {
        mIntervalTimer.cancel();

        mIsRunning = false;
        Log.d(TAG, "Stopped monitoring graphics stats");
    }

    /**
     * Takes a snapshot of the graphics stats and incorporates them into the process stats history
     */
    public void grabStatsImage () {
        Log.v(TAG, "Grabbing image of graphics stats");
        List<JankStat> allStats = gatherGraphicsStats();

        for (JankStat procStats : allStats) {
            List<JankStat> history;
            if (mGraphicsStatsRecords.containsKey(procStats.packageName)) {
                history = mGraphicsStatsRecords.get(procStats.packageName);
                // Has the process been killed and restarted?
                if (procStats.isContinuedFrom(history.get(history.size() - 1))) {
                    // Process hasn't been killed and restarted; put the data
                    history.set(history.size() - 1, procStats);
                    Log.v(TAG, String.format("Process %s stats have not changed, overwriting data.",
                            procStats.packageName));
                } else {
                    // Process has been killed and restarted; append the data
                    history.add(procStats);
                    Log.v(TAG, String.format("Process %s stats were restarted, appending data.",
                            procStats.packageName));
                }
            } else {
                // Initialize the process stats history list
                history = new ArrayList<>();
                history.add(procStats);
                // Put the history list in the JankStats map
                mGraphicsStatsRecords.put(procStats.packageName, history);
                Log.v(TAG, String.format("New process, %s. Creating jank history.",
                        procStats.packageName));
            }
        }
    }

    /**
     * Aggregates the graphics stats for each process over its history. Merging specifications can
     * be found in the static method {@link JankStat#mergeStatHistory}.
     */
    public List<JankStat> aggregateStatsImages () {
        Log.d(TAG, "Aggregating graphics stats history");
        List<JankStat> mergedStatsList = new ArrayList<JankStat>();

        for (Map.Entry<String, List<JankStat>> record : mGraphicsStatsRecords.entrySet()) {
            String proc = record.getKey();
            List<JankStat> history = record.getValue();

            Log.v(TAG, String.format("Aggregating stats for %s (%d set%s)", proc, history.size(),
                    (history.size() > 1 ? "s" : "")));

            JankStat mergedStats = JankStat.mergeStatHistory(history);
            mergedStatsList.add(mergedStats);
        }

        return mergedStatsList;
    }

    /**
     * Clears all graphics stats history data for all processes
     */
    public void clearStatsImages () {
        mGraphicsStatsRecords.clear();
    }

    /**
     * Resets graphics stats for all currently tracked processes
     */
    public void clearGraphicsStats () {
        Log.d(TAG, "Reset all graphics stats");
        List<JankStat> existingStats = gatherGraphicsStats();
        for (JankStat stat : existingStats) {
            executeShellCommand(String.format("dumpsys gfxinfo %s reset", stat.packageName));
            Log.v(TAG, String.format("Cleared graphics stats for %s", stat.packageName));
        }
    }

    /**
     * Return JankStat objects with metric data for all currently tracked processes
     */
    public List<JankStat> gatherGraphicsStats () {
        Log.v(TAG, "Gather all graphics stats");
        List<JankStat> result = new ArrayList<>();

        BufferedReader stream = executeShellCommand("dumpsys graphicsstats");
        try {
            // Read the stream process by process
            String line;

            while ((line = stream.readLine()) != null) {
                String proc = JankStat.StatPattern.PACKAGE.parse(line);
                if (proc != null) {
                    // "Package: a.b.c"
                    Log.v(TAG, String.format("Found process, %s. Gathering jank info.", proc));
                    // "Stats since: ###ns"
                    line = stream.readLine();
                    Long since = Long.parseLong(JankStat.StatPattern.STATS_SINCE.parse(line));
                    // "Total frames rendered: ###"
                    line = stream.readLine();
                    int total = Integer.valueOf(JankStat.StatPattern.TOTAL_FRAMES.parse(line));
                    // "Janky frames: ## (#.#%)" OR
                    // "Janky frames: ## (nan%)"
                    line = stream.readLine();
                    int janky = Integer.valueOf(JankStat.StatPattern.NUM_JANKY.parse(line));
                    // (optional, N+) "50th percentile: ##ms"
                    line = stream.readLine();
                    int perc50;
                    String parsed50 = JankStat.StatPattern.FRAME_TIME_50TH.parse(line);
                    if (parsed50 != null || !parsed50.isEmpty()) {
                        perc50 = Integer.valueOf(parsed50);
                        line = stream.readLine();
                    } else {
                        perc50 = -1;
                    }
                    // "90th percentile: ##ms"
                    int perc90 = Integer.valueOf(JankStat.StatPattern.FRAME_TIME_90TH.parse(line));
                    // "95th percentile: ##ms"
                    line = stream.readLine();
                    int perc95 = Integer.valueOf(JankStat.StatPattern.FRAME_TIME_95TH.parse(line));
                    // "99th percentile: ##ms"
                    line = stream.readLine();
                    int perc99 = Integer.valueOf(JankStat.StatPattern.FRAME_TIME_99TH.parse(line));
                    // "Slowest frames last 24h: ##ms ##ms ..."
                    line = stream.readLine();
                    String slowest = JankStat.StatPattern.SLOWEST_FRAMES_24H.parse(line);
                    if (slowest != null && !slowest.isEmpty()) {
                        line = stream.readLine();
                    }
                    // "Number Missed Vsync: #"
                    int vsync = Integer.valueOf(JankStat.StatPattern.NUM_MISSED_VSYNC.parse(line));
                    // "Number High input latency: #"
                    line = stream.readLine();
                    int latency = Integer.valueOf(
                            JankStat.StatPattern.NUM_HIGH_INPUT_LATENCY.parse(line));
                    // "Number slow UI thread: #"
                    line = stream.readLine();
                    int ui = Integer.valueOf(JankStat.StatPattern.NUM_SLOW_UI_THREAD.parse(line));
                    // "Number Slow bitmap uploads: #"
                    line = stream.readLine();
                    int bmp = Integer.valueOf(
                            JankStat.StatPattern.NUM_SLOW_BITMAP_UPLOADS.parse(line));
                    // "Number slow issue draw commands: #"
                    line = stream.readLine();
                    int draw = Integer.valueOf(JankStat.StatPattern.NUM_SLOW_DRAW.parse(line));

                    JankStat stat = new JankStat(proc, since, total, janky, perc50, perc90, perc95,
                            perc99, slowest, vsync, latency, ui, bmp, draw, 1);
                    result.add(stat);
                }
            }

            return result;
        } catch (IOException exception) {
            Log.e(TAG, "Error with buffered reader", exception);
            return null;
        } finally {
            try {
                if (stream != null) {
                    stream.close();
                }
            } catch (IOException exception) {
                Log.e(TAG, "Error with closing the stream", exception);
            }
        }
    }

    /**
     * UiAutomation is included solely for the purpose of executing shell commands
     */
    private UiAutomation mUiAutomation;

    /**
     * Executes a shell command through UiAutomation and puts the results in an
     * InputStreamReader that is returned inside a BufferedReader.
     * @param command the command to be executed in the adb shell
     * @result a BufferedReader that reads the command output
     */
    public BufferedReader executeShellCommand (String command) {
        ParcelFileDescriptor stdout = getUiAutomation().executeShellCommand(command);

        BufferedReader stream = new BufferedReader(new InputStreamReader(
                new ParcelFileDescriptor.AutoCloseInputStream(stdout)));
        return stream;
    }

    /**
     * Sets the UiAutomation member for shell execution
     */
    public void setUiAutomation (UiAutomation uiAutomation) {
        mUiAutomation = uiAutomation;
    }

    /**
     * @return UiAutomation instance from Aupt instrumentation
     */
    public UiAutomation getUiAutomation () {
        return mUiAutomation;
    }
}
