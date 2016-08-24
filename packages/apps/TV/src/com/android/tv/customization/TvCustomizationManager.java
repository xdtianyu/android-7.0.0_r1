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

package com.android.tv.customization;

import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.util.Log;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class TvCustomizationManager {
    private static final String TAG = "TvCustomizationManager";
    private static final boolean DEBUG = false;

    private static final String CATEGORY_TV_CUSTOMIZATION =
            "com.android.tv.category";

    /**
     * Row IDs to share customized actions.
     * Only rows listed below can have customized action.
     */
    public static final String ID_OPTIONS_ROW = "options_row";
    public static final String ID_PARTNER_ROW = "partner_row";

    private static final HashMap<String, String> INTENT_CATEGORY_TO_ROW_ID;
    static {
        INTENT_CATEGORY_TO_ROW_ID = new HashMap<>();
        INTENT_CATEGORY_TO_ROW_ID.put(CATEGORY_TV_CUSTOMIZATION + ".OPTIONS_ROW", ID_OPTIONS_ROW);
        INTENT_CATEGORY_TO_ROW_ID.put(CATEGORY_TV_CUSTOMIZATION + ".PARTNER_ROW", ID_PARTNER_ROW);
    }

    private static final String RES_ID_PARTNER_ROW_TITLE = "partner_row_title";

    private static final String RES_TYPE_STRING = "string";

    private final Context mContext;
    private boolean mInitialized;
    private String mCustomizationPackage;

    private String mPartnerRowTitle;
    private final Map<String, List<CustomAction>> mRowIdToCustomActionsMap = new HashMap<>();

    public TvCustomizationManager(Context context) {
        mContext = context;
        mInitialized = false;
    }

    /**
     * Initialize TV customization options.
     * Run this API only on the main thread.
     */
    public void initialize() {
        if (mInitialized) {
            return;
        }
        mInitialized = true;
        buildCustomActions();
        if (!TextUtils.isEmpty(mCustomizationPackage)) {
            buildPartnerRow();
        }
    }

    private void buildCustomActions() {
        mCustomizationPackage = null;
        mRowIdToCustomActionsMap.clear();
        PackageManager pm = mContext.getPackageManager();
        for (String intentCategory : INTENT_CATEGORY_TO_ROW_ID.keySet()) {
            Intent customOptionIntent = new Intent(Intent.ACTION_MAIN);
            customOptionIntent.addCategory(intentCategory);

            List<ResolveInfo> activities = pm.queryIntentActivities(customOptionIntent,
                    PackageManager.GET_RECEIVERS | PackageManager.GET_RESOLVED_FILTER
                            | PackageManager.GET_META_DATA);
            for (ResolveInfo info : activities) {
                String packageName = info.activityInfo.packageName;
                if (TextUtils.isEmpty(mCustomizationPackage)) {
                    if (DEBUG) Log.d(TAG, "Found TV customization package " + packageName);
                    if ((info.activityInfo.applicationInfo.flags
                            & ApplicationInfo.FLAG_SYSTEM) == 0) {
                        Log.w(TAG, "Only system app can customize TV. Ignoring " + packageName);
                        continue;
                    }
                    mCustomizationPackage = packageName;
                } else if (!packageName.equals(mCustomizationPackage)) {
                    Log.w(TAG, "A customization package " + mCustomizationPackage
                            + " already exist. Ignoring " + packageName);
                    continue;
                }

                int position = info.filter.getPriority();
                String title = info.loadLabel(pm).toString();
                Drawable drawable = info.loadIcon(pm);
                Intent intent = new Intent(Intent.ACTION_MAIN);
                intent.addCategory(intentCategory);
                intent.setClassName(mCustomizationPackage, info.activityInfo.name);

                String rowId = INTENT_CATEGORY_TO_ROW_ID.get(intentCategory);
                List<CustomAction> actions = mRowIdToCustomActionsMap.get(rowId);
                if (actions == null) {
                    actions = new ArrayList<>();
                    mRowIdToCustomActionsMap.put(rowId, actions);
                }
                actions.add(new CustomAction(position, title, drawable, intent));
            }
        }
        // Sort items by position
        for (List<CustomAction> actions : mRowIdToCustomActionsMap.values()) {
            Collections.sort(actions);
        }

        if (DEBUG) {
            Log.d(TAG, "Dumping custom actions");
            for (String id : mRowIdToCustomActionsMap.keySet()) {
                for (CustomAction action : mRowIdToCustomActionsMap.get(id)) {
                    Log.d(TAG, "Custom row rowId=" + id + " title=" + action.getTitle()
                        + " class=" + action.getIntent());
                }
            }
            Log.d(TAG, "Dumping custom actions - end of dump");
        }
    }

    /**
     * Returns custom actions for given row id.
     *
     * Row ID is one of ID_OPTIONS_ROW or ID_PARTNER_ROW.
     */
    public List<CustomAction> getCustomActions(String rowId) {
        return mRowIdToCustomActionsMap.get(rowId);
    }

    private void buildPartnerRow() {
        mPartnerRowTitle = null;
        Resources res;
        try {
            res = mContext.getPackageManager()
                    .getResourcesForApplication(mCustomizationPackage);
        } catch (NameNotFoundException e) {
            Log.w(TAG, "Could not get resources for package " + mCustomizationPackage);
            return;
        }
        int resId = res.getIdentifier(
                RES_ID_PARTNER_ROW_TITLE, RES_TYPE_STRING, mCustomizationPackage);
        if (resId != 0) {
            mPartnerRowTitle = res.getString(resId);
        }
        if (DEBUG) Log.d(TAG, "Partner row title [" + mPartnerRowTitle + "]");
    }

    /**
     * Returns partner row title.
     */
    public String getPartnerRowTitle() {
        return mPartnerRowTitle;
    }
}
