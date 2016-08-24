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

package com.android.usbtuner.setup;

import android.animation.LayoutTransition;
import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.ConditionVariable;
import android.os.Handler;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.android.tv.common.AutoCloseableUtils;
import com.android.tv.common.ui.setup.SetupFragment;
import com.android.usbtuner.ChannelScanFileParser;
import com.android.usbtuner.ChannelScanFileParser.ScanChannel;
import com.android.usbtuner.FileDataSource;
import com.android.usbtuner.InputStreamSource;
import com.android.usbtuner.R;
import com.android.usbtuner.UsbTunerPreferences;
import com.android.usbtuner.UsbTunerTsScannerSource;
import com.android.usbtuner.data.Channel;
import com.android.usbtuner.data.PsiData;
import com.android.usbtuner.data.PsipData;
import com.android.usbtuner.data.TunerChannel;
import com.android.usbtuner.tvinput.ChannelDataManager;
import com.android.usbtuner.tvinput.EventDetector;

import junit.framework.Assert;

import java.util.ArrayList;
import java.util.List;

/**
 * A fragment for scanning channels.
 */
public class ScanFragment extends SetupFragment {
    private static final String TAG = "ScanFragment";
    private static final boolean DEBUG = false;
    // In the fake mode, the connection to antenna or cable is not necessary.
    // Instead dummy channels are added.
    private static final boolean FAKE_MODE = false;

    public static final String ACTION_CATEGORY = "com.android.usbtuner.setup.ScanFragment";
    public static final int ACTION_CANCEL = 1;
    public static final int ACTION_FINISH = 2;

    public static final String EXTRA_FOR_CHANNEL_SCAN_FILE = "scan_file_choice";

    private static final long CHANNEL_SCAN_SHOW_DELAY_MS = 10000;
    private static final long CHANNEL_SCAN_PERIOD_MS = 4000;
    private static final long SHOW_PROGRESS_DIALOG_DELAY_MS = 300;

    // Build channels out of the locally stored TS streams.
    private static final boolean SCAN_LOCAL_STREAMS = true;

    private ChannelDataManager mChannelDataManager;
    private ChannelScanTask mChannelScanTask;
    private ProgressBar mProgressBar;
    private TextView mScanningMessage;
    private View mChannelHolder;
    private ChannelAdapter mAdapter;
    private volatile boolean mChannelListVisible;
    private Button mCancelButton;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        View view = super.onCreateView(inflater, container, savedInstanceState);
        mChannelDataManager = new ChannelDataManager(getActivity());
        mChannelDataManager.checkDataVersion(getActivity());
        mAdapter = new ChannelAdapter();
        mProgressBar = (ProgressBar) view.findViewById(R.id.tune_progress);
        mScanningMessage = (TextView) view.findViewById(R.id.tune_description);
        ListView channelList = (ListView) view.findViewById(R.id.channel_list);
        channelList.setAdapter(mAdapter);
        channelList.setOnItemClickListener(null);
        ViewGroup progressHolder = (ViewGroup) view.findViewById(R.id.progress_holder);
        LayoutTransition transition = new LayoutTransition();
        transition.enableTransitionType(LayoutTransition.CHANGING);
        progressHolder.setLayoutTransition(transition);
        mChannelHolder = view.findViewById(R.id.channel_holder);
        mCancelButton = (Button) view.findViewById(R.id.tune_cancel);
        mCancelButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                finishScan(false);
            }
        });
        Bundle args = getArguments();
        startScan(args == null ? 0 : args.getInt(EXTRA_FOR_CHANNEL_SCAN_FILE, 0));
        return view;
    }

    @Override
    protected int getLayoutResourceId() {
        return R.layout.ut_channel_scan;
    }

    @Override
    protected int[] getParentIdsForDelay() {
        return new int[] {R.id.progress_holder};
    }

    private void startScan(int channelMapId) {
        mChannelScanTask = new ChannelScanTask(channelMapId);
        mChannelScanTask.execute();
    }

    @Override
    public void onDetach() {
        // Ensure scan task will stop.
        mChannelScanTask.stopScan();
        super.onDetach();
    }

    /**
     * Finishes the current scan thread. This fragment will be popped after the scan thread ends.
     *
     * @param cancel a flag which indicates the scan is canceled or not.
     */
    public void finishScan(boolean cancel) {
        if (mChannelScanTask != null) {
            mChannelScanTask.cancelScan(cancel);

            // Notifies a user of waiting to finish the scanning process.
            new Handler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    mChannelScanTask.showFinishingProgressDialog();
                }
            }, SHOW_PROGRESS_DIALOG_DELAY_MS);

            // Hides the cancel button.
            mCancelButton.setEnabled(false);
        }
    }

    private class ChannelAdapter extends BaseAdapter {
        private final ArrayList<TunerChannel> mChannels;

        public ChannelAdapter() {
            mChannels = new ArrayList<>();
        }

        @Override
        public boolean areAllItemsEnabled() {
            return false;
        }

        @Override
        public boolean isEnabled(int pos) {
            return false;
        }

        @Override
        public int getCount() {
            return mChannels.size();
        }

        @Override
        public Object getItem(int pos) {
            return pos;
        }

        @Override
        public long getItemId(int pos) {
            return pos;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            final Context context = parent.getContext();

            if (convertView == null) {
                LayoutInflater inflater = (LayoutInflater) context
                        .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                convertView = inflater.inflate(R.layout.ut_channel_list, parent, false);
            }

            TextView channelNum = (TextView) convertView.findViewById(R.id.channel_num);
            channelNum.setText(mChannels.get(position).getDisplayNumber());

            TextView channelName = (TextView) convertView.findViewById(R.id.channel_name);
            channelName.setText(mChannels.get(position).getName());
            return convertView;
        }

        public void add(TunerChannel channel) {
            mChannels.add(channel);
            notifyDataSetChanged();
        }
    }

    private class ChannelScanTask extends AsyncTask<Void, Integer, Void>
            implements EventDetector.EventListener {
        private static final int MAX_PROGRESS = 100;

        private final Activity mActivity;
        private final int mChannelMapId;
        private final InputStreamSource mTunerSource;
        private final InputStreamSource mFileSource;
        private final ConditionVariable mConditionStopped;

        private List<ScanChannel> mScanChannelList;
        private boolean mIsCanceled;
        private boolean mIsFinished;
        private ProgressDialog mFinishingProgressDialog;

        public ChannelScanTask(int channelMapId) {
            mActivity = getActivity();
            mChannelMapId = channelMapId;
            mTunerSource = FAKE_MODE ? new FakeInputStreamSource(this)
                    : new UsbTunerTsScannerSource(mActivity.getApplicationContext(), this);
            mFileSource = SCAN_LOCAL_STREAMS ? new FileDataSource(this) : null;
            mConditionStopped = new ConditionVariable();
        }

        private void maybeSetChannelListVisible() {
            mActivity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    int channelsFound = mAdapter.getCount();
                    if (!mChannelListVisible && channelsFound > 0) {
                        String format = getResources().getQuantityString(
                                R.plurals.ut_channel_scan_message, channelsFound, channelsFound);
                        mScanningMessage.setText(String.format(format, channelsFound));
                        mChannelHolder.setVisibility(View.VISIBLE);
                        mChannelListVisible = true;
                    }
                }
            });
        }

        private void addChannel(final TunerChannel channel) {
            mActivity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mAdapter.add(channel);
                    if (mChannelListVisible) {
                        int channelsFound = mAdapter.getCount();
                        String format = getResources().getQuantityString(
                                R.plurals.ut_channel_scan_message, channelsFound, channelsFound);
                        mScanningMessage.setText(String.format(format, channelsFound));
                    }
                }
            });
        }

        private synchronized void finishStanTask() {
            if (!mIsFinished) {
                mIsFinished = true;
                UsbTunerPreferences.setScannedChannelCount(mActivity.getApplicationContext(),
                        mChannelDataManager.getScannedChannelCount());
                // Cancel a previously shown recommendation card.
                TunerSetupActivity.cancelRecommendationCard(mActivity.getApplicationContext());
                // Mark scan as done
                UsbTunerPreferences.setScanDone(mActivity.getApplicationContext());
                // finishing will be done manually.
                if (mFinishingProgressDialog != null) {
                    mFinishingProgressDialog.dismiss();
                }
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        onActionClick(ACTION_CATEGORY, mIsCanceled ? ACTION_CANCEL : ACTION_FINISH);
                    }
                });
            }
        }

        @Override
        protected Void doInBackground(Void... params) {
            mScanChannelList = ChannelScanFileParser.parseScanFile(
                    getResources().openRawResource(mChannelMapId));
            if (SCAN_LOCAL_STREAMS) {
                FileDataSource.addLocalStreamFiles(mScanChannelList);
            }
            scanChannels();
            mChannelDataManager.setCurrentVersion(mActivity);
            mChannelDataManager.release();
            finishStanTask();
            return null;
        }

        @Override
        protected void onProgressUpdate(Integer... values) {
            mProgressBar.setProgress(values[0]);
        }

        private void stopScan() {
            mConditionStopped.open();
        }

        private void cancelScan(boolean cancel) {
            mIsCanceled = cancel;
            stopScan();
        }

        private void scanChannels() {
            if (DEBUG) Log.i(TAG, "Channel scan starting");
            mChannelDataManager.notifyScanStarted();

            long startMs = System.currentTimeMillis();
            int i = 1;
            for (ScanChannel scanChannel : mScanChannelList) {
                int frequency = scanChannel.frequency;
                String modulation = scanChannel.modulation;
                Log.i(TAG, "Tuning to " + frequency + " " + modulation);

                InputStreamSource source = getDataSource(scanChannel.type);
                Assert.assertNotNull(source);
                if (source.setScanChannel(scanChannel)) {
                    source.startStream();
                    mConditionStopped.block(CHANNEL_SCAN_PERIOD_MS);
                    source.stopStream();

                    if (System.currentTimeMillis() > startMs + CHANNEL_SCAN_SHOW_DELAY_MS
                            && !mChannelListVisible) {
                        maybeSetChannelListVisible();
                    }
                }
                if (mConditionStopped.block(-1)) {
                    break;
                }
                onProgressUpdate(MAX_PROGRESS * i++ / mScanChannelList.size());
            }
            AutoCloseableUtils.closeQuietly(mTunerSource);
            AutoCloseableUtils.closeQuietly(mFileSource);
            mChannelDataManager.notifyScanCompleted();
            if (!mConditionStopped.block(-1)) {
                publishProgress(MAX_PROGRESS);
            }
            if (DEBUG) Log.i(TAG, "Channel scan ended");
        }


        private InputStreamSource getDataSource(int type) {
            switch (type) {
                case Channel.TYPE_TUNER:
                    return mTunerSource;
                case Channel.TYPE_FILE:
                    return mFileSource;
                default:
                    return null;
            }
        }

        @Override
        public void onEventDetected(TunerChannel channel, List<PsipData.EitItem> items) {
            mChannelDataManager.notifyEventDetected(channel, items);
        }

        @Override
        public void onChannelDetected(TunerChannel channel, boolean channelArrivedAtFirstTime) {
            if (DEBUG && channelArrivedAtFirstTime) {
                Log.d(TAG, "Found channel " + channel);
            }
            if (channelArrivedAtFirstTime) {
                addChannel(channel);
            }
            mChannelDataManager.notifyChannelDetected(channel, channelArrivedAtFirstTime);
        }

        public synchronized void showFinishingProgressDialog() {
            // Show a progress dialog to wait for the scanning process if it's not done yet.
            if (!mIsFinished && mFinishingProgressDialog == null) {
                mFinishingProgressDialog = ProgressDialog.show(mActivity, "",
                        getString(R.string.ut_setup_cancel), true, false);
            }
        }
    }

    private static class FakeInputStreamSource implements InputStreamSource {
        private final EventDetector.EventListener mEventListener;
        private int mProgramNumber = 0;

        FakeInputStreamSource(EventDetector.EventListener eventListener) {
            mEventListener = eventListener;
        }

        @Override
        public int getType() {
            return 0;
        }

        @Override
        public boolean setScanChannel(ScanChannel channel) {
            return true;
        }

        @Override
        public boolean tuneToChannel(TunerChannel channel) {
            return false;
        }

        @Override
        public void startStream() {
            if (++mProgramNumber % 2 == 1) {
                return;
            }
            final String displayNumber = Integer.toString(mProgramNumber);
            final String name = "Channel-" + mProgramNumber;
            mEventListener.onChannelDetected(new TunerChannel(mProgramNumber,
                    new ArrayList<PsiData.PmtItem>()) {
                @Override
                public String getDisplayNumber() {
                    return displayNumber;
                }

                @Override
                public String getName() {
                    return name;
                }
            }, true);
        }

        @Override
        public void stopStream() {
        }

        @Override
        public long getLimit() {
            return 0;
        }

        @Override
        public long getPosition() {
            return 0;
        }

        @Override
        public void close() {
        }
    }
}
