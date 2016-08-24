package com.android.bluetooth.tests;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.BaseColumns;
import android.test.AndroidTestCase;
import android.util.Log;

import java.text.SimpleDateFormat;
import java.util.Date;

import com.android.bluetooth.mapapi.BluetoothMapContract;
import com.android.bluetooth.mapapi.BluetoothMapContract.ConversationColumns;

//import info.guardianproject.otr.app.im.provider.Imps;
//import info.guardianproject.otr.app.im.provider.ImpsBluetoothProvider;

public class BluetoothMapIMContentTest extends AndroidTestCase {
    private static final String TAG = "BluetoothMapIMContentTest";

    private static final boolean D = true;
    private static final boolean V = true;

    private Context mContext;
    private ContentResolver mResolver;

    private String getDateTimeString(long timestamp) {
        SimpleDateFormat format = new SimpleDateFormat("yyyyMMdd'T'HHmmss");
        Date date = new Date(timestamp);
        return format.format(date); // Format to YYYYMMDDTHHMMSS local time
    }

    private void printCursor(Cursor c) {
        StringBuilder sb = new StringBuilder();
        sb.append("\nprintCursor:\n");
        for(int i = 0; i < c.getColumnCount(); i++) {
            if(c.getColumnName(i).equals(BluetoothMapContract.MessageColumns.DATE) ||
               c.getColumnName(i).equals(BluetoothMapContract.ConversationColumns.LAST_THREAD_ACTIVITY) ||
               c.getColumnName(i).equals(BluetoothMapContract.ChatStatusColumns.LAST_ACTIVE) ||
               c.getColumnName(i).equals(BluetoothMapContract.PresenceColumns.LAST_ONLINE) ){
                sb.append("  ").append(c.getColumnName(i)).append(" : ").append(getDateTimeString(c.getLong(i))).append("\n");
            } else {
                sb.append("  ").append(c.getColumnName(i)).append(" : ").append(c.getString(i)).append("\n");
            }
        }
        Log.d(TAG, sb.toString());
    }

    private void dumpImMessageTable() {
        Log.d(TAG, "**** Dump of im message table ****");

        Cursor c = mResolver.query(
                BluetoothMapContract.buildMessageUri("info.guardianproject.otr.app.im.provider.bluetoothprovider"),
                BluetoothMapContract.BT_INSTANT_MESSAGE_PROJECTION, null, null, "_id DESC");
        if (c != null) {
            Log.d(TAG, "c.getCount() = " + c.getCount());
            c.moveToPosition(-1);
            while (c.moveToNext()) {
                printCursor(c);
            }
        } else {
            Log.d(TAG, "query failed");
            c.close();
        }

    }

    private void insertImMessage( ) {
        Log.d(TAG, "**** Insert message in im message table ****");
        ContentValues cv = new ContentValues();
        cv.put(BluetoothMapContract.MessageColumns.BODY, "This is a test to insert a message");
        cv.put(BluetoothMapContract.MessageColumns.DATE, System.currentTimeMillis());
        cv.put(BluetoothMapContract.MessageColumns.THREAD_ID, 2);
        Uri uri = BluetoothMapContract.buildMessageUri("info.guardianproject.otr.app.im.provider.bluetoothprovider");
        Uri uriWithId = mResolver.insert(uri, cv);
        if (uriWithId != null) {
            Log.d(TAG, "uriWithId = " + uriWithId.toString());
        } else {
            Log.d(TAG, "query failed");
        }

    }

    private void dumpImConversationTable() {
        Log.d(TAG, "**** Dump of conversation message table ****");

        Uri uri = BluetoothMapContract.buildConversationUri(
                "info.guardianproject.otr.app.im.provider.bluetoothprovider", "1");
        uri = uri.buildUpon().appendQueryParameter(BluetoothMapContract.FILTER_ORIGINATOR_SUBSTRING,
                "asp").build();

        Cursor convo = mResolver.query(
                uri,
                BluetoothMapContract.BT_CONVERSATION_PROJECTION, null, null,
                null);

        if (convo != null) {
            Log.d(TAG, "c.getCount() = " + convo.getCount());

            while(convo.moveToNext()) {
                printCursor(convo);
            }
            convo.close();
        } else {
            Log.d(TAG, "query failed");
        }
    }


    private void dumpImContactsTable() {
        Log.d(TAG, "**** Dump of contacts message table ****");
        Cursor cContact = mResolver.query(
                BluetoothMapContract.buildConvoContactsUri("info.guardianproject.otr.app.im.provider.bluetoothprovider","1"),
                BluetoothMapContract.BT_CONTACT_CHATSTATE_PRESENCE_PROJECTION, null, null, "_id DESC");

        if (cContact != null && cContact.moveToFirst()) {
            Log.d(TAG, "c.getCount() = " +  cContact.getCount());
            do {
                printCursor(cContact);
            } while(cContact.moveToNext());

        } else {
            Log.d(TAG, "query failed");
            cContact.close();
        }
    }

    private void dumpImAccountsTable() {
        Log.d(TAG, "**** Dump of accounts table ****");
        Cursor cContact = mResolver.query(
                BluetoothMapContract.buildAccountUri("info.guardianproject.otr.app.im.provider.bluetoothprovider"),
                BluetoothMapContract.BT_ACCOUNT_PROJECTION, null, null, "_id DESC");

        if (cContact != null && cContact.moveToFirst()) {
            Log.d(TAG, "c.getCount() = " +  cContact.getCount());
            do {
                printCursor(cContact);
            } while(cContact.moveToNext());

        } else {
            Log.d(TAG, "query failed");
            cContact.close();
        }
    }


    public BluetoothMapIMContentTest() {
        super();
    }

    public void testDumpMessages() {
        mContext = this.getContext();
        mResolver = mContext.getContentResolver();
        dumpImMessageTable();
        dumpImConversationTable();
        dumpImContactsTable();
        dumpImAccountsTable();

        insertImMessage();

    }

    public void testDumpConversations() {
        mContext = this.getContext();
        mResolver = mContext.getContentResolver();
        dumpImConversationTable();
    }
}