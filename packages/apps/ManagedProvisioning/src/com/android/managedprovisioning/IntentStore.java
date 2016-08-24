/*
 * Copyright 2014, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.managedprovisioning;

import android.accounts.Account;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.PersistableBundle;
import android.util.Xml;

import com.android.internal.util.FastXmlSerializer;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Set;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

/**
 * This class is used to store an intent to an xml file, and then restore it.
 * Can only store:
 * - the action
 * - the component name
 * - extras of type string, Integer, Long, Boolean, ComponentName, PersistableBundle, Account
 */
public class IntentStore {

    private static final String TAG_INTENT_STORE = "intent-store";

    private static final String TAG_ACTION = "action";
    private static final String TAG_COMPONENT_NAME = "component-name";

    private static final String TAG_EXTRAS = "extras";
    private static final String TAG_STRING = "string";
    private static final String TAG_INTEGER = "integer";
    private static final String TAG_LONG = "long";
    private static final String TAG_BOOLEAN = "boolean";
    private static final String TAG_ACCOUNT = "account";
    private static final String TAG_PERSISTABLE_BUNDLE = "persistable-bundle";

    private static final String ATTR_VALUE = "value";
    private static final String ATTR_NAME = "name";
    private static final String ATTR_ACCOUNT_NAME = "account-name";
    private static final String ATTR_ACCOUNT_TYPE = "account-type";

    private final Context mContext;
    private final String mName;

    private static final Object STATIC_LOCK = new Object();

    public IntentStore(Context context, String name) {
        mName = name;
        mContext = context;
    }

    public boolean save(Intent intent) {
        synchronized(STATIC_LOCK) {
            File file = getFile();
            if (file.exists()) {
                ProvisionLogger.loge("Cannot save to the intent store because it already contains"
                        + " an intent");
                return false;
            }
            try (FileOutputStream stream = new FileOutputStream(file)){
                XmlSerializer serializer = new FastXmlSerializer();
                serializer.setOutput(stream, StandardCharsets.UTF_8.name());
                serializer.startDocument(null, true);
                serializer.startTag(null, TAG_INTENT_STORE);
                writeIntent(intent, serializer);
                serializer.endTag(null, TAG_INTENT_STORE);
                serializer.endDocument();
                return true;
            } catch (IOException | XmlPullParserException e) {
                ProvisionLogger.loge("Caught exception while trying to save an intent to the"
                        + " intentStore", e);
                file.delete();
                return false;
            }
        }
    }

    private void writeIntent(Intent intent, XmlSerializer serializer)
            throws IOException, XmlPullParserException {
        if (intent.getAction() != null) {
            writeTag(serializer, TAG_ACTION, intent.getAction());
        }
        if (intent.getComponent() != null) {
            writeTag(serializer, TAG_COMPONENT_NAME,
                    intent.getComponent().flattenToString());
        }
        if (intent.getExtras() != null) {
            writeExtras(intent, serializer);
        }
    }

    private void writeExtras(Intent intent, XmlSerializer serializer)
            throws IOException, XmlPullParserException {
        serializer.startTag(null, TAG_EXTRAS);
        for (String key : intent.getExtras().keySet()) {
            Object o = intent.getExtra(key);
            if (o instanceof String) {
                writeTag(serializer, TAG_STRING, key, (String) o);
            } else if (o instanceof Integer) {
                writeTag(serializer, TAG_INTEGER, key, o.toString());
            } else if (o instanceof Long) {
                writeTag(serializer, TAG_LONG, key, o.toString());
            } else if (o instanceof Boolean) {
                writeTag(serializer, TAG_BOOLEAN, key, o.toString());
            } else if (o instanceof ComponentName) {
                writeTag(serializer, TAG_COMPONENT_NAME, key,
                        ((ComponentName) o).flattenToString());
            } else if (o instanceof Account) {
                Account account = (Account) o;
                serializer.startTag(null, TAG_ACCOUNT);
                serializer.attribute(null, ATTR_NAME, key);
                serializer.attribute(null, ATTR_ACCOUNT_NAME, account.name);
                serializer.attribute(null, ATTR_ACCOUNT_TYPE, account.type);
                serializer.endTag(null, TAG_ACCOUNT);
            } else if (o instanceof PersistableBundle) {
                serializer.startTag(null, TAG_PERSISTABLE_BUNDLE);
                serializer.attribute(null, ATTR_NAME, key);
                ((PersistableBundle) o).saveToXml(serializer);
                serializer.endTag(null, TAG_PERSISTABLE_BUNDLE);
            } else if (o != null) {
                ProvisionLogger.loge("extra for key " + key + " cannot be save in the intent"
                        + " store: " + o, new RuntimeException());
            }
        }
        serializer.endTag(null, TAG_EXTRAS);
    }

    public Intent load() {
        File file = getFile();
        if (!file.exists()) {
            return null;
        }
        synchronized(STATIC_LOCK) {
            try (FileInputStream stream = new FileInputStream(file)) {
                XmlPullParser parser = Xml.newPullParser();
                parser.setInput(stream, null);
                Intent result = parseIntent(parser);
                return result;
            } catch (IOException | XmlPullParserException e) {
                ProvisionLogger.loge("Caught exception while trying to load an intent from the"
                        + " IntentStore", e);
                return null;
            }
        }
    }

    private Intent parseIntent(XmlPullParser parser) throws XmlPullParserException, IOException {
        Intent intent = new Intent();
        int outerDepth = parser.getDepth();
        int type;
        while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
               && (type != XmlPullParser.END_TAG || parser.getDepth() > outerDepth)) {
            if (type == XmlPullParser.END_TAG || type == XmlPullParser.TEXT) {
                continue;
            }
            String tag = parser.getName();
            if (tag.equals(TAG_ACTION)) {
                intent.setAction(parser.getAttributeValue(null, ATTR_VALUE));
            } else if (tag.equals(TAG_COMPONENT_NAME)) {
                intent.setComponent(ComponentName.unflattenFromString(
                        parser.getAttributeValue(null, ATTR_VALUE)));
            } else if (tag.equals(TAG_EXTRAS)) {
                parseExtras(intent, parser);
            }
        }
        return intent;
    }

    private void parseExtras(Intent intent, XmlPullParser parser) throws XmlPullParserException,
            IOException {
        int outerDepth = parser.getDepth();
        int type;
        while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
               && (type != XmlPullParser.END_TAG || parser.getDepth() > outerDepth)) {
            if (type == XmlPullParser.END_TAG || type == XmlPullParser.TEXT) {
                continue;
            }
            String tag = parser.getName();
            switch (tag) {
                case TAG_STRING:
                    intent.putExtra(parser.getAttributeValue(null, ATTR_NAME),
                            parser.getAttributeValue(null, ATTR_VALUE));
                    break;
                case TAG_INTEGER:
                    intent.putExtra(parser.getAttributeValue(null, ATTR_NAME),
                            Integer.parseInt(parser.getAttributeValue(null, ATTR_VALUE)));
                    break;
                case TAG_LONG:
                    intent.putExtra(parser.getAttributeValue(null, ATTR_NAME),
                            Long.parseLong(parser.getAttributeValue(null, ATTR_VALUE)));
                    break;
                case TAG_BOOLEAN:
                    intent.putExtra(parser.getAttributeValue(null, ATTR_NAME),
                            Boolean.parseBoolean(
                            parser.getAttributeValue(null, ATTR_VALUE)));
                    break;
                case TAG_COMPONENT_NAME:
                    intent.putExtra(parser.getAttributeValue(null, ATTR_NAME),
                            ComponentName.unflattenFromString(
                            parser.getAttributeValue(null, ATTR_VALUE)));
                    break;
                case TAG_ACCOUNT:
                    Account a = new Account(
                            parser.getAttributeValue(null, ATTR_ACCOUNT_NAME),
                            parser.getAttributeValue(null, ATTR_ACCOUNT_TYPE));
                    intent.putExtra(parser.getAttributeValue(null, ATTR_NAME), a);
                    break;
                case TAG_PERSISTABLE_BUNDLE:
                    intent.putExtra(parser.getAttributeValue(null, ATTR_NAME),
                            PersistableBundle.restoreFromXml(parser));
                    break;
            }
        }
    }

    public void clear() {
        getFile().delete();
    }

    private void writeTag(XmlSerializer serializer, String tag, String value) throws IOException {
        serializer.startTag(null, tag);
        serializer.attribute(null, ATTR_VALUE, value);
        serializer.endTag(null, tag);
    }

    private void writeTag(XmlSerializer serializer, String tag, String name, String value)
            throws IOException {
        serializer.startTag(null, tag);
        serializer.attribute(null, ATTR_NAME, name);
        serializer.attribute(null, ATTR_VALUE, value);
        serializer.endTag(null, tag);
    }

    private File getFile() {
        return new File(mContext.getFilesDir(), "intent_store_" + mName + ".xml");
    }
}
