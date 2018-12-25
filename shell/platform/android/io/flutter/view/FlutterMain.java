// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package io.flutter.view;

import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.os.Looper;
import android.os.SystemClock;
import android.util.Log;
import io.flutter.util.PathUtils;

import java.io.File;
import java.io.IOException;
import java.util.*;

/**
 * A class to intialize the Flutter engine.
 */
public class FlutterMain {
    private static String TAG = "FlutterMain";

    // Must match values in sky::shell::switches
    private static String AOT_SHARED_LIBRARY_PATH = "aot-shared-library-path";
    private static String AOT_SNAPSHOT_PATH_KEY = "aot-snapshot-path";
    private static String AOT_VM_SNAPSHOT_DATA_KEY = "vm-snapshot-data";
    private static String AOT_VM_SNAPSHOT_INSTR_KEY = "vm-snapshot-instr";
    private static String AOT_ISOLATE_SNAPSHOT_DATA_KEY = "isolate-snapshot-data";
    private static String AOT_ISOLATE_SNAPSHOT_INSTR_KEY = "isolate-snapshot-instr";
    private static String FLX_KEY = "flx";
    private static String FLUTTER_ASSETS_DIR_KEY = "flutter-assets-dir";

    // XML Attribute keys supported in AndroidManifest.xml
    public static String PUBLIC_AOT_AOT_SHARED_LIBRARY_PATH =
            FlutterMain.class.getName() + '.' + AOT_SHARED_LIBRARY_PATH;
    public static String PUBLIC_AOT_VM_SNAPSHOT_DATA_KEY =
            FlutterMain.class.getName() + '.' + AOT_VM_SNAPSHOT_DATA_KEY;
    public static String PUBLIC_AOT_VM_SNAPSHOT_INSTR_KEY =
            FlutterMain.class.getName() + '.' + AOT_VM_SNAPSHOT_INSTR_KEY;
    public static String PUBLIC_AOT_ISOLATE_SNAPSHOT_DATA_KEY =
            FlutterMain.class.getName() + '.' + AOT_ISOLATE_SNAPSHOT_DATA_KEY;
    public static String PUBLIC_AOT_ISOLATE_SNAPSHOT_INSTR_KEY =
            FlutterMain.class.getName() + '.' + AOT_ISOLATE_SNAPSHOT_INSTR_KEY;
    public static String PUBLIC_FLX_KEY =
            FlutterMain.class.getName() + '.' + FLX_KEY;
    public static String PUBLIC_FLUTTER_ASSETS_DIR_KEY =
            FlutterMain.class.getName() + '.' + FLUTTER_ASSETS_DIR_KEY;

    // Resource names used for components of the precompiled snapshot.
    public static String DEFAULT_AOT_SHARED_LIBRARY_PATH = "app.so";
    public static String DEFAULT_AOT_VM_SNAPSHOT_DATA = "vm_snapshot_data";
    public static String DEFAULT_AOT_VM_SNAPSHOT_INSTR = "vm_snapshot_instr";
    public static String DEFAULT_AOT_ISOLATE_SNAPSHOT_DATA = "isolate_snapshot_data";
    public static String DEFAULT_AOT_ISOLATE_SNAPSHOT_INSTR = "isolate_snapshot_instr";
    public static String DEFAULT_FLX = "app.flx";
    public static String DEFAULT_KERNEL_BLOB = "kernel_blob.bin";
    public static String DEFAULT_FLUTTER_ASSETS_DIR = "flutter_assets";

    // Assets that are shared among all Flutter apps within an APK.
    public static String SHARED_ASSET_DIR = "flutter_shared";
    public static String SHARED_ASSET_ICU_DATA = "icudtl.dat";

    private static String fromFlutterAssets(String filePath) {
        return sFlutterAssetsDir + File.separator + filePath;
    }

    // Mutable because default values can be overridden via config properties
    public static String sAotSharedLibraryPath = DEFAULT_AOT_SHARED_LIBRARY_PATH;
    public static String sAotVmSnapshotData = DEFAULT_AOT_VM_SNAPSHOT_DATA;
    public static String sAotVmSnapshotInstr = DEFAULT_AOT_VM_SNAPSHOT_INSTR;
    public static String sAotIsolateSnapshotData = DEFAULT_AOT_ISOLATE_SNAPSHOT_DATA;
    public static String sAotIsolateSnapshotInstr = DEFAULT_AOT_ISOLATE_SNAPSHOT_INSTR;
    public static String sFlx = DEFAULT_FLX;
    public static String sFlutterAssetsDir = DEFAULT_FLUTTER_ASSETS_DIR;

    private static boolean sInitialized = false;
    // private static ResourceUpdater sResourceUpdater;
    private static ResourceExtractor sResourceExtractor;
    private static boolean sIsPrecompiledAsBlobs;
    private static boolean sIsPrecompiledAsSharedLibrary;
    private static Settings sSettings;
    private static String sIcuDataPath;

    private static final class ImmutableSetBuilder<T> {
        static <T> ImmutableSetBuilder<T> newInstance() {
            return new ImmutableSetBuilder<>();
        }

        HashSet<T> set = new HashSet<>();

        private ImmutableSetBuilder() {
        }

        ImmutableSetBuilder<T> add(T element) {
            set.add(element);
            return this;
        }

        @SafeVarargs
        ImmutableSetBuilder<T> add(T... elements) {
            for (T element : elements) {
                set.add(element);
            }
            return this;
        }

        Set<T> build() {
            return Collections.unmodifiableSet(set);
        }
    }

    public static class Settings {
        private String logTag;
        private String sAotSharedLibraryPath = DEFAULT_AOT_SHARED_LIBRARY_PATH;
        private String sAotVmSnapshotData = DEFAULT_AOT_VM_SNAPSHOT_DATA;
        private String sAotVmSnapshotInstr = DEFAULT_AOT_VM_SNAPSHOT_INSTR;
        private String sAotIsolateSnapshotData = DEFAULT_AOT_ISOLATE_SNAPSHOT_DATA;
        private String sAotIsolateSnapshotInstr = DEFAULT_AOT_ISOLATE_SNAPSHOT_INSTR;
        private String sFlx = DEFAULT_FLX;
        private String sFlutterAssetsDir = DEFAULT_FLUTTER_ASSETS_DIR;


        public String getLogTag() {
            return logTag;
        }

        /**
         * Set the tag associated with Flutter app log messages.
         *
         * @param tag Log tag.
         */
        public void setLogTag(String tag) {
            logTag = tag;
        }

        public static class Builder {
            private Settings mSettings = new Settings();

            public Builder setAotSharedLibrayPath(String sAotSharedLibraryPath) {
                mSettings.sAotSharedLibraryPath = sAotSharedLibraryPath;
                return this;
            }

            public Builder setAotVmSnapshotData(String sAotVmSnapshotData) {
                mSettings.sAotVmSnapshotData = sAotVmSnapshotData;
                return this;
            }

            public Builder setAotVmSnapshotInstr(String sAotVmSnapshotInstr) {
                mSettings.sAotVmSnapshotInstr = sAotVmSnapshotInstr;
                return this;
            }

            public Builder setAotIsolateSnapshotData(String sAotIsolateSnapshotData) {
                mSettings.sAotIsolateSnapshotData = sAotIsolateSnapshotData;
                return this;
            }

            public Builder setAotIsolateSnapshotInstr(String sAotIsolateSnapshotInstr) {
                mSettings.sAotIsolateSnapshotInstr = sAotIsolateSnapshotInstr;
                return this;
            }

            public Builder setFlx(String sFlx) {
                mSettings.sFlx = sFlx;
                return this;
            }

            public Builder setFlutterAssetsDir(String sFlutterAssetsDir) {
                mSettings.sFlutterAssetsDir = sFlutterAssetsDir;
                return this;
            }

            public Settings build() {
                return mSettings;
            }
        }

        public static Settings.Builder create() {
            return new Settings.Builder();
        }
    }

    /**
     * Starts initialization of the native system.
     *
     * @param applicationContext The Android application context.
     */
    public static void startInitialization(Context applicationContext) {
        startInitialization(applicationContext, new Settings());
    }

    /**
     * Starts initialization of the native system.
     *
     * @param applicationContext The Android application context.
     * @param settings           Configuration settings.
     */
    public static void startInitialization(Context applicationContext, Settings settings) {
        if (Looper.myLooper() != Looper.getMainLooper()) {
            throw new IllegalStateException("startInitialization must be called on the main thread");
        }
        // Do not run startInitialization more than once.
        if (sSettings != null) {
            return;
        }

        sSettings = settings;

        long initStartTimestampMillis = SystemClock.uptimeMillis();
        initConfig(applicationContext);
        initAot(applicationContext);
        initResources(applicationContext);
        System.loadLibrary("flutter");

        // We record the initialization time using SystemClock because at the start of the
        // initialization we have not yet loaded the native library to call into dart_tools_api.h.
        // To get Timeline timestamp of the start of initialization we simply subtract the delta
        // from the Timeline timestamp at the current moment (the assumption is that the overhead
        // of the JNI call is negligible).
        long initTimeMillis = SystemClock.uptimeMillis() - initStartTimestampMillis;
        nativeRecordStartTimestamp(initTimeMillis);
    }

    /**
     * Blocks until initialization of the native system has completed.
     *
     * @param applicationContext The Android application context.
     * @param args               Flags sent to the Flutter runtime.
     */
    public static void ensureInitializationComplete(Context applicationContext, String[] args) {
        if (Looper.myLooper() != Looper.getMainLooper()) {
            throw new IllegalStateException("ensureInitializationComplete must be called on the main thread");
        }
        if (sSettings == null) {
            throw new IllegalStateException("ensureInitializationComplete must be called after startInitialization");
        }
        if (sInitialized) {
            return;
        }
        try {
            sResourceExtractor.waitForCompletion();

            List<String> shellArgs = new ArrayList<>();
            shellArgs.add("--icu-data-file-path=" + sIcuDataPath);
            if (args != null) {
                Collections.addAll(shellArgs, args);
            }
            if (sIsPrecompiledAsSharedLibrary) {
                shellArgs.add("--" + AOT_SHARED_LIBRARY_PATH + "=" +
                        new File(PathUtils.getDataDirectory(applicationContext), sAotSharedLibraryPath));
            } else {
                if (sIsPrecompiledAsBlobs) {
                    shellArgs.add("--" + AOT_SNAPSHOT_PATH_KEY + "=" +
                            PathUtils.getDataDirectory(applicationContext));
                } else {
                    shellArgs.add("--cache-dir-path=" +
                            PathUtils.getCacheDirectory(applicationContext));

                    shellArgs.add("--" + AOT_SNAPSHOT_PATH_KEY + "=" +
                            PathUtils.getDataDirectory(applicationContext) + "/" + sFlutterAssetsDir);
                }
                shellArgs.add("--" + AOT_VM_SNAPSHOT_DATA_KEY + "=" + sAotVmSnapshotData);
                shellArgs.add("--" + AOT_VM_SNAPSHOT_INSTR_KEY + "=" + sAotVmSnapshotInstr);
                shellArgs.add("--" + AOT_ISOLATE_SNAPSHOT_DATA_KEY + "=" + sAotIsolateSnapshotData);
                shellArgs.add("--" + AOT_ISOLATE_SNAPSHOT_INSTR_KEY + "=" + sAotIsolateSnapshotInstr);
            }

            if (sSettings.getLogTag() != null) {
                shellArgs.add("--log-tag=" + sSettings.getLogTag());
            }

            String appBundlePath = findAppBundlePath(applicationContext);
            String appBundlePath = findAppBundlePath(applicationContext);
            String appStoragePath = PathUtils.getFilesDir(applicationContext);
            String engineCachesPath = PathUtils.getCacheDirectory(applicationContext);
            nativeInit(applicationContext, shellArgs.toArray(new String[0]),
                    appBundlePath, appStoragePath, engineCachesPath);

            sInitialized = true;
        } catch (Exception e) {
            Log.e(TAG, "Flutter initialization failed.", e);
            throw new RuntimeException(e);
        }
    }

    public static native void nativeInit(Context context, String[] args, String bundlePath, String appStoragePath, String engineCachesPath);

    public static native void nativeRecordStartTimestamp(long initTimeMillis);

    /**
     * Initialize our Flutter config values by obtaining them from the
     * manifest XML file, falling back to default values.
     */
    private static void initConfig(Context applicationContext) {
        try {
            Bundle metadata = applicationContext.getPackageManager().getApplicationInfo(
                    applicationContext.getPackageName(), PackageManager.GET_META_DATA).metaData;
            if (metadata != null) {
                sAotSharedLibraryPath = metadata.getString(PUBLIC_AOT_AOT_SHARED_LIBRARY_PATH, DEFAULT_AOT_SHARED_LIBRARY_PATH);
                sAotVmSnapshotData = metadata.getString(PUBLIC_AOT_VM_SNAPSHOT_DATA_KEY, DEFAULT_AOT_VM_SNAPSHOT_DATA);
                sAotVmSnapshotInstr = metadata.getString(PUBLIC_AOT_VM_SNAPSHOT_INSTR_KEY, DEFAULT_AOT_VM_SNAPSHOT_INSTR);
                sAotIsolateSnapshotData = metadata.getString(PUBLIC_AOT_ISOLATE_SNAPSHOT_DATA_KEY, DEFAULT_AOT_ISOLATE_SNAPSHOT_DATA);
                sAotIsolateSnapshotInstr = metadata.getString(PUBLIC_AOT_ISOLATE_SNAPSHOT_INSTR_KEY, DEFAULT_AOT_ISOLATE_SNAPSHOT_INSTR);
                sFlx = metadata.getString(PUBLIC_FLX_KEY, DEFAULT_FLX);
                sFlutterAssetsDir = metadata.getString(PUBLIC_FLUTTER_ASSETS_DIR_KEY, DEFAULT_FLUTTER_ASSETS_DIR);
            }
        } catch (PackageManager.NameNotFoundException e) {
            throw new RuntimeException(e);
        }
    }

    private static void initResources(Context applicationContext) {
        Context context = applicationContext;
        new ResourceCleaner(context).start();

        Bundle metaData = null;
        try {
            metaData = context.getPackageManager().getApplicationInfo(
                    context.getPackageName(), PackageManager.GET_META_DATA).metaData;

        } catch (PackageManager.NameNotFoundException e) {
            Log.e(TAG, "Unable to read application info", e);
        }

//        if (metaData != null && metaData.getBoolean("DynamicUpdates")) {
//            sResourceUpdater = new ResourceUpdater(context);
//            sResourceUpdater.startUpdateDownloadOnce();
//            sResourceUpdater.waitForDownloadCompletion();
//        }

        sResourceExtractor = new ResourceExtractor(context);

        String icuAssetPath = SHARED_ASSET_DIR + File.separator + SHARED_ASSET_ICU_DATA;
        sResourceExtractor.addResource(icuAssetPath);
        sIcuDataPath = PathUtils.getDataDirectory(applicationContext) + File.separator + icuAssetPath;

        sResourceExtractor
                .addResource(fromFlutterAssets(sFlx))
                .addResource(fromFlutterAssets(sAotVmSnapshotData))
                .addResource(fromFlutterAssets(sAotVmSnapshotInstr))
                .addResource(fromFlutterAssets(sAotIsolateSnapshotData))
                .addResource(fromFlutterAssets(sAotIsolateSnapshotInstr))
                .addResource(fromFlutterAssets(DEFAULT_KERNEL_BLOB));
        if (sIsPrecompiledAsSharedLibrary) {
            sResourceExtractor
                    .addResource(sAotSharedLibraryPath);
        } else {
            sResourceExtractor
                    .addResource(sAotVmSnapshotData)
                    .addResource(sAotVmSnapshotInstr)
                    .addResource(sAotIsolateSnapshotData)
                    .addResource(sAotIsolateSnapshotInstr);
        }
        sResourceExtractor.start();
    }

    /**
     * Returns a list of the file names at the root of the application's asset
     * path.
     */
    private static Set<String> listAssets(Context applicationContext, String path) {
        AssetManager manager = applicationContext.getResources().getAssets();
        try {
            return ImmutableSetBuilder.<String>newInstance()
                    .add(manager.list(path))
                    .build();
        } catch (IOException e) {
            Log.e(TAG, "Unable to list assets", e);
            throw new RuntimeException(e);
        }
    }

    private static void initAot(Context applicationContext) {
        Set<String> assets = listAssets(applicationContext, "");
        sIsPrecompiledAsBlobs = assets.containsAll(Arrays.asList(
                sAotVmSnapshotData,
                sAotVmSnapshotInstr,
                sAotIsolateSnapshotData,
                sAotIsolateSnapshotInstr
        ));
        sIsPrecompiledAsSharedLibrary = assets.contains(sAotSharedLibraryPath);
        if (sIsPrecompiledAsBlobs && sIsPrecompiledAsSharedLibrary) {
            throw new RuntimeException(
                    "Found precompiled app as shared library and as Dart VM snapshots.");
        }
    }

    public static boolean isRunningPrecompiledCode() {
        return sIsPrecompiledAsBlobs || sIsPrecompiledAsSharedLibrary;
    }

    public static String findAppBundlePath(Context applicationContext) {
        String dataDirectory = PathUtils.getDataDirectory(applicationContext);
        File appBundle = new File(dataDirectory, sFlutterAssetsDir);
        return appBundle.exists() ? appBundle.getPath() : null;
    }

    public static String getUpdateInstallationPath() {
//        return /*sResourceUpdater == null ? null :*/ sResourceUpdater.getUpdateInstallationPath();
        return null;
    }

    /**
     * Returns the file name for the given asset.
     * The returned file name can be used to access the asset in the APK
     * through the {@link AssetManager} API.
     *
     * @param asset the name of the asset. The name can be hierarchical
     * @return the filename to be used with {@link AssetManager}
     */
    public static String getLookupKeyForAsset(String asset) {
        return fromFlutterAssets(asset);
    }

    /**
     * Returns the file name for the given asset which originates from the
     * specified packageName. The returned file name can be used to access
     * the asset in the APK through the {@link AssetManager} API.
     *
     * @param asset       the name of the asset. The name can be hierarchical
     * @param packageName the name of the package from which the asset originates
     * @return the file name to be used with {@link AssetManager}
     */
    public static String getLookupKeyForAsset(String asset, String packageName) {
        return getLookupKeyForAsset(
                "packages" + File.separator + packageName + File.separator + asset);
    }
}
