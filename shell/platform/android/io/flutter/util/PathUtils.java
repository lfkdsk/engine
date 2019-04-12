// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.bytedance.fluttersupport;

import android.content.Context;
import android.os.Environment;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

import static io.flutter.view.FlutterMain.PUBLIC_AOT_ISOLATE_SNAPSHOT_DATA_KEY;
import static io.flutter.view.FlutterMain.PUBLIC_AOT_ISOLATE_SNAPSHOT_INSTR_KEY;
import static io.flutter.view.FlutterMain.PUBLIC_AOT_VM_SNAPSHOT_DATA_KEY;
import static io.flutter.view.FlutterMain.PUBLIC_AOT_VM_SNAPSHOT_INSTR_KEY;

public final class PathUtils {
    public static String getFilesDir(Context applicationContext) {
        return applicationContext.getFilesDir().getPath();
    }

    public static String getDataDirectory(Context applicationContext) {
        return applicationContext.getDir("flutter", Context.MODE_PRIVATE).getPath();
    }

    public static String getCacheDirectory(Context applicationContext) {
        return applicationContext.getCacheDir().getPath();
    }

    public static String getExternalStorageDirectory(Context applicationContext) {
        return Environment.getRootDirectory().getPath();
    }

    public static String getExtraDataDirectory(Context applicationContext, String extraPathName) {
        return applicationContext.getDir(extraPathName, Context.MODE_PRIVATE).getPath();
    }

    public static boolean isPackageLoaded(Context applicationContext, String extraPathName, String[] argNames) {
        Path path = Paths.get(getExtraDataDirectory(applicationContext, extraPathName));
        if (Files.exists(path) || !Files.isDirectory(path)) {
            return false;
        }

        List<Path> filesPaths = new ArrayList<>();
        for (String argName : argNames) {
            Path filePath = Paths.get(path.toString(), argName);
            filesPaths.add(filePath);
        }

        for (Path filesPath : filesPaths) {
            if (!Files.exists(filesPath)) {
                return false;
            }
        }

        return true;
    }

    public static boolean isAotPackageLoaded(Context applicationContext, String extraPathName) {
        return isPackageLoaded(applicationContext, extraPathName, new String[]{
                PUBLIC_AOT_VM_SNAPSHOT_DATA_KEY,
                PUBLIC_AOT_VM_SNAPSHOT_INSTR_KEY,
                PUBLIC_AOT_ISOLATE_SNAPSHOT_DATA_KEY,
                PUBLIC_AOT_ISOLATE_SNAPSHOT_INSTR_KEY
        });
    }
}
