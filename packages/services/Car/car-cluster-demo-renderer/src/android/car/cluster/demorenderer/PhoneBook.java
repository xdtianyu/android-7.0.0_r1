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

package android.car.cluster.demorenderer;

import static android.provider.ContactsContract.Contacts.openContactPhotoInputStream;

import android.annotation.Nullable;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.content.CursorLoader;
import android.content.Loader;
import android.content.Loader.OnLoadCompleteListener;
import android.content.res.Resources;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.BitmapFactory.Options;
import android.graphics.Rect;
import android.net.Uri;
import android.os.AsyncTask;
import android.provider.ContactsContract;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.PhoneLookup;
import android.telephony.PhoneNumberUtils;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;
import android.util.LruCache;

import java.io.InputStream;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;

/**
 * Class that provides contact information.
 */
class PhoneBook {

    private final static String TAG = PhoneBook.class.getSimpleName();

    private final ContentResolver mContentResolver;
    private final Context mContext;
    private final TelephonyManager mTelephonyManager;
    private final Object mSyncContact = new Object();
    private final Object mSyncPhoto = new Object();

    private volatile String mVoiceMail;

    private static final String[] CONTACT_ID_PROJECTION = new String[] {
            PhoneLookup.DISPLAY_NAME,
            PhoneLookup.TYPE,
            PhoneLookup.LABEL,
            PhoneLookup._ID
    };

    private HashMap<String, Contact> mContactByNumber;
    private LruCache<Integer, Bitmap> mContactPhotoById;
    private Set<Integer> mContactsWithoutImage;

    PhoneBook(Context context, TelephonyManager telephonyManager) {
        mContentResolver = context.getContentResolver();
        mContext = context;
        mTelephonyManager = telephonyManager;
    }

    /**
     * Formats provided number according to current locale.
     * */
    public static String getFormattedNumber(String number) {
        if (TextUtils.isEmpty(number)) {
            return "";
        }

        String countryIso = Locale.getDefault().getCountry();
        if (countryIso == null || countryIso.length() != 2) {
            countryIso = "US";
        }
        String e164 = PhoneNumberUtils.formatNumberToE164(number, countryIso);
        String formattedNumber = PhoneNumberUtils.formatNumber(number, e164, countryIso);
        formattedNumber = TextUtils.isEmpty(formattedNumber) ? number : formattedNumber;
        return formattedNumber;
    }

    /**
     * Loads contact details for a given phone number asynchronously. It may call listener's
     * callback function immediately if there were image in the cache.
     */
    public void getContactDetailsAsync(String number, ContactLoadedListener listener) {
        if (number == null || number.isEmpty()) {
            listener.onContactLoaded(number, null);
            return;
        }

        synchronized (mSyncContact) {
            if (mContactByNumber == null) {
                mContactByNumber = new HashMap<>();
            } else if (mContactByNumber.containsKey(number)) {
                listener.onContactLoaded(number, mContactByNumber.get(number));
                return;
            }
        }

        fetchContactAsync(number, listener);
    }

    /**
     * Loads photo for a given contactId asynchronously. It may call listener's callback function
     * immediately if there were image in the cache.
     */
    public void getContactPictureAsync(int contactId, ContactPhotoLoadedListener listener) {
        synchronized (mSyncPhoto) {
            if (mContactsWithoutImage != null && mContactsWithoutImage.contains(contactId)) {
                listener.onPhotoLoaded(contactId, null);
                return;
            }

            if (mContactPhotoById == null) {
                mContactPhotoById = new LruCache<Integer, Bitmap>(4 << 20 /* 4mb */) {
                    @Override
                    protected int sizeOf(Integer key, Bitmap value) {
                        return value.getByteCount();
                    }
                };
            } else {
                Bitmap photo = mContactPhotoById.get(contactId);
                if (photo != null) {
                    listener.onPhotoLoaded(contactId, photo);
                    return;
                }
            }
        }

        fetchPhotoAsync(contactId, listener);
    }

    /** Returns true if given phone number is a voice mail number. */
    public boolean isVoicemail(String number) {
        return !TextUtils.isEmpty(number) && number.equals(getVoiceMailNumber());
    }

    @Nullable
    private String getVoiceMailNumber() {
        if (mVoiceMail == null) {
            mVoiceMail = mTelephonyManager.getVoiceMailNumber();
        }

        return mVoiceMail;
    }

    interface ContactLoadedListener {
        void onContactLoaded(String number, @Nullable Contact contact);
    }

    interface ContactPhotoLoadedListener {
        void onPhotoLoaded(int contactId, @Nullable Bitmap picture);
    }

    private void fetchContactAsync(String number, ContactLoadedListener listener) {
        CursorLoader cursorLoader = new CursorLoader(mContext);
        cursorLoader.setUri(Uri.withAppendedPath(
                PhoneLookup.CONTENT_FILTER_URI,
                Uri.encode(number)));
        cursorLoader.setProjection(CONTACT_ID_PROJECTION);
        cursorLoader.registerListener(0, new LoadCompleteListener(this, number, listener));
        cursorLoader.startLoading();
    }

    private void fetchPhotoAsync(int contactId, ContactPhotoLoadedListener listener) {
        LoadPhotoAsyncTask.createAndExecute(this, contactId, listener);
    }

    private void cacheContactPhoto(int contactId, Bitmap bitmap) {
        synchronized (mSyncPhoto) {
            if (bitmap != null) {
                mContactPhotoById.put(contactId, bitmap);
            } else {
                if (mContactsWithoutImage == null) {
                    mContactsWithoutImage = new HashSet<>();
                }
                mContactsWithoutImage.add(contactId);
            }
        }
    }

    static class Contact {
        private final int mId;
        private final String mName;
        private final CharSequence mType;
        private final String mNumber;

        Contact(Resources resources, String number, int id, String name, String label, int type) {
            mNumber = number;
            mId = id;
            mName = name;
            mType = Phone.getTypeLabel(resources, type, label);
        }

        int getId() {
            return mId;
        }

        public String getName() {
            return mName;
        }

        public CharSequence getType() {
            return mType;
        }

        public String getNumber() { return mNumber; }
    }

    private static class LoadPhotoAsyncTask extends AsyncTask<Void, Void, Bitmap> {

        private final WeakReference<PhoneBook> mPhoneBookRef;
        private final ContactPhotoLoadedListener mListener;
        private final int mContactId;

        static void createAndExecute(PhoneBook phoneBook, int contactId,
                ContactPhotoLoadedListener listener) {
            new LoadPhotoAsyncTask(phoneBook, contactId, listener)
                    .execute();
        }

        private LoadPhotoAsyncTask(PhoneBook phoneBook, int contactId,
                ContactPhotoLoadedListener listener) {
            mPhoneBookRef = new WeakReference<>(phoneBook);
            mContactId = contactId;
            mListener = listener;
        }

        @Nullable
        private Bitmap fetchBitmap(int contactId) {
            Log.d(TAG, "fetchBitmap, contactId: " + contactId);
            PhoneBook phoneBook = mPhoneBookRef.get();
            if (phoneBook == null) {
                return null;
            }

            Uri uri = ContentUris.withAppendedId(ContactsContract.Contacts.CONTENT_URI, contactId);
            InputStream photoDataStream = openContactPhotoInputStream(
                    phoneBook.mContentResolver, uri, true);
            Log.d(TAG, "fetchBitmap, uri: " + uri);

            Options options = new Options();
            options.inPreferQualityOverSpeed = true;
            options.inScaled = false;
            Rect nullPadding = null;
            Bitmap photo = BitmapFactory.decodeStream(photoDataStream, nullPadding, options);
            if (photo != null) {
                photo.setDensity(Bitmap.DENSITY_NONE);
            }
            Log.d(TAG, "bitmap fetched: " + photo);
            return photo;
        }

        @Override
        protected Bitmap doInBackground(Void... params) {
            return fetchBitmap(mContactId);
        }

        @Override
        protected void onPostExecute(Bitmap bitmap) {
            PhoneBook phoneBook = mPhoneBookRef.get();
            if (phoneBook != null) {
                phoneBook.cacheContactPhoto(mContactId, bitmap);
            }
            mListener.onPhotoLoaded(0, bitmap);
        }
    }

    private static class LoadCompleteListener implements OnLoadCompleteListener<Cursor> {
        private final String mNumber;
        private final ContactLoadedListener mContactLoadedListener;
        private final WeakReference<PhoneBook> mPhoneBookRef;

        private LoadCompleteListener(PhoneBook phoneBook, String number,
                ContactLoadedListener contactLoadedListener) {
            mPhoneBookRef = new WeakReference<>(phoneBook);
            mNumber = number;
            mContactLoadedListener = contactLoadedListener;
        }

        @Override
        public void onLoadComplete(Loader<Cursor> loader, Cursor cursor) {
            Log.d(TAG, "onLoadComplete, cursor: " + cursor);
            PhoneBook phoneBook = mPhoneBookRef.get();
            Contact contact = null;
            if (cursor != null && phoneBook != null) {
                try {
                    if (cursor.moveToFirst()) {
                        int id = cursor.getInt(cursor.getColumnIndex(PhoneLookup._ID));
                        String name = cursor
                                .getString(cursor.getColumnIndex(PhoneLookup.DISPLAY_NAME));
                        String label = cursor.getString(cursor.getColumnIndex(PhoneLookup.LABEL));
                        int type = cursor.getInt(cursor.getColumnIndex(PhoneLookup.TYPE));
                        Resources resources = phoneBook.mContext.getResources();
                        contact = new Contact(resources, mNumber, id, name, label, type);
                    }
                } finally {
                    cursor.close();
                }

                if (contact != null) {
                    synchronized (phoneBook.mSyncContact) {
                        phoneBook.mContactByNumber.put(mNumber, contact);
                    }
                }
            }

            mContactLoadedListener.onContactLoaded(mNumber, contact);
        }
    }
}
