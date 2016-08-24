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

import android.util.Log;

import java.lang.StringBuilder;
import java.util.List;
import java.util.regex.Pattern;
import java.util.regex.Matcher;

/**
 * This class is like a C-style struct that holds individual process information from the
 * dumpsys graphicsstats command. It also includes an enumeration, originally from the
 * JankTestHelper code, which pattern matches against the dump data to find the relevant
 * information.
 */
public class JankStat {
    private static final String TAG = "JankStat";

    // Patterns used for parsing dumpsys graphicsstats
    public enum StatPattern {
        PACKAGE(Pattern.compile("\\s*Package: (.*)"), 1),
        STATS_SINCE(Pattern.compile("\\s*Stats since: (\\d+)ns"), 1),
        TOTAL_FRAMES(Pattern.compile("\\s*Total frames rendered: (\\d+)"), 1),
        NUM_JANKY(Pattern.compile("\\s*Janky frames: (\\d+) (.*)"), 1),
        FRAME_TIME_50TH(Pattern.compile("\\s*50th percentile: (\\d+)ms"), 1),
        FRAME_TIME_90TH(Pattern.compile("\\s*90th percentile: (\\d+)ms"), 1),
        FRAME_TIME_95TH(Pattern.compile("\\s*95th percentile: (\\d+)ms"), 1),
        FRAME_TIME_99TH(Pattern.compile("\\s*99th percentile: (\\d+)ms"), 1),
        SLOWEST_FRAMES_24H(Pattern.compile("\\s*Slowest frames over last 24h: (.*)"), 1),
        NUM_MISSED_VSYNC(Pattern.compile("\\s*Number Missed Vsync: (\\d+)"), 1),
        NUM_HIGH_INPUT_LATENCY(Pattern.compile("\\s*Number High input latency: (\\d+)"), 1),
        NUM_SLOW_UI_THREAD(Pattern.compile("\\s*Number Slow UI thread: (\\d+)"), 1),
        NUM_SLOW_BITMAP_UPLOADS(Pattern.compile("\\s*Number Slow bitmap uploads: (\\d+)"), 1),
        NUM_SLOW_DRAW(Pattern.compile("\\s*Number Slow issue draw commands: (\\d+)"), 1);

        private Pattern mParsePattern;
        private int mGroupIdx;

        StatPattern(Pattern pattern, int idx) {
            mParsePattern = pattern;
            mGroupIdx = idx;
        }

        String parse(String line) {
            String ret = null;
            Matcher matcher = mParsePattern.matcher(line);
            if (matcher.matches()) {
                ret = matcher.group(mGroupIdx);
            }
            return ret;
        }
    }

    public String packageName;
    public long statsSince;
    public int totalFrames;
    public int jankyFrames;
    public int frameTime50th;
    public int frameTime90th;
    public int frameTime95th;
    public int frameTime99th;
    public String slowestFrames24h;
    public int numMissedVsync;
    public int numHighLatency;
    public int numSlowUiThread;
    public int numSlowBitmap;
    public int numSlowDraw;

    public int aggregateCount;

    public JankStat (String pkg, long since, int total, int janky, int ft50, int ft90, int ft95,
            int ft99, String slow24h, int vsync, int latency, int slowUi, int slowBmp, int slowDraw,
            int aggCount) {
        packageName = pkg;
        statsSince = since;
        totalFrames = total;
        jankyFrames = janky;
        frameTime50th = ft50;
        frameTime90th = ft90;
        frameTime95th = ft95;
        frameTime99th = ft99;
        slowestFrames24h = slow24h;
        numMissedVsync = vsync;
        numHighLatency = latency;
        numSlowUiThread = slowUi;
        numSlowBitmap = slowBmp;
        numSlowDraw = slowDraw;

        aggregateCount = aggCount;
    }

    /**
     * Determines if this set of janks stats is aggregated from the
     * previous set of metrics or if they are a new set, meaning the
     * old process was killed, had its stats reset, and was then
     * restarted.
     */
    public boolean isContinuedFrom (JankStat prevMetrics) {
        return statsSince == prevMetrics.statsSince;
    }

    /**
     * Returns the percent of frames that appeared janky
     */
    public float getPercentJankyFrames () {
        return jankyFrames / (float)totalFrames;
    }

    /**
     * Returns the jank stats similar to how they are presented in the shell
     */
    @Override
    public String toString () {
        String result = packageName +
                "\nStats since: " + statsSince +
                "\nTotal frames: " + totalFrames +
                "\nJanky frames: " + jankyFrames +
                "\n50th percentile: " + frameTime50th +
                "\n90th percentile: " + frameTime90th +
                "\n95th percentile: " + frameTime95th +
                "\n99th percentile: " + frameTime99th +
                "\nSlowest frames over last 24h: " + slowestFrames24h +
                "\nNumber Missed Vsync: " + numMissedVsync +
                "\nNumber High input latency: " + numHighLatency +
                "\nNumber Slow UI thread: " + numSlowUiThread +
                "\nNumber Slow bitmap uploads: " + numSlowBitmap +
                "\nNumber Slow draw commands: " + numSlowDraw +
                "\nAggregated stats count: " + aggregateCount;
        return result;
    }

    /**
     * Merges the stat history of a sequence of stats.
     *
     * Final count value = sum of count values across stats
     * Final ##th percentile = weighted average of ##th, weight by total frames
     *     ## = 90, 95, and 99
     */
    public static JankStat mergeStatHistory (List<JankStat> statHistory) {
        if (statHistory.size() == 0)
            return null;
        else if (statHistory.size() == 1)
            return statHistory.get(0);

        String pkg = statHistory.get(0).packageName;
        long totalStatsSince = statHistory.get(0).statsSince;
        int totalTotalFrames = 0;
        int totalJankyFrames = 0;
        int totalNumMissedVsync = 0;
        int totalNumHighLatency = 0;
        int totalNumSlowUiThread = 0;
        int totalNumSlowBitmap = 0;
        int totalNumSlowDraw = 0;
        String totalSlow24h = "";

        for (JankStat stat : statHistory) {
            totalTotalFrames += stat.totalFrames;
            totalJankyFrames += stat.jankyFrames;
            totalNumMissedVsync += stat.numMissedVsync;
            totalNumHighLatency += stat.numHighLatency;
            totalNumSlowUiThread += stat.numSlowUiThread;
            totalNumSlowBitmap += stat.numSlowBitmap;
            totalNumSlowDraw += stat.numSlowDraw;
            totalSlow24h += stat.slowestFrames24h;
        }

        float wgtAvgPercentile50 = 0f;
        float wgtAvgPercentile90 = 0f;
        float wgtAvgPercentile95 = 0f;
        float wgtAvgPercentile99 = 0f;
        for (JankStat stat : statHistory) {
            float weight = ((float)stat.totalFrames / totalTotalFrames);
            Log.v(TAG, String.format("Calculated weight is %f", weight));
            wgtAvgPercentile90 += stat.frameTime50th * weight;
            wgtAvgPercentile90 += stat.frameTime90th * weight;
            wgtAvgPercentile95 += stat.frameTime95th * weight;
            wgtAvgPercentile99 += stat.frameTime99th * weight;
        }

        int perc50 = (int)Math.ceil(wgtAvgPercentile50);
        int perc90 = (int)Math.ceil(wgtAvgPercentile90);
        int perc95 = (int)Math.ceil(wgtAvgPercentile95);
        int perc99 = (int)Math.ceil(wgtAvgPercentile99);

        return new JankStat(pkg, totalStatsSince, totalTotalFrames,
                totalJankyFrames, perc50, perc90, perc95, perc99, totalSlow24h,
                totalNumMissedVsync, totalNumHighLatency, totalNumSlowUiThread, totalNumSlowBitmap,
                totalNumSlowDraw, statHistory.size());
    }

    /**
     * Returns a long String containing each JankStat object separated by a
     * newline. Ideally, this would omit objects with zero rendered total
     * frames, which is junk data.
     */
    public static String statsListToString (List<JankStat> statsList) {
        StringBuilder result = new StringBuilder();
        for (JankStat stats : statsList) {
            result.append(stats.toString());
            result.append("\n");
        }

        return result.toString();
    }
}
