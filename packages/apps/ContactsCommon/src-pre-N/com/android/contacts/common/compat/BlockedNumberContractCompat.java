package com.android.contacts.common.compat;

import android.content.Context;

public class BlockedNumberContractCompat {
    public static boolean canCurrentUserBlockNumbers(Context context) {
        return false;
    }
}