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

package com.android.tv.search;

import android.app.SearchManager;
import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;

import com.android.tv.util.PermissionUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class LocalSearchProvider extends ContentProvider {
    private static final boolean DEBUG = false;
    private static final String TAG = "LocalSearchProvider";

    public static final int PROGRESS_PERCENTAGE_HIDE = -1;

    // TODO: Remove this once added to the SearchManager.
    private static final String SUGGEST_COLUMN_PROGRESS_BAR_PERCENTAGE = "progress_bar_percentage";

    private static final String[] SEARCHABLE_COLUMNS = new String[] {
        SearchManager.SUGGEST_COLUMN_TEXT_1,
        SearchManager.SUGGEST_COLUMN_TEXT_2,
        SearchManager.SUGGEST_COLUMN_RESULT_CARD_IMAGE,
        SearchManager.SUGGEST_COLUMN_INTENT_ACTION,
        SearchManager.SUGGEST_COLUMN_INTENT_DATA,
        SearchManager.SUGGEST_COLUMN_CONTENT_TYPE,
        SearchManager.SUGGEST_COLUMN_IS_LIVE,
        SearchManager.SUGGEST_COLUMN_VIDEO_WIDTH,
        SearchManager.SUGGEST_COLUMN_VIDEO_HEIGHT,
        SearchManager.SUGGEST_COLUMN_DURATION,
        SUGGEST_COLUMN_PROGRESS_BAR_PERCENTAGE
    };

    private static final String EXPECTED_PATH_PREFIX = "/" + SearchManager.SUGGEST_URI_PATH_QUERY;
    // The launcher passes 10 as a 'limit' parameter by default.
    private static final int DEFAULT_SEARCH_LIMIT = 10;

    private static final String NO_LIVE_CONTENTS = "0";
    private static final String LIVE_CONTENTS = "1";

    static final String SUGGEST_PARAMETER_ACTION = "action";
    static final int DEFAULT_SEARCH_ACTION = SearchInterface.ACTION_TYPE_AMBIGUOUS;

    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
            String sortOrder) {
        if (DEBUG) {
            Log.d(TAG, "query(" + uri + ", " + Arrays.toString(projection) + ", " + selection + ", "
                    + Arrays.toString(selectionArgs) + ", " + sortOrder + ")");
        }
        SearchInterface search;
        if (PermissionUtils.hasAccessAllEpg(getContext())) {
            search = new TvProviderSearch(getContext());
        } else {
            search = new DataManagerSearch(getContext());
        }
        String query = uri.getLastPathSegment();
        int limit = DEFAULT_SEARCH_LIMIT;
        int action = DEFAULT_SEARCH_ACTION;
        try {
            limit = Integer.parseInt(uri.getQueryParameter(SearchManager.SUGGEST_PARAMETER_LIMIT));
            action = Integer.parseInt(uri.getQueryParameter(SUGGEST_PARAMETER_ACTION));
        } catch (NumberFormatException | UnsupportedOperationException e) {
            // Ignore the exceptions
        }
        List<SearchResult> results = new ArrayList<>();
        if (!TextUtils.isEmpty(query)) {
            results.addAll(search.search(query, limit, action));
        }
        return createSuggestionsCursor(results);
    }

    private Cursor createSuggestionsCursor(List<SearchResult> results) {
        MatrixCursor cursor = new MatrixCursor(SEARCHABLE_COLUMNS, results.size());
        List<String> row = new ArrayList<>(SEARCHABLE_COLUMNS.length);

        for (SearchResult result : results) {
            row.clear();
            row.add(result.title);
            row.add(result.description);
            row.add(result.imageUri);
            row.add(result.intentAction);
            row.add(result.intentData);
            row.add(result.contentType);
            row.add(result.isLive ? LIVE_CONTENTS : NO_LIVE_CONTENTS);
            row.add(result.videoWidth == 0 ? null : String.valueOf(result.videoWidth));
            row.add(result.videoHeight == 0 ? null : String.valueOf(result.videoHeight));
            row.add(result.duration == 0 ? null : String.valueOf(result.duration));
            row.add(String.valueOf(result.progressPercentage));
            cursor.addRow(row);
        }
        return cursor;
    }

    @Override
    public String getType(Uri uri) {
        if (!checkUriCorrect(uri)) return null;
        return SearchManager.SUGGEST_MIME_TYPE;
    }

    private static boolean checkUriCorrect(Uri uri) {
        return uri != null && uri.getPath().startsWith(EXPECTED_PATH_PREFIX);
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException();
    }

    /**
     * A placeholder to a search result.
     */
    public static class SearchResult {
        public long channelId;
        public String channelNumber;
        public String title;
        public String description;
        public String imageUri;
        public String intentAction;
        public String intentData;
        public String contentType;
        public boolean isLive;
        public int videoWidth;
        public int videoHeight;
        public long duration;
        public int progressPercentage;

        @Override
        public String toString() {
            return "channelId: " + channelId +
                    ", channelNumber: " + channelNumber +
                    ", title: " + title;
        }
    }
}