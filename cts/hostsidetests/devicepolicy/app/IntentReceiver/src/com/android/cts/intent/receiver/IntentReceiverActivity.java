/*
 * Copyright (C) 2014 The Android Open Source Project
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
package com.android.cts.intent.receiver;

import android.app.Activity;
import android.content.ClipboardManager;
import android.content.ClipData;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;

/**
 * Class to receive intents sent across profile boundaries, and read/write to content uri specified
 * in these intents to test cross-profile content uris.
 */
public class IntentReceiverActivity extends Activity {

    private static final String TAG = "IntentReceiverActivity";

    private static final String ACTION_COPY_TO_CLIPBOARD =
            "com.android.cts.action.COPY_TO_CLIPBOARD";

    private static final String ACTION_READ_FROM_URI =
            "com.android.cts.action.READ_FROM_URI";

    private static final String ACTION_TAKE_PERSISTABLE_URI_PERMISSION =
            "com.android.cts.action.TAKE_PERSISTABLE_URI_PERMISSION";

    private static final String ACTION_WRITE_TO_URI =
            "com.android.cts.action.WRITE_TO_URI";

    private static final String ACTION_JUST_CREATE =
            "com.android.cts.action.JUST_CREATE";

    public static final String RECEIVING_ACTIVITY_CREATED_ACTION
            = "com.android.cts.deviceowner.RECEIVING_ACTIVITY_CREATED_ACTION";

    private static final String EXTRA_CAUGHT_SECURITY_EXCEPTION = "extra_caught_security_exception";

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final Intent received = getIntent();
        final String action = received.getAction();
        final ClipData clipData = getIntent().getClipData();
        final Uri uri = clipData != null ? clipData.getItemAt(0).getUri() : null;
        if (ACTION_COPY_TO_CLIPBOARD.equals(action)) {
            String text = received.getStringExtra("extra_text");
            Log.i(TAG, "Copying \"" + text + "\" to the clipboard");
            ClipData clip = ClipData.newPlainText("", text);
            ClipboardManager clipboard =
                    (ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
            clipboard.setPrimaryClip(clip);
            setResult(Activity.RESULT_OK);
        } else if (ACTION_READ_FROM_URI.equals(action)) {
            Intent result = new Intent();
            String message = null;
            try {
                message = getFirstLineFromUri(uri);
            } catch (SecurityException e) {
                Log.i(TAG, "Caught a SecurityException while trying to read " + uri, e);
                result.putExtra(EXTRA_CAUGHT_SECURITY_EXCEPTION, true);
            } catch (IOException e) {
                Log.i(TAG, "Caught a IOException while trying to read " + uri, e);
            }
            Log.i(TAG, "Message received in reading test: " + message);
            result.putExtra("extra_response", message);
            setResult(Activity.RESULT_OK, result);
        } else if (ACTION_TAKE_PERSISTABLE_URI_PERMISSION.equals(action)) {
            Log.i(TAG, "Taking persistable uri permission to " + uri);
            getContentResolver().takePersistableUriPermission(uri,
                    Intent.FLAG_GRANT_READ_URI_PERMISSION);
            setResult(Activity.RESULT_OK);
        } else if (ACTION_WRITE_TO_URI.equals(action)) {
            Intent result = new Intent();
            String message = received.getStringExtra("extra_message");
            Log.i(TAG, "Message received in writing test: " + message);
            try {
                writeToUri(uri, message);
            } catch (SecurityException e) {
                Log.i(TAG, "Caught a SecurityException while trying to write to " + uri, e);
                result.putExtra(EXTRA_CAUGHT_SECURITY_EXCEPTION, true);
            } catch (IOException e) {
                Log.i(TAG, "Caught a IOException while trying to write to " + uri, e);
            }
            setResult(Activity.RESULT_OK, result);
        } else if (ACTION_JUST_CREATE.equals(action)) {
            sendBroadcast(new Intent(RECEIVING_ACTIVITY_CREATED_ACTION));
        }
        finish();
    }

    /**
     * Returns the first line of the file associated with uri.
     */
    private String getFirstLineFromUri(Uri uri) throws IOException {
        InputStream is = getContentResolver().openInputStream(uri);
        BufferedReader r = new BufferedReader(new InputStreamReader(is));
        return r.readLine();
    }

    private void writeToUri(Uri uri, String text) throws IOException {
        OutputStreamWriter writer = new OutputStreamWriter(
                getContentResolver().openOutputStream(uri));
        writer.write(text);
        writer.flush();
        writer.close();
    }
}
