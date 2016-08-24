package com.android.tv.util;

import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;

/**
 * Util class to handle permissions.
 */
public class PermissionUtils {
    private static Boolean sHasAccessAllEpgPermission;
    private static Boolean sHasAccessWatchedHistoryPermission;
    private static Boolean sHasModifyParentalControlsPermission;

    public static boolean hasAccessAllEpg(Context context) {
        if (sHasAccessAllEpgPermission == null) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                sHasAccessAllEpgPermission = context.checkSelfPermission(
                        "com.android.providers.tv.permission.ACCESS_ALL_EPG_DATA")
                        == PackageManager.PERMISSION_GRANTED;
            } else {
                sHasAccessAllEpgPermission = context.getPackageManager().checkPermission(
                        "com.android.providers.tv.permission.ACCESS_ALL_EPG_DATA",
                        context.getPackageName()) == PackageManager.PERMISSION_GRANTED;
            }
        }
        return sHasAccessAllEpgPermission;
    }

    public static boolean hasAccessWatchedHistory(Context context) {
        if (sHasAccessWatchedHistoryPermission == null) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                sHasAccessWatchedHistoryPermission = context.checkSelfPermission(
                        "com.android.providers.tv.permission.ACCESS_WATCHED_PROGRAMS")
                        == PackageManager.PERMISSION_GRANTED;
            } else {
                sHasAccessWatchedHistoryPermission = context.getPackageManager().checkPermission(
                        "com.android.providers.tv.permission.ACCESS_WATCHED_PROGRAMS",
                        context.getPackageName()) == PackageManager.PERMISSION_GRANTED;
            }
        }
        return sHasAccessWatchedHistoryPermission;
    }

    public static boolean hasModifyParentalControls(Context context) {
        if (sHasModifyParentalControlsPermission == null) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                sHasModifyParentalControlsPermission = context.checkSelfPermission(
                        "android.permission.MODIFY_PARENTAL_CONTROLS")
                        == PackageManager.PERMISSION_GRANTED;
            } else {
                sHasModifyParentalControlsPermission = context.getPackageManager().checkPermission(
                        "android.permission.MODIFY_PARENTAL_CONTROLS",
                        context.getPackageName()) == PackageManager.PERMISSION_GRANTED;
            }
        }
        return sHasModifyParentalControlsPermission;
    }

    public static boolean hasReadTvListings(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return context.checkSelfPermission("android.permission.READ_TV_LISTINGS")
                    == PackageManager.PERMISSION_GRANTED;
        } else {
            return context.getPackageManager().checkPermission(
                    "android.permission.MODIFY_PARENTAL_CONTROLS",
                    context.getPackageName()) == PackageManager.PERMISSION_GRANTED;
        }
    }
}
