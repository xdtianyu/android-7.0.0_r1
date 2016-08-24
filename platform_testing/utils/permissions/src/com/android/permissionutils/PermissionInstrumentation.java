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

package com.android.permissionutils;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PermissionInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.os.UserHandle;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

/**
 * A utility to dump or grant all revoked runtime permissions
 */
public class PermissionInstrumentation extends Instrumentation {

    private static final String DUMP_TAG = "DUMP";
    private static final String PARAM_COMMAND = "command";
    private static final String COMMAND_DUMP = "dump";
    private static final String COMMAND_GRANTALL = "grant-all";

    private static enum Action {DUMP, GRANTALL};

    @Override
    public void onCreate(Bundle arguments) {
        super.onCreate(arguments);
        String command = arguments.getString(PARAM_COMMAND);
        if (command == null) {
            throw new IllegalArgumentException("missing command parameter");
        }
        if (COMMAND_DUMP.equals(command)) {
            runCommand(Action.DUMP);
        } else if (COMMAND_GRANTALL.equals(command)) {
            runCommand(Action.GRANTALL);
        } else {
            throw new IllegalArgumentException(
                    String.format("unrecognized command \"%s\"", command));
        }
        finish(Activity.RESULT_OK, new Bundle());
    }

    private void runCommand(Action action) {
        PackageManager pm = getContext().getPackageManager();
        List<PackageInfo> pkgInfos = pm.getInstalledPackages(PackageManager.GET_PERMISSIONS);
        List<String> permissions = new ArrayList<>();
        for (PackageInfo info : pkgInfos) {
            if (info.requestedPermissions == null) {
                continue;
            }
            for (String permission : info.requestedPermissions) {
                PermissionInfo pi = null;
                try {
                    pi = pm.getPermissionInfo(permission, 0);
                } catch (NameNotFoundException nnfe) {
                    // ignore
                }
                if (pi == null) {
                    continue;
                }
                if (!isRuntime(pi)) {
                    continue;
                }
                int flag = pm.checkPermission(permission, info.packageName);
                if (flag == PackageManager.PERMISSION_DENIED) {
                    if (action == Action.DUMP) {
                        permissions.add(permission);
                    } else if (action == Action.GRANTALL) {
                        pm.grantRuntimePermission(info.packageName, permission, UserHandle.OWNER);
                    }
                }
            }
            if (action == Action.DUMP && !permissions.isEmpty()) {
                Log.e(DUMP_TAG, String.format("Revoked permissions for %s", info.packageName));
                for (String permission : permissions) {
                    Log.e(DUMP_TAG, "    " + permission);
                }
                permissions.clear();
            }
        }
    }

    private boolean isRuntime(PermissionInfo pi) {
        return (pi.protectionLevel & PermissionInfo.PROTECTION_MASK_BASE)
                == PermissionInfo.PROTECTION_DANGEROUS;
    }
}
