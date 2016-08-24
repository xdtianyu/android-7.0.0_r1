/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.cts.verifier.backup;

import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.backup.BackupManager;
import android.app.backup.FileBackupHelper;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Color;
import android.os.AsyncTask;
import android.os.Bundle;
import android.provider.Settings;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.BaseAdapter;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.Scanner;

/**
 * Test for checking whether Accessibility Settings are being backed up properly. It lists the
 * values of the accessibility preferences that should get backed up and restored after running the
 * backup manager and reinstalling the CTS verifier.
 */
public class BackupAccessibilityTestActivity extends PassFailButtons.ListActivity {

    private static final String TAG = BackupAccessibilityTestActivity.class.getSimpleName();

    private static final int INSTRUCTIONS_DIALOG_ID = 1;

    private static final List<String> ACCESSIBILITY_SETTINGS = new ArrayList();
    private static final List<String> COLOR_CORRECTION_SETTINGS = new ArrayList();
    private static final List<String> ACCESSIBILITY_SERVICE_SETTINGS = new ArrayList();
    private static final List<String> CAPTIONS_SETTINGS = new ArrayList();
    private static final List<String> TTS_SETTINGS = new ArrayList();
    private static final List<String> SYSTEM_SETTINGS = new ArrayList();

    static {
        ACCESSIBILITY_SETTINGS.add("accessibility_display_magnification_enabled");
        ACCESSIBILITY_SETTINGS.add("accessibility_autoclick_enabled");
        ACCESSIBILITY_SETTINGS.add("accessibility_autoclick_delay");
        ACCESSIBILITY_SETTINGS.add("high_text_contrast_enabled");
        ACCESSIBILITY_SETTINGS.add("incall_power_button_behavior");
        ACCESSIBILITY_SETTINGS.add(Settings.Secure.ACCESSIBILITY_SPEAK_PASSWORD);
        ACCESSIBILITY_SETTINGS.add("accessibility_large_pointer_icon");
        ACCESSIBILITY_SETTINGS.add("long_press_timeout");
        ACCESSIBILITY_SETTINGS.add(Settings.Secure.ACCESSIBILITY_DISPLAY_INVERSION_ENABLED);

        COLOR_CORRECTION_SETTINGS.add("accessibility_display_daltonizer");
        COLOR_CORRECTION_SETTINGS.add("accessibility_display_daltonizer_enabled");

        CAPTIONS_SETTINGS.add("accessibility_captioning_preset");
        CAPTIONS_SETTINGS.add("accessibility_captioning_enabled");
        CAPTIONS_SETTINGS.add("accessibility_captioning_locale");
        CAPTIONS_SETTINGS.add("accessibility_captioning_background_color");
        CAPTIONS_SETTINGS.add("accessibility_captioning_foreground_color");
        CAPTIONS_SETTINGS.add("accessibility_captioning_edge_type");
        CAPTIONS_SETTINGS.add("accessibility_captioning_edge_color");
        CAPTIONS_SETTINGS.add("accessibility_captioning_typeface");
        CAPTIONS_SETTINGS.add("accessibility_captioning_font_scale");
        CAPTIONS_SETTINGS.add("accessibility_captioning_window_color");

        TTS_SETTINGS.add(Settings.Secure.TTS_DEFAULT_RATE);
        TTS_SETTINGS.add("tts_default_locale");

        ACCESSIBILITY_SERVICE_SETTINGS.add(Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES);
        ACCESSIBILITY_SERVICE_SETTINGS.add("touch_exploration_granted_accessibility_services");
        ACCESSIBILITY_SERVICE_SETTINGS.add(Settings.Secure.TOUCH_EXPLORATION_ENABLED);

        SYSTEM_SETTINGS.add(Settings.System.FONT_SCALE);
        SYSTEM_SETTINGS.add(Settings.System.STAY_ON_WHILE_PLUGGED_IN);
        SYSTEM_SETTINGS.add(Settings.System.SCREEN_OFF_TIMEOUT);
        SYSTEM_SETTINGS.add(Settings.System.SCREEN_BRIGHTNESS);
        SYSTEM_SETTINGS.add(Settings.System.SCREEN_BRIGHTNESS_MODE);
        SYSTEM_SETTINGS.add(Settings.System.TEXT_SHOW_PASSWORD);
        SYSTEM_SETTINGS.add(Settings.System.HAPTIC_FEEDBACK_ENABLED);
        SYSTEM_SETTINGS.add("power_sounds_enabled");
        SYSTEM_SETTINGS.add("lockscreen_sounds_enabled");
        SYSTEM_SETTINGS.add("pointer_speed");
        SYSTEM_SETTINGS.add(Settings.System.VIBRATE_WHEN_RINGING);
        SYSTEM_SETTINGS.add(Settings.System.ACCELEROMETER_ROTATION);
    }

    private BackupAdapter mAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
        setContentView(R.layout.bua_main);
        setPassFailButtonClickListeners();
        setInfoResources(R.string.backup_accessibility_test, R.string.backup_accessibility_info, 0);

        mAdapter = new BackupAdapter(this);
        setListAdapter(mAdapter);

        new ReadCurrentSettingsValuesTask().execute();

        findViewById(R.id.generate_button).setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                new ReadCurrentSettingsValuesTask().execute();
            }
        });

        findViewById(R.id.show_instructions_button).setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                showDialog(INSTRUCTIONS_DIALOG_ID);
            }
        });
    }

    class ReadCurrentSettingsValuesTask extends AsyncTask<Void, Void, List<BackupItem>> {

        @Override
        protected void onPreExecute() {
            super.onPreExecute();
            setProgressBarIndeterminateVisibility(true);
        }

        @Override
        protected List<BackupItem> doInBackground(Void... params) {
            List<BackupItem> items = new ArrayList<BackupItem>();

            items.add(new CategoryBackupItem(R.string.bua_settings));
            addSecureSettings(items, ACCESSIBILITY_SETTINGS);

            items.add(new CategoryBackupItem(R.string.bua_settings_color_correction));
            addSecureSettings(items, COLOR_CORRECTION_SETTINGS);

            items.add(new CategoryBackupItem(R.string.bua_settings_captions));
            addSecureSettings(items, CAPTIONS_SETTINGS);

            items.add(new CategoryBackupItem(R.string.bua_settings_tts));
            addSecureSettings(items, TTS_SETTINGS);

            items.add(new CategoryBackupItem(R.string.bua_settings_accessibility_services));
            addSecureSettings(items, ACCESSIBILITY_SERVICE_SETTINGS);

            items.add(new CategoryBackupItem(R.string.bua_settings_system));
            addSystemSettings(items, SYSTEM_SETTINGS);

            return items;
        }

        private void addSecureSettings(List<BackupItem> items, List<String> settings) {
            for (String setting : settings) {
                String value = Settings.Secure.getString(getContentResolver(), setting);
                items.add(new PreferenceBackupItem(setting, value));
            }
        }

        private void addSystemSettings(List<BackupItem> items, List<String> settings) {
            for (String setting : settings) {
                String value = Settings.System.getString(getContentResolver(), setting);
                items.add(new PreferenceBackupItem(setting, value));
            }
        }

        @Override
        protected void onPostExecute(List<BackupItem> result) {
            super.onPostExecute(result);
            setProgressBarIndeterminateVisibility(false);
            mAdapter.clear();
            mAdapter.addAll(result);
        }
    }

    @Override
    public Dialog onCreateDialog(int id, Bundle args) {
        switch (id) {
            case INSTRUCTIONS_DIALOG_ID:
                return new AlertDialog.Builder(this)
                    .setIcon(android.R.drawable.ic_dialog_info)
                    .setTitle(R.string.backup_accessibility_test)
                    .setMessage(R.string.bua_instructions)
                    .setPositiveButton(android.R.string.ok, null)
                    .setNeutralButton(R.string.bu_settings, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            startActivity(new Intent(Settings.ACTION_PRIVACY_SETTINGS));
                        }
                    }).create();

            default:
                return super.onCreateDialog(id, args);
        }
    }

    interface BackupItem {
        int getViewType();
        View getView(LayoutInflater inflater, int position, View convertView, ViewGroup parent);
    }

    static class CategoryBackupItem implements BackupItem {

        private final int mTitleResId;

        CategoryBackupItem(int titleResId) {
            mTitleResId = titleResId;
        }

        @Override
        public int getViewType() {
            return 0;
        }

        @Override
        public View getView(LayoutInflater inflater, int position, View convertView,
                ViewGroup parent) {
            TextView view = (TextView) convertView;
            if (convertView == null) {
                view = (TextView) inflater.inflate(R.layout.test_category_row, parent, false);
            }
            view.setText(mTitleResId);
            view.setAllCaps(true);
            view.setTextAppearance(1);  // Bold
            return view;
        }
    }

    static class PreferenceBackupItem implements BackupItem {

        private final String mName;
        private final String mValue;

        PreferenceBackupItem(String name, String value) {
            mName = name;
            mValue = value;
        }

        @Override
        public int getViewType() {
            if (mValue == null || mValue.equals("0")) {
                return 1;
            } else {
                return 2;
            }
        }

        @Override
        public View getView(LayoutInflater inflater, int position, View convertView,
                ViewGroup parent) {
            TextView view = (TextView) convertView;
            if (convertView == null) {
                view = (TextView) inflater.inflate(R.layout.bu_preference_row, parent, false);
            }
            view.setText(mName + " : " + mValue);
            if (mValue == null || mValue.equals("0")) {
                view.setTextColor(Color.GREEN);
            }
            return view;
        }
    }

    class BackupAdapter extends BaseAdapter {

        private final LayoutInflater mLayoutInflater;

        private final List<BackupItem> mItems = new ArrayList<BackupItem>();

        public BackupAdapter(Context context) {
            mLayoutInflater = (LayoutInflater) context.getSystemService(LAYOUT_INFLATER_SERVICE);
        }

        public void clear() {
            mItems.clear();
        }

        public void addAll(List<BackupItem> items) {
            mItems.addAll(items);
            notifyDataSetChanged();
        }

        @Override
        public int getCount() {
            return mItems.size();
        }

        @Override
        public BackupItem getItem(int position) {
            return mItems.get(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public boolean isEnabled(int position) {
            return false;
        }

        @Override
        public int getViewTypeCount() {
            return 3;
        }

        @Override
        public int getItemViewType(int position) {
            return getItem(position).getViewType();
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            return getItem(position).getView(mLayoutInflater, position, convertView, parent);
        }
    }
}
