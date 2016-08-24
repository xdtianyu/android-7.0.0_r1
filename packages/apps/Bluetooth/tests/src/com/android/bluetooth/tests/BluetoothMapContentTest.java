package com.android.bluetooth.tests;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.LinkedHashMap;

import android.annotation.TargetApi;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Debug;
import android.os.ParcelFileDescriptor;
import android.provider.BaseColumns;
import android.provider.Telephony.Mms;
import android.provider.Telephony.MmsSms;
import android.provider.Telephony.Threads;
import android.test.AndroidTestCase;
import android.util.Log;

import com.android.bluetooth.map.BluetoothMapMasInstance;
import com.android.bluetooth.map.BluetoothMapAccountItem;
import com.android.bluetooth.map.BluetoothMapAccountLoader;
import com.android.bluetooth.map.BluetoothMapAppParams;
import com.android.bluetooth.map.BluetoothMapContent;
import com.android.bluetooth.map.BluetoothMapFolderElement;
import com.android.bluetooth.map.BluetoothMapMessageListing;
import com.android.bluetooth.map.BluetoothMapUtils;
import com.android.bluetooth.map.BluetoothMapUtils.TYPE;
import com.android.bluetooth.map.MapContact;
import com.android.bluetooth.map.SmsMmsContacts;
import com.android.bluetooth.mapapi.BluetoothMapContract;

public class BluetoothMapContentTest extends AndroidTestCase {

    private static final String TAG = "BluetoothMapContentTest";

    private static final boolean D = true;

    private Context mContext;
    private ContentResolver mResolver;
    private SmsMmsContacts mContacts = new SmsMmsContacts();

    private BluetoothMapFolderElement mCurrentFolder;
    private BluetoothMapAccountItem mAccount = null;

    private static final int MAS_ID = 0;
    private static final int REMOTE_FEATURE_MASK = 0x07FFFFFF;
    private static final BluetoothMapMasInstance mMasInstance =
            new MockMasInstance(MAS_ID, REMOTE_FEATURE_MASK);


    private Uri mEmailUri = null;
    private Uri mEmailMessagesUri = null;
    private Uri mEmailFolderUri = null;
    private Uri mEmailAccountUri = null;

    static final String[] EMAIL_ACCOUNT_PROJECTION = new String[] {
        BluetoothMapContract.MessageColumns.FOLDER_ID,
        BluetoothMapContract.MessageColumns.ACCOUNT_ID,
    };

    private void printAccountInfo(Cursor c) {
        if (D) Log.d(TAG, BluetoothMapContract.MessageColumns.ACCOUNT_ID + " : " +
                c.getInt(c.getColumnIndex(BluetoothMapContract.MessageColumns.ACCOUNT_ID)) );
    }

    static final String[] BT_MESSAGE_ID_PROJECTION = new String[] {
        BluetoothMapContract.MessageColumns._ID,
        BluetoothMapContract.MessageColumns.DATE,
    };

    static final String[] BT_MESSAGE_PROJECTION = BluetoothMapContract.BT_MESSAGE_PROJECTION;

    static final String[] BT_ACCOUNT_PROJECTION = BluetoothMapContract.BT_ACCOUNT_PROJECTION;

    static final String[] BT_FOLDER_PROJECTION = BluetoothMapContract.BT_FOLDER_PROJECTION;

    BluetoothMapAccountLoader loader;
    LinkedHashMap<BluetoothMapAccountItem, ArrayList<BluetoothMapAccountItem>> mFullList;

    public BluetoothMapContentTest() {
        super();
    }

    private void initTestSetup(){
        mContext = this.getContext();
        mResolver = mContext.getContentResolver();

        // find enabled account
        loader    = new BluetoothMapAccountLoader(mContext);
        mFullList = loader.parsePackages(false);
        String accountId = getEnabledAccount();
        Uri tmpEmailUri = Uri.parse("content://com.android.email.bluetoothprovider/");

        mEmailUri = Uri.withAppendedPath(tmpEmailUri, accountId + "/");
        mEmailMessagesUri = Uri.parse(mEmailUri + BluetoothMapContract.TABLE_MESSAGE);
        mEmailFolderUri = Uri.parse(mEmailUri + BluetoothMapContract.TABLE_FOLDER);
        mEmailAccountUri = Uri.parse(tmpEmailUri + BluetoothMapContract.TABLE_ACCOUNT);

        buildFolderStructure();

    }

    public String getEnabledAccount(){
        if(D)Log.d(TAG,"getEnabledAccountItems()\n");
        String account = null;
        for(BluetoothMapAccountItem app:mFullList.keySet()){
            ArrayList<BluetoothMapAccountItem> accountList = mFullList.get(app);
            for(BluetoothMapAccountItem acc: accountList){
                mAccount = acc;
                account = acc.getId();
                break;
            }
        }
        return account;
    }

    private void buildFolderStructure(){
        mCurrentFolder = new BluetoothMapFolderElement("root", null); // This will be the root element
        BluetoothMapFolderElement tmpFolder;
        tmpFolder = mCurrentFolder.addFolder("telecom"); // root/telecom
        tmpFolder = tmpFolder.addFolder("msg");          // root/telecom/msg
        if(mEmailFolderUri != null) {
            addEmailFolders(tmpFolder);
        }
    }

    private void addEmailFolders(BluetoothMapFolderElement parentFolder) {
        BluetoothMapFolderElement newFolder;
        String where = BluetoothMapContract.FolderColumns.PARENT_FOLDER_ID +
                        " = " + parentFolder.getFolderId();
        Cursor c = mContext.getContentResolver().query(mEmailFolderUri,
                        BluetoothMapContract.BT_FOLDER_PROJECTION, where, null, null);
        if (c != null) {
            c.moveToPosition(-1);
            while (c.moveToNext()) {
                String name = c.getString(c.getColumnIndex(BluetoothMapContract.FolderColumns.NAME));
                long id = c.getLong(c.getColumnIndex(BluetoothMapContract.FolderColumns._ID));
                newFolder = parentFolder.addEmailFolder(name, id);
                addEmailFolders(newFolder); // Use recursion to add any sub folders
            }
            c.close();
        } else {
            if (D) Log.d(TAG, "addEmailFolders(): no elements found");
        }
    }

    private BluetoothMapFolderElement getInbox() {
        BluetoothMapFolderElement tmpFolderElement = null;

        tmpFolderElement = mCurrentFolder.getSubFolder("telecom");
        tmpFolderElement = tmpFolderElement.getSubFolder("msg");
        tmpFolderElement = tmpFolderElement.getSubFolder("inbox");
        return tmpFolderElement;
    }

    private BluetoothMapFolderElement getOutbox() {
        BluetoothMapFolderElement tmpFolderElement = null;

        tmpFolderElement = mCurrentFolder.getSubFolder("telecom");
        tmpFolderElement = tmpFolderElement.getSubFolder("msg");
        tmpFolderElement = tmpFolderElement.getSubFolder("outbox");
        return tmpFolderElement;
    }


    private String getDateTimeString(long timestamp) {
        SimpleDateFormat format = new SimpleDateFormat("yyyyMMdd'T'HHmmss");
        Date date = new Date(timestamp);
        return format.format(date); // Format to YYYYMMDDTHHMMSS local time
    }

    private void printCursor(Cursor c) {
        StringBuilder sb = new StringBuilder();
        sb.append("\nprintCursor:\n");
        for(int i = 0; i < c.getColumnCount(); i++) {
            if(c.getColumnName(i).equals(BluetoothMapContract.MessageColumns.DATE)){
                sb.append("  ").append(c.getColumnName(i))
                    .append(" : ").append(getDateTimeString(c.getLong(i))).append("\n");
            } else {
                sb.append("  ").append(c.getColumnName(i))
                    .append(" : ").append(c.getString(i)).append("\n");
            }
        }
        Log.d(TAG, sb.toString());
    }

    private void dumpMessageContent(Cursor c) {
        long id = c.getLong(c.getColumnIndex(BluetoothMapContract.MessageColumns._ID));
        Uri uri = Uri.parse(mEmailMessagesUri + "/" + id
                + "/" + BluetoothMapContract.FILE_MSG_NO_ATTACHMENTS);
        FileInputStream is = null;
        ParcelFileDescriptor fd = null;
        int count;
        try {
            fd = mResolver.openFileDescriptor(uri, "r");
            is = new FileInputStream(fd.getFileDescriptor());
            byte[] buffer = new byte[1024];

            while((count = is.read(buffer)) != -1) {
                Log.d(TAG, new String(buffer,0, count));
            }


        } catch (FileNotFoundException e) {
            Log.w(TAG, e);
        } catch (IOException e) {
            Log.w(TAG, e);
        }
        finally {
            try {
               if(is != null)
                    is.close();
            } catch (IOException e) {}
               try {
                if(fd != null)
                    fd.close();
            } catch (IOException e) {}
        }
    }

    /**
     * Create a new message in the database outbox, based on the content of c.
     * @param c
     */
    private void writeMessageContent(Cursor c) {
        long id = c.getLong(c.getColumnIndex(BluetoothMapContract.MessageColumns._ID));
        Uri uri = Uri.parse(mEmailMessagesUri + "/" + id + "/"
                + BluetoothMapContract.FILE_MSG_NO_ATTACHMENTS);
        FileInputStream is = null;
        ParcelFileDescriptor fd = null;
        FileOutputStream os = null;
        ParcelFileDescriptor fdOut = null;

        ContentValues newMessage = new ContentValues();
        BluetoothMapFolderElement outFolder = getOutbox();
        newMessage.put(BluetoothMapContract.MessageColumns.FOLDER_ID, outFolder.getFolderId());
        // Now insert the empty message into outbox (Maybe it should be draft first, and then a move?)
        // TODO: Examine if we need to set some additional flags, e.g. visable?
        Uri uriOut = mResolver.insert(mEmailMessagesUri, newMessage);
        int count;
        try {
            fd = mResolver.openFileDescriptor(uri, "r");
            is = new FileInputStream(fd.getFileDescriptor());
            fdOut = mResolver.openFileDescriptor(uri, "w");
            os = new FileOutputStream(fdOut.getFileDescriptor());
            byte[] buffer = new byte[1024];

            while((count = is.read(buffer)) != -1) {
                Log.d(TAG, new String(buffer,0, count));
                os.write(buffer, 0, count);
            }
        } catch (FileNotFoundException e) {
            Log.w(TAG, e);
        } catch (IOException e) {
            Log.w(TAG, e);
        }
        finally {
            try {
               if(is != null)
                    is.close();
            } catch (IOException e) {}
               try {
                if(fd != null)
                    fd.close();
            } catch (IOException e) {}
               try {
                   if(os != null)
                        os.close();
                } catch (IOException e) {}
                   try {
                    if(fdOut != null)
                        fdOut.close();
                } catch (IOException e) {}
        }
    }

    private void writeMessage(Cursor c) {
        Log.d(TAG, "c.getCount() = " + c.getCount());
            c.moveToPosition(-1);
        if (c.moveToNext()) {
            writeMessageContent(c);
        }
        c.close();
    }


    private void dumpCursor(Cursor c) {
        Log.d(TAG, "c.getCount() = " + c.getCount());
        c.moveToPosition(-1);
        while (c.moveToNext()) {
            printCursor(c);
        }
        c.close();
    }

    private void callBluetoothProvider() {
        Log.d(TAG, "**** Test call into email provider ****");
        int accountId = 0;
        int mailboxId = 0;

        Log.d(TAG, "contentUri = " + mEmailMessagesUri);

        Cursor c = mResolver.query(mEmailMessagesUri, EMAIL_ACCOUNT_PROJECTION,
                null, null, "_id DESC");
        if (c != null) {
            Log.d(TAG, "c.getCount() = " + c.getCount());
            c.moveToPosition(-1);
            while (c.moveToNext()) {
                printAccountInfo(c);
                mailboxId = c.getInt(c.getColumnIndex(
                                BluetoothMapContract.MessageColumns.FOLDER_ID));
                accountId = c.getInt(c.getColumnIndex(
                                BluetoothMapContract.MessageColumns.ACCOUNT_ID));
            }
           c.close();
        } else {
            Log.d(TAG, "query failed");
        }

        final Bundle extras = new Bundle(2);
        /* TODO: find mailbox from DB */
        extras.putLong(BluetoothMapContract.EXTRA_UPDATE_FOLDER_ID, mailboxId);
        extras.putLong(BluetoothMapContract.EXTRA_UPDATE_ACCOUNT_ID, accountId);
        Bundle myBundle = mResolver.call(mEmailUri, BluetoothMapContract.METHOD_UPDATE_FOLDER,
                                            null, extras);
    }


    public void testMsgListing() {
        initTestSetup();
        BluetoothMapContent mBtMapContent = new BluetoothMapContent(mContext, mAccount,
                mMasInstance);
        BluetoothMapAppParams appParams = new BluetoothMapAppParams();
        Log.d(TAG, "**** testMsgListing **** ");
        BluetoothMapFolderElement fe = getInbox();

        if (fe != null) {
            if (D) Log.d(TAG, "folder name=" + fe.getName());

            appParams.setFilterMessageType(0x0B);
            appParams.setMaxListCount(1024);
            appParams.setStartOffset(0);

            BluetoothMapMessageListing msgListing = mBtMapContent.msgListing(fe, appParams);
            int listCount = msgListing.getCount();
            int msgListingSize = mBtMapContent.msgListingSize(fe, appParams);

            if (listCount == msgListingSize) {
                Log.d(TAG, "testMsgListing - " + listCount );
            }
            else {
                Log.d(TAG, "testMsgListing - None");
            }
        }
        else {
            Log.d(TAG, "testMsgListing - failed ");
        }

    }

    public void testMsgListingUnread() {
        initTestSetup();
        BluetoothMapContent mBtMapContent = new BluetoothMapContent(mContext, mAccount,
                mMasInstance);
        BluetoothMapAppParams appParams = new BluetoothMapAppParams();
        Log.d(TAG, "**** testMsgListingUnread **** ");
        BluetoothMapFolderElement fe = getInbox();

        if (fe != null) {

            appParams.setFilterReadStatus(0x01);
            appParams.setFilterMessageType(0x0B);
            appParams.setMaxListCount(1024);
            appParams.setStartOffset(0);

            BluetoothMapMessageListing msgListing = mBtMapContent.msgListing(fe, appParams);

            int listCount = msgListing.getCount();
            if (msgListing.getCount() > 0) {
                Log.d(TAG, "testMsgListingUnread - " + listCount );
            }
            else {
                Log.d(TAG, "testMsgListingUnread - None");
            }
        }
        else {
            Log.d(TAG, "testMsgListingUnread - getInbox failed ");
        }
    }

    public void testMsgListingWithOriginator() {
        initTestSetup();
        BluetoothMapContent mBtMapContent = new BluetoothMapContent(mContext, mAccount,
                mMasInstance);
        BluetoothMapAppParams appParams = new BluetoothMapAppParams();
        Log.d(TAG, "**** testMsgListingUnread **** ");
        BluetoothMapFolderElement fe = getInbox();

        if (fe != null) {

            appParams.setFilterOriginator("*scsc.*");
            appParams.setFilterMessageType(0x0B);
            appParams.setMaxListCount(1024);
            appParams.setStartOffset(0);

            BluetoothMapMessageListing msgListing = mBtMapContent.msgListing(fe, appParams);

            int listCount = msgListing.getCount();
            if (msgListing.getCount() > 0) {
                Log.d(TAG, "testMsgListingWithOriginator - " + listCount );
            }
            else {
                Log.d(TAG, "testMsgListingWithOriginator - None");
            }
        } else {
            Log.d(TAG, "testMsgListingWithOriginator - getInbox failed ");
        }
    }

    public void testGetMessages() {
        initTestSetup();
        BluetoothMapContent mBtMapContent = new BluetoothMapContent(mContext, mAccount,
                mMasInstance);
        BluetoothMapAppParams appParams = new BluetoothMapAppParams();
        Log.d(TAG, "**** testGetMessages **** ");
        BluetoothMapFolderElement fe = getInbox();

        if (fe != null) {
            appParams.setAttachment(0);
            appParams.setCharset(BluetoothMapContent.MAP_MESSAGE_CHARSET_UTF8);

            //get message handles
            Cursor c = mResolver.query(mEmailMessagesUri, BT_MESSAGE_ID_PROJECTION,
                    null, null, "_id DESC");
            if (c != null) {
                c.moveToPosition(-1);
                while (c.moveToNext()) {
                    Long id = c.getLong(c.getColumnIndex(BluetoothMapContract.MessageColumns._ID));
                    String handle = BluetoothMapUtils.getMapHandle(id, TYPE.EMAIL);
                    try {
                        // getMessage
                        byte[] bytes = mBtMapContent.getMessage(handle, appParams, fe, "1.1");
                        Log.d(TAG, "testGetMessages id=" + id + ", handle=" + handle +
                                ", length=" + bytes.length );
                        String testPrint = new String(bytes);
                        Log.d(TAG, "testGetMessage (only dump first part):\n" + testPrint );
                    } catch (UnsupportedEncodingException e) {
                        Log.w(TAG, e);
                    } finally {

                    }
                }
            } else {
                Log.d(TAG, "testGetMessages - no cursor ");
            }
        } else {
            Log.d(TAG, "testGetMessages - getInbox failed ");
        }

    }

    public void testDumpAccounts() {
        initTestSetup();
        Log.d(TAG, "**** testDumpAccounts **** \n from: " + mEmailAccountUri.toString());
        Cursor c = mResolver.query(mEmailAccountUri, BT_ACCOUNT_PROJECTION, null, null, "_id DESC");
        if (c != null) {
            dumpCursor(c);
        } else {
            Log.d(TAG, "query failed");
        }
        Log.w(TAG, "testDumpAccounts(): ThreadId: " + Thread.currentThread().getId());

    }

    public void testAccountUpdate() {
        initTestSetup();
        Log.d(TAG, "**** testAccountUpdate **** \n of: " + mEmailAccountUri.toString());
        Cursor c = mResolver.query(mEmailAccountUri, BT_ACCOUNT_PROJECTION, null, null, "_id DESC");

        if (c != null) {
            c.moveToPosition(-1);
            while (c.moveToNext()) {
                printCursor(c);
                Long id = c.getLong(c.getColumnIndex(BluetoothMapContract.AccountColumns._ID));
                int exposeFlag = c.getInt(
                        c.getColumnIndex(BluetoothMapContract.AccountColumns.FLAG_EXPOSE));
                String where = BluetoothMapContract.AccountColumns._ID + " = " + id;
                ContentValues values = new ContentValues();
                if(exposeFlag == 1) {
                    values.put(BluetoothMapContract.AccountColumns.FLAG_EXPOSE, (int) 0);
                } else {
                    values.put(BluetoothMapContract.AccountColumns.FLAG_EXPOSE, (int) 1);
                }
                Log.i(TAG, "Calling update() with selection: " + where +
                           "values(exposeFlag): " +
                            values.getAsInteger(BluetoothMapContract.AccountColumns.FLAG_EXPOSE));
                mResolver.update(mEmailAccountUri, values, where, null);
            }
            c.close();
        }

    }

    public void testDumpMessages() {
        initTestSetup();

        if (D) Log.d(TAG, "**** testDumpMessages **** \n uri=" + mEmailMessagesUri.toString());
        BluetoothMapFolderElement fe = getInbox();
        if (fe != null)
        {
            String where ="";
            //where = BluetoothMapContract.MessageColumns.FOLDER_ID + " = " + fe.getEmailFolderId();
            Cursor c = mResolver.query(mEmailMessagesUri, BT_MESSAGE_PROJECTION,
                    where, null, "_id DESC");
            if (c != null) {
                dumpCursor(c);
            } else {
                if (D) Log.d(TAG, "query failed");
            }
            if (D) Log.w(TAG, "dumpMessage(): ThreadId: " + Thread.currentThread().getId());
        } else {
            if (D) Log.w(TAG, "dumpMessage(): ThreadId: " + Thread.currentThread().getId());
        }
    }

    public void testDumpMessageContent() {
        initTestSetup();

        Log.d(TAG, "**** testDumpMessageContent **** from: " + mEmailMessagesUri.toString());
//        BluetoothMapFolderElement fe = getInbox();
//        String where = BluetoothMapContract.MessageColumns.FOLDER_ID + " = " + fe.getEmailFolderId();
//        where += " AND " + BluetoothMapContract.MessageColumns.FLAG_HIGH_PRIORITY + " = 0";

        Cursor c = mResolver.query(mEmailMessagesUri, BT_MESSAGE_PROJECTION, null, null, "_id DESC");
        if (c != null && c.moveToNext()) {
            dumpMessageContent(c);
        } else {
            Log.d(TAG, "query failed");
        }
        Log.w(TAG, "dumpMessage(): ThreadId: " + Thread.currentThread().getId());
    }

    public void testWriteMessageContent() {
        initTestSetup();
        Log.d(TAG, "**** testWriteMessageContent **** from: " + mEmailMessagesUri.toString());
        BluetoothMapFolderElement fe = getInbox();
        String where = BluetoothMapContract.MessageColumns.FOLDER_ID + " = " + fe.getFolderId();
//        where += " AND " + BluetoothMapContract.MessageColumns.HIGH_PRIORITY + " = 0";
        Cursor c = mResolver.query(mEmailMessagesUri, BT_MESSAGE_PROJECTION, where, null, "_id DESC");
        if (c != null) {
            writeMessage(c);
        } else {
            Log.d(TAG, "query failed");
        }
        Log.w(TAG, "writeMessage(): ThreadId: " + Thread.currentThread().getId());
    }

    /*
     * Handle test cases
     */
    private static final long HANDLE_TYPE_SMS_CDMA_MASK            = (((long)0x1)<<60);

    public void testHandle() {
        String handleStr = null;
        Debug.startMethodTracing("str_format");
        for(long i = 0; i < 10000; i++) {
            handleStr = String.format("%016X",(i | HANDLE_TYPE_SMS_CDMA_MASK));
        }
        Debug.stopMethodTracing();
        Debug.startMethodTracing("getHandleString");
        for(long i = 0; i < 10000; i++) {
            handleStr = BluetoothMapUtils.getLongAsString(i | HANDLE_TYPE_SMS_CDMA_MASK);
        }
        Debug.stopMethodTracing();
    }

    /*
     * Folder test cases
     */

    public void testDumpEmailFolders() {
        initTestSetup();
        Debug.startMethodTracing();
        String where = null;
        Cursor c = mResolver.query(mEmailFolderUri, BT_FOLDER_PROJECTION, where, null, "_id DESC");
        if (c != null) {
            dumpCursor(c);
            c.close();
        } else {
            Log.d(TAG, "query failed");
        }
        Debug.stopMethodTracing();
    }

    public void testFolderPath() {
        initTestSetup();
        Log.d(TAG, "**** testFolderPath **** ");
        BluetoothMapFolderElement fe = getInbox();
        BluetoothMapFolderElement folder = fe.getFolderById(fe.getFolderId());
        if(folder == null) {
            Log.d(TAG, "**** testFolderPath unable to find the folder with id: " +
                    fe.getFolderId());
        }
        else {
            Log.d(TAG, "**** testFolderPath found the folder with id: " +
                    fe.getFolderId() + "\nFull path: " +
                    folder.getFullPath());
        }
    }

    public void testFolderElement() {
        Log.d(TAG, "**** testFolderElement **** ");
        BluetoothMapFolderElement fe = new BluetoothMapFolderElement("root", null);
        fe = fe.addEmailFolder("MsG", 1);
        fe.addEmailFolder("Outbox", 100);
        fe.addEmailFolder("Sent", 200);
        BluetoothMapFolderElement inbox = fe.addEmailFolder("Inbox", 300);
        fe.addEmailFolder("Draft", 400);
        fe.addEmailFolder("Deleted", 500);
        inbox.addEmailFolder("keep", 301);
        inbox.addEmailFolder("private", 302);
        inbox.addEmailFolder("junk", 303);

        BluetoothMapFolderElement folder = fe.getFolderById(400);
        assertEquals("draft", folder.getName());
        assertEquals("private", fe.getFolderById(302).getName());
        assertEquals("junk", fe.getRoot().getFolderById(303).getName());
        assertEquals("msg/inbox/keep", fe.getFolderById(301).getFullPath());
    }

    /*
     * SMS test cases
     */
    public void testAddSmsEntries() {
        int count = 1000;
        mContext = this.getContext();
        mResolver = mContext.getContentResolver();
        ContentValues values[] = new ContentValues[count];
        long date = System.currentTimeMillis();
        Log.i(TAG, "Preparing messages...");
        for (int x=0;x<count;x++){
            //if (D) Log.d(TAG, "*** Adding dummy sms #"+x);

            ContentValues item = new ContentValues(4);
            item.put("address", "1234");
            item.put("body", "test message "+x);
            item.put("date", date);
            item.put("read", "0");

            values[x] = item;
            // Uri mUri = mResolver.insert(Uri.parse("content://sms"), item);
        }
        Log.i(TAG, "Starting bulk insert...");
        mResolver.bulkInsert(Uri.parse("content://sms"), values);
        Log.i(TAG, "Bulk insert done.");
    }

    public void testAddSms() {
        mContext = this.getContext();
        mResolver = mContext.getContentResolver();
        if (D) Log.d(TAG, "*** Adding dummy sms #");

        ContentValues item = new ContentValues();
        item.put("address", "1234");
        item.put("body", "test message");
        item.put("date", System.currentTimeMillis());
        item.put("read", "0");

        Uri mUri = mResolver.insert(Uri.parse("content://sms"), item);
    }

    public void testServiceSms() {
        mContext = this.getContext();
        mResolver = mContext.getContentResolver();
        if (D) Log.d(TAG, "*** Adding dummy sms #");

        ContentValues item = new ContentValues();
        item.put("address", "C-Bonde");
        item.put("body", "test message");
        item.put("date", System.currentTimeMillis());
        item.put("read", "0");

        Uri mUri = mResolver.insert(Uri.parse("content://sms"), item);
    }

    /*
     * MMS content test cases
     */
    public static final int MMS_FROM = 0x89;
    public static final int MMS_TO = 0x97;
    public static final int MMS_BCC = 0x81;
    public static final int MMS_CC = 0x82;

    private void printMmsAddr(long id) {
        final String[] projection = null;
        String selection = new String("msg_id=" + id);
        String uriStr = String.format("content://mms/%d/addr", id);
        Uri uriAddress = Uri.parse(uriStr);
        Cursor c = mResolver.query(uriAddress, projection, selection, null, null);

        if (c.moveToFirst()) {
            do {
                String add = c.getString(c.getColumnIndex("address"));
                Integer type = c.getInt(c.getColumnIndex("type"));
                if (type == MMS_TO) {
                    if (D) Log.d(TAG, "   recipient: " + add + " (type: " + type + ")");
                } else if (type == MMS_FROM) {
                    if (D) Log.d(TAG, "   originator: " + add + " (type: " + type + ")");
                } else {
                    if (D) Log.d(TAG, "   address other: " + add + " (type: " + type + ")");
                }
                printCursor(c);

            } while(c.moveToNext());
        }
    }

    private void printMmsPartImage(long partid) {
        String uriStr = String.format("content://mms/part/%d", partid);
        Uri uriAddress = Uri.parse(uriStr);
        int ch;
        StringBuffer sb = new StringBuffer("");
        InputStream is = null;

        try {
            is = mResolver.openInputStream(uriAddress);

            while ((ch = is.read()) != -1) {
                sb.append((char)ch);
            }
            if (D) Log.d(TAG, sb.toString());

        } catch (IOException e) {
            // do nothing for now
            e.printStackTrace();
        }
    }

    private void printMmsParts(long id) {
        final String[] projection = null;
        String selection = new String("mid=" + id);
        String uriStr = String.format("content://mms/%d/part", id);
        Uri uriAddress = Uri.parse(uriStr);
        Cursor c = mResolver.query(uriAddress, projection, selection, null, null);

        if (c.moveToFirst()) {
            int i = 0;
            do {
                if (D) Log.d(TAG, "   part " + i++);
                printCursor(c);

                /* if (ct.equals("image/jpeg")) { */
                /*     printMmsPartImage(partid); */
                /* } */
            } while(c.moveToNext());
        }
    }

    public void dumpMmsTable() {
        mContext = this.getContext();
        mResolver = mContext.getContentResolver();

        if (D) Log.d(TAG, "**** Dump of mms table ****");
        Cursor c = mResolver.query(Mms.CONTENT_URI,
                null, null, null, "_id DESC");
        if (c != null) {
            if (D) Log.d(TAG, "c.getCount() = " + c.getCount());
            c.moveToPosition(-1);
            while (c.moveToNext()) {
                Log.d(TAG,"Message:");
                printCursor(c);
                long id = c.getLong(c.getColumnIndex(BaseColumns._ID));
                Log.d(TAG,"Address:");
                printMmsAddr(id);
                Log.d(TAG,"Parts:");
                printMmsParts(id);
            }
            c.close();
        } else {
            Log.d(TAG, "query failed");
        }
    }

    /**
     * This dumps the thread database.
     * Interesting how useful this is.
     *  - DATE is described to be the creation date of the thread. But it actually
     *    contains the time-date of the last activity of the thread.
     *  - RECIPIENTS is a list of the contacts related to the thread. The number can
     *    be found for both MMS and SMS in the "canonical-addresses" table.
     *  - The READ column tells if the thread have been read. (read = 1: no unread messages)
     *  - The snippet is a small piece of text from the last message, and could be used as thread
     *    name. Please however note that if we do this, the version-counter should change each
     *    time a message is added to the thread. But since it changes the read attribute and
     *    last activity, it changes anyway.
     *  -
     */


    public void dumpThreadsTable() {
        mContext = this.getContext();
        mResolver = mContext.getContentResolver();
        mContacts.clearCache();
        Uri uri = Threads.CONTENT_URI.buildUpon().appendQueryParameter("simple", "true").build();

        if (D) Log.d(TAG, "**** Dump of Threads table ****\nUri: " + uri);
        Cursor c = mResolver.query(uri,
                null, null, null, "_id DESC");
        if (c != null) {
            if (D) Log.d(TAG, "c.getCount() = " + c.getCount());
            c.moveToPosition(-1);
            while (c.moveToNext()) {
                Log.d(TAG,"Threads:");
                printCursor(c);
                String ids = c.getString(c.getColumnIndex(Threads.RECIPIENT_IDS));
                Log.d(TAG,"Address:");
                printAddresses(ids);
/*                Log.d(TAG,"Parts:");
                printMmsParts(id);*/
            }
            c.close();
        } else {
            Log.d(TAG, "query failed");
        }
    }

    /**
     * This test shows the content of the canonicalAddresses table.
     * Conclusion:
     * The _id column matches the id's from the RECIPIENT_IDS column
     * in the Threads table, hence are to be used to map from an id to
     * a phone number, which then can be matched to a contact.
     */
    public void dumpCanAddrTable() {
        mContext = this.getContext();
        mResolver = mContext.getContentResolver();
        Uri uri = Uri.parse("content://mms-sms/canonical-addresses");
        uri = MmsSms.CONTENT_URI.buildUpon().appendPath("canonical-addresses").build();
        dumpUri(uri);
    }

    public void dumpUri(Uri uri) {
        if (D) Log.d(TAG, "**** Dump of table ****\nUri: " + uri);
        Cursor c = mResolver.query(uri, null, null, null, null);
        if (c != null) {
            if (D) Log.d(TAG, "c.getCount() = " + c.getCount());
            c.moveToPosition(-1);
            while (c.moveToNext()) {
                Log.d(TAG,"Entry: " + c.getPosition());
                printCursor(c);
            }
            c.close();
        } else {
            Log.d(TAG, "query failed");
        }
    }

    private void printAddresses(String idsStr) {
        String[] ids = idsStr.split(" ");
        for (String id : ids) {
            long longId;
            try {
                longId = Long.parseLong(id);
                String addr = mContacts.getPhoneNumber(mResolver, longId);
                MapContact contact = mContacts.getContactNameFromPhone(addr, mResolver);
                Log.d(TAG, "  id " + id + ": " + addr + " - " + contact.getName()
                        + "  X-BT-UID: " + contact.getXBtUidString());
            } catch (NumberFormatException ex) {
                // skip this id
                continue;
            }
        }
    }

}
