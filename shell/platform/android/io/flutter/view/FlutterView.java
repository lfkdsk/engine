// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package io.flutter.view;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.os.Build;
import android.os.Handler;
import android.os.LocaleList;
import android.support.annotation.RequiresApi;
import android.text.format.DateFormat;
import android.util.AttributeSet;
import android.util.Log;
// BD MOD: START
//import android.view.*;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowInsets;
import android.view.WindowManager;
// END
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityNodeProvider;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
// BD MOD: START
//import android.view.inputmethod.InputMethodManager;

import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicLong;

// END
import io.flutter.embedding.engine.FlutterJNI;
import io.flutter.app.FlutterPluginRegistry;
import io.flutter.embedding.android.AndroidKeyProcessor;
import io.flutter.embedding.android.AndroidTouchProcessor;
import io.flutter.embedding.engine.dart.DartExecutor;
import io.flutter.embedding.engine.renderer.FlutterRenderer;
import io.flutter.embedding.engine.systemchannels.AccessibilityChannel;
import io.flutter.embedding.engine.systemchannels.KeyEventChannel;
import io.flutter.embedding.engine.systemchannels.LifecycleChannel;
import io.flutter.embedding.engine.systemchannels.LocalizationChannel;
import io.flutter.embedding.engine.systemchannels.NavigationChannel;
import io.flutter.embedding.engine.systemchannels.PlatformChannel;
import io.flutter.embedding.engine.systemchannels.SettingsChannel;
import io.flutter.embedding.engine.systemchannels.SystemChannel;
// BD MOD: START
//import io.flutter.plugin.common.*;
import io.flutter.plugin.common.ActivityLifecycleListener;
import io.flutter.plugin.common.BinaryMessenger;
// END
import io.flutter.plugin.editing.TextInputPlugin;
import io.flutter.plugin.platform.PlatformPlugin;
import io.flutter.plugin.platform.PlatformViewsController;
// BD MOD: START
//
//import java.lang.ref.WeakReference;
//import java.nio.ByteBuffer;
//import java.nio.ByteOrder;
//import java.util.*;
//import java.util.concurrent.atomic.AtomicLong;
// END

/**
 * An Android view containing a Flutter app.
 */
// BD MOD: add ImageLoaderRegistry
public class FlutterView extends SurfaceView implements BinaryMessenger, TextureRegistry, IFlutterView {
    /**
     * Interface for those objects that maintain and expose a reference to a
     * {@code FlutterView} (such as a full-screen Flutter activity).
     *
     * <p>
     * This indirection is provided to support applications that use an activity
     * other than {@link io.flutter.app.FlutterActivity} (e.g. Android v4 support
     * library's {@code FragmentActivity}). It allows Flutter plugins to deal in
     * this interface and not require that the activity be a subclass of
     * {@code FlutterActivity}.
     * </p>
     */
    public interface Provider {
        /**
         * Returns a reference to the Flutter view maintained by this object. This may
         * be {@code null}.
         */
        FlutterView getFlutterView();
    }

    private static final String TAG = "FlutterView";

    public static final class ViewportMetrics {
        float devicePixelRatio = 1.0f;
        int physicalWidth = 0;
        int physicalHeight = 0;
        int physicalPaddingTop = 0;
        int physicalPaddingRight = 0;
        int physicalPaddingBottom = 0;
        int physicalPaddingLeft = 0;
        int physicalViewInsetTop = 0;
        int physicalViewInsetRight = 0;
        int physicalViewInsetBottom = 0;
        int physicalViewInsetLeft = 0;

        void update(ViewportMetrics metrics) {
            devicePixelRatio = metrics.devicePixelRatio;
            physicalWidth = metrics.physicalWidth;
            physicalHeight = metrics.physicalHeight;
            physicalPaddingTop = metrics.physicalPaddingTop;
            physicalPaddingRight = metrics.physicalPaddingRight;
            physicalPaddingBottom = metrics.physicalPaddingBottom;
            physicalPaddingLeft = metrics.physicalPaddingLeft;
            physicalViewInsetTop = metrics.physicalViewInsetTop;
            physicalViewInsetRight = metrics.physicalViewInsetRight;
            physicalViewInsetBottom = metrics.physicalViewInsetBottom;
            physicalViewInsetLeft = metrics.physicalViewInsetLeft;
        }

        /**
         * 获取当前对象的拷贝
         */
        ViewportMetrics snapShot() {
            ViewportMetrics metrics = new ViewportMetrics();
            metrics.devicePixelRatio = devicePixelRatio;
            metrics.physicalWidth = physicalWidth;
            metrics.physicalHeight = physicalHeight;
            metrics.physicalPaddingTop = physicalPaddingTop;
            metrics.physicalPaddingRight = physicalPaddingRight;
            metrics.physicalPaddingBottom = physicalPaddingBottom;
            metrics.physicalPaddingLeft = physicalPaddingLeft;
            metrics.physicalViewInsetTop = physicalViewInsetTop;
            metrics.physicalViewInsetRight = physicalViewInsetRight;
            metrics.physicalViewInsetBottom = physicalViewInsetBottom;
            metrics.physicalViewInsetLeft = physicalViewInsetLeft;
            return metrics;
        }

    }

    private final DartExecutor dartExecutor;
    private final FlutterRenderer flutterRenderer;
    private final NavigationChannel navigationChannel;
    private final KeyEventChannel keyEventChannel;
    private final LifecycleChannel lifecycleChannel;
    private final LocalizationChannel localizationChannel;
    private final PlatformChannel platformChannel;
    private final SettingsChannel settingsChannel;
    private final SystemChannel systemChannel;
    private final TextInputPlugin mTextInputPlugin;
    private final AndroidKeyProcessor androidKeyProcessor;
    private final AndroidTouchProcessor androidTouchProcessor;
    private AccessibilityBridge mAccessibilityNodeProvider;
    private final SurfaceHolder.Callback mSurfaceCallback;
    private final ViewportMetrics mMetrics;
    private final List<ActivityLifecycleListener> mActivityLifecycleListeners;
    private final List<FirstFrameListener> mFirstFrameListeners;
    private final AtomicLong nextTextureId = new AtomicLong(0L);
    private FlutterNativeView mNativeView;
    private boolean mIsSoftwareRenderingEnabled = false; // using the software renderer or not

    private final AccessibilityBridge.OnAccessibilityChangeListener onAccessibilityChangeListener = new AccessibilityBridge.OnAccessibilityChangeListener() {
        @Override
        public void onAccessibilityChanged(boolean isAccessibilityEnabled, boolean isTouchExplorationEnabled) {
            resetWillNotDraw(isAccessibilityEnabled, isTouchExplorationEnabled);
        }
    };

    private PlatformPlugin mPlatformPlugin;
    private WeakReference<Activity> mActivityRef;

    public FlutterView(Context context) {
        this(context, null);
    }

    public FlutterView(Context context, AttributeSet attrs) {
        this(context, attrs, null);
    }

    public FlutterView(Context context, AttributeSet attrs, FlutterNativeView nativeView) {
        super(context.getApplicationContext(), attrs);

        Activity activity = getActivity(context);
        if (activity == null) {
            throw new IllegalArgumentException("Bad context");
        }

        if (nativeView == null) {
            mNativeView = new FlutterNativeView(activity.getApplicationContext());
        } else {
            mNativeView = nativeView;
        }

        dartExecutor = mNativeView.getDartExecutor();
        flutterRenderer = new FlutterRenderer(mNativeView.getFlutterJNI());
        mIsSoftwareRenderingEnabled = FlutterJNI.nativeGetIsSoftwareRenderingEnabled();
        mMetrics = new ViewportMetrics();
        mMetrics.devicePixelRatio = context.getResources().getDisplayMetrics().density;
        setFocusable(true);
        setFocusableInTouchMode(true);

        mSurfaceCallback = new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                assertAttached();
                mNativeView.getFlutterJNI().onSurfaceCreated(holder.getSurface());
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                assertAttached();
                mNativeView.getFlutterJNI().onSurfaceChanged(width, height);
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                assertAttached();
                mNativeView.getFlutterJNI().onSurfaceDestroyed();
            }
        };
        getHolder().addCallback(mSurfaceCallback);

        mActivityLifecycleListeners = new ArrayList<>();
        mFirstFrameListeners = new ArrayList<>();

        // Create all platform channels
        navigationChannel = new NavigationChannel(dartExecutor);
        keyEventChannel = new KeyEventChannel(dartExecutor);
        lifecycleChannel = new LifecycleChannel(dartExecutor);
        localizationChannel = new LocalizationChannel(dartExecutor);
        platformChannel = new PlatformChannel(dartExecutor);
        systemChannel = new SystemChannel(dartExecutor);
        settingsChannel = new SettingsChannel(dartExecutor);

        // Create and setup plugins
        // PlatformPlugin platformPlugin = new PlatformPlugin(activity, platformChannel);
        // addActivityLifecycleListener(platformPlugin);
        mTextInputPlugin = new TextInputPlugin(this, dartExecutor);
        androidKeyProcessor = new AndroidKeyProcessor(keyEventChannel, mTextInputPlugin);
        androidTouchProcessor = new AndroidTouchProcessor(flutterRenderer);

        // Send initial platform information to Dart
        sendLocalesToDart(getResources().getConfiguration());
        sendUserPlatformSettingsToDart();

        attachActivity((Activity) context);
    }

    /**
     * Call this method when reuse this FlutterView on another Activity.
     */
    public void attachActivity(Activity activity) {
        detachActivity();
        mNativeView.attachViewAndActivity(this, activity);
        mPlatformPlugin = new PlatformPlugin(activity, platformChannel);
        addActivityLifecycleListener(mPlatformPlugin);
        mActivityRef = new WeakReference<>(activity);
    }

    /**
     * Call this method on Activity onDestroy in case of memory leak on Activity.
     */
    public void detachActivity() {
        // mPlatformPlugin hold the strong reference of Activity, need to release.
        if (mPlatformPlugin != null) {
            removeActivityLifecycleListener(mPlatformPlugin);
            mPlatformPlugin = null;
            platformChannel.setPlatformMessageHandler(null);
            if (mNativeView != null && mNativeView.isAttached()) {
                mNativeView.detachFromFlutterView();
            }
            mActivityRef = null;
        }
    }

    /**
     * 跳出到其他Activity时取出ViewportMetrics存下来，用于恢复MediaQuery中的值
     */
    public ViewportMetrics getViewPortMetrics() {
        return mMetrics.snapShot();
    }

    /**
     * attach到新Activity时恢复之前的值
     */
    public void updateViewportMetrics(ViewportMetrics viewportMetrics) {
        mMetrics.update(viewportMetrics);
        updateViewportMetrics();
    }

    private static Activity getActivity(Context context) {
        if (context == null) {
            return null;
        }
        if (context instanceof Activity) {
            return (Activity) context;
        }
        if (context instanceof ContextWrapper) {
            // Recurse up chain of base contexts until we find an Activity.
            return getActivity(((ContextWrapper) context).getBaseContext());
        }
        return null;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (!isAttached()) {
            return super.onKeyUp(keyCode, event);
        }
        androidKeyProcessor.onKeyUp(event);
        return super.onKeyUp(keyCode, event);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (!isAttached()) {
            return super.onKeyDown(keyCode, event);
        }
        androidKeyProcessor.onKeyDown(event);
        return super.onKeyDown(keyCode, event);
    }

    public FlutterNativeView getFlutterNativeView() {
        return mNativeView;
    }

    public FlutterPluginRegistry getPluginRegistry() {
        return mNativeView.getPluginRegistry();
    }

    public String getLookupKeyForAsset(String asset) {
        return FlutterMain.getLookupKeyForAsset(asset);
    }

    public String getLookupKeyForAsset(String asset, String packageName) {
        return FlutterMain.getLookupKeyForAsset(asset, packageName);
    }

    public void addActivityLifecycleListener(ActivityLifecycleListener listener) {
        mActivityLifecycleListeners.add(listener);
    }

    public void removeActivityLifecycleListener(ActivityLifecycleListener listener) {
        mActivityLifecycleListeners.remove(listener);
    }

    public void onStart() {
        lifecycleChannel.appIsInactive();
    }

    public void onPause() {
        lifecycleChannel.appIsInactive();
    }

    public void onPostResume() {
        for (ActivityLifecycleListener listener : mActivityLifecycleListeners) {
            listener.onPostResume();
        }
        lifecycleChannel.appIsResumed();
    }

    public void onStop() {
        lifecycleChannel.appIsPaused();
    }

    public void onMemoryPressure() {
        systemChannel.sendMemoryPressureWarning();
    }

    /**
     * Provide a listener that will be called once when the FlutterView renders its
     * first frame to the underlaying SurfaceView.
     */
    public void addFirstFrameListener(FirstFrameListener listener) {
        mFirstFrameListeners.add(listener);
    }

    /**
     * Remove an existing first frame listener.
     */
    public void removeFirstFrameListener(FirstFrameListener listener) {
        mFirstFrameListeners.remove(listener);
    }

    /**
     * Updates this to support rendering as a transparent {@link SurfaceView}.
     *
     * Sets it on top of its window. The background color still needs to be
     * controlled from within the Flutter UI itself.
     */
    public void enableTransparentBackground() {
        setZOrderOnTop(true);
        getHolder().setFormat(PixelFormat.TRANSPARENT);
    }

    /**
     * Reverts this back to the {@link SurfaceView} defaults, at the back of its
     * window and opaque.
     */
    public void disableTransparentBackground() {
        setZOrderOnTop(false);
        getHolder().setFormat(PixelFormat.OPAQUE);
    }

    public void setInitialRoute(String route) {
        navigationChannel.setInitialRoute(route);
    }

    public void pushRoute(String route) {
        navigationChannel.pushRoute(route);
    }

    public void popRoute() {
        navigationChannel.popRoute();
    }

    private void sendUserPlatformSettingsToDart() {
        // Lookup the current brightness of the Android OS.
        boolean isNightModeOn = (getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK) == Configuration.UI_MODE_NIGHT_YES;
        SettingsChannel.PlatformBrightness brightness = isNightModeOn
            ? SettingsChannel.PlatformBrightness.dark
            : SettingsChannel.PlatformBrightness.light;

        settingsChannel
            .startMessage()
            .setTextScaleFactor(getResources().getConfiguration().fontScale)
            .setUse24HourFormat(DateFormat.is24HourFormat(getContext()))
            .setPlatformBrightness(brightness)
            .send();
    }

    @SuppressWarnings("deprecation")
    private void sendLocalesToDart(Configuration config) {
        List<Locale> locales = new ArrayList<>();
        if (Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N) {
            LocaleList localeList = config.getLocales();
            int localeCount = localeList.size();
            for (int index = 0; index < localeCount; ++index) {
                Locale locale = localeList.get(index);
                locales.add(locale);
            }
        } else {
            locales.add(config.locale);
        }
        localizationChannel.sendLocales(locales);
    }

    @Override
    protected void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        sendLocalesToDart(newConfig);
        sendUserPlatformSettingsToDart();
    }

    float getDevicePixelRatio() {
        return mMetrics.devicePixelRatio;
    }

    public FlutterNativeView detach() {
        if (!isAttached())
            return null;
        getHolder().removeCallback(mSurfaceCallback);
        mNativeView.detachFromFlutterView();

        FlutterNativeView view = mNativeView;
        mNativeView = null;
        return view;
    }

    public void destroy() {
        // BD ADD: START
        if (mAccessibilityNodeProvider != null) {
            mAccessibilityNodeProvider.release();
            mAccessibilityNodeProvider = null;
        }
        // END

        if (!isAttached())
            return;

        getHolder().removeCallback(mSurfaceCallback);

        mNativeView.destroy();
        mNativeView = null;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        return mTextInputPlugin.createInputConnection(this, outAttrs);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (!isAttached()) {
            return super.onTouchEvent(event);
        }

        // TODO(abarth): This version check might not be effective in some
        // versions of Android that statically compile code and will be upset
        // at the lack of |requestUnbufferedDispatch|. Instead, we should factor
        // version-dependent code into separate classes for each supported
        // version and dispatch dynamically.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            requestUnbufferedDispatch(event);
        }

        return androidTouchProcessor.onTouchEvent(event);
    }

    @Override
    public boolean onHoverEvent(MotionEvent event) {
        if (!isAttached()) {
            return super.onHoverEvent(event);
        }

        boolean handled = mAccessibilityNodeProvider.onAccessibilityHoverEvent(event);
        if (!handled) {
            // TODO(ianh): Expose hover events to the platform,
            // implementing ADD, REMOVE, etc.
        }
        return handled;
    }

    /**
     * Invoked by Android when a generic motion event occurs, e.g., joystick movement, mouse hover,
     * track pad touches, scroll wheel movements, etc.
     *
     * Flutter handles all of its own gesture detection and processing, therefore this
     * method forwards all {@link MotionEvent} data from Android to Flutter.
     */
    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
        boolean handled = isAttached() && androidTouchProcessor.onGenericMotionEvent(event);
        return handled ? true : super.onGenericMotionEvent(event);
    }

    @Override
    protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        mMetrics.physicalWidth = width;
        mMetrics.physicalHeight = height;
        updateViewportMetrics();
        super.onSizeChanged(width, height, oldWidth, oldHeight);
    }

    // TODO(garyq): Add support for notch cutout API
    // Decide if we want to zero the padding of the sides. When in Landscape orientation,
    // android may decide to place the software navigation bars on the side. When the nav
    // bar is hidden, the reported insets should be removed to prevent extra useless space
    // on the sides.
    enum ZeroSides { NONE, LEFT, RIGHT, BOTH }
    ZeroSides calculateShouldZeroSides() {
        // We get both orientation and rotation because rotation is all 4
        // rotations relative to default rotation while orientation is portrait
        // or landscape. By combining both, we can obtain a more precise measure
        // of the rotation.
        Activity activity = mActivityRef != null ? mActivityRef.get() : null;
        if (activity == null) {
            return ZeroSides.NONE;
        }
        int orientation = activity.getResources().getConfiguration().orientation;
        int rotation = activity.getWindowManager().getDefaultDisplay().getRotation();

        if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
            if (rotation == Surface.ROTATION_90) {
                return ZeroSides.RIGHT;
            }
            else if (rotation == Surface.ROTATION_270) {
                // In android API >= 23, the nav bar always appears on the "bottom" (USB) side.
                return Build.VERSION.SDK_INT >= 23 ? ZeroSides.LEFT : ZeroSides.RIGHT;
            }
            // Ambiguous orientation due to landscape left/right default. Zero both sides.
            else if (rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_180) {
                return ZeroSides.BOTH;
            }
        }
        // Square orientation deprecated in API 16, we will not check for it and return false
        // to be safe and not remove any unique padding for the devices that do use it.
        return ZeroSides.NONE;
    }

    // TODO(garyq): Use clean ways to detect keyboard instead of heuristics if possible
    // TODO(garyq): The keyboard detection may interact strangely with
    //   https://github.com/flutter/flutter/issues/22061

    // Uses inset heights and screen heights as a heuristic to determine if the insets should
    // be padded. When the on-screen keyboard is detected, we want to include the full inset
    // but when the inset is just the hidden nav bar, we want to provide a zero inset so the space
    // can be used.
    @TargetApi(20)
    @RequiresApi(20)
    int calculateBottomKeyboardInset(WindowInsets insets) {
        int screenHeight = getRootView().getHeight();
        // Magic number due to this being a heuristic. This should be replaced, but we have not
        // found a clean way to do it yet (Sept. 2018)
        final double keyboardHeightRatioHeuristic = 0.18;
        if (insets.getSystemWindowInsetBottom() < screenHeight * keyboardHeightRatioHeuristic) {
            // Is not a keyboard, so return zero as inset.
            return 0;
        }
        else {
            // Is a keyboard, so return the full inset.
            return insets.getSystemWindowInsetBottom();
        }
    }

    // This callback is not present in API < 20, which means lower API devices will see
    // the wider than expected padding when the status and navigation bars are hidden.
    @Override
    @TargetApi(20)
    @RequiresApi(20)
    public final WindowInsets onApplyWindowInsets(WindowInsets insets) {
        boolean statusBarHidden =
            (SYSTEM_UI_FLAG_FULLSCREEN & getWindowSystemUiVisibility()) != 0;
        boolean navigationBarHidden =
            (SYSTEM_UI_FLAG_HIDE_NAVIGATION & getWindowSystemUiVisibility()) != 0;

        // We zero the left and/or right sides to prevent the padding the
        // navigation bar would have caused.
        ZeroSides zeroSides = ZeroSides.NONE;
        if (navigationBarHidden) {
            zeroSides = calculateShouldZeroSides();
        }

        // The padding on top should be removed when the statusbar is hidden.
        mMetrics.physicalPaddingTop = statusBarHidden ? 0 : insets.getSystemWindowInsetTop();
        mMetrics.physicalPaddingRight =
            zeroSides == ZeroSides.RIGHT || zeroSides == ZeroSides.BOTH ? 0 : insets.getSystemWindowInsetRight();
        mMetrics.physicalPaddingBottom = 0;
        mMetrics.physicalPaddingLeft =
            zeroSides == ZeroSides.LEFT || zeroSides == ZeroSides.BOTH ? 0 : insets.getSystemWindowInsetLeft();

        // Bottom system inset (keyboard) should adjust scrollable bottom edge (inset).
        mMetrics.physicalViewInsetTop = 0;
        mMetrics.physicalViewInsetRight = 0;
        // We perform hidden navbar and keyboard handling if the navbar is set to hidden. Otherwise,
        // the navbar padding should always be provided.
        mMetrics.physicalViewInsetBottom =
            navigationBarHidden ? calculateBottomKeyboardInset(insets) : insets.getSystemWindowInsetBottom();
        mMetrics.physicalViewInsetLeft = 0;
        updateViewportMetrics();
        return super.onApplyWindowInsets(insets);
    }

    @Override
    @SuppressWarnings("deprecation")
    protected boolean fitSystemWindows(Rect insets) {
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.KITKAT) {
            // Status bar, left/right system insets partially obscure content (padding).
            mMetrics.physicalPaddingTop = insets.top;
            mMetrics.physicalPaddingRight = insets.right;
            mMetrics.physicalPaddingBottom = 0;
            mMetrics.physicalPaddingLeft = insets.left;

            // Bottom system inset (keyboard) should adjust scrollable bottom edge (inset).
            mMetrics.physicalViewInsetTop = 0;
            mMetrics.physicalViewInsetRight = 0;
            mMetrics.physicalViewInsetBottom = insets.bottom;
            mMetrics.physicalViewInsetLeft = 0;
            updateViewportMetrics();
            return true;
        } else {
            return super.fitSystemWindows(insets);
        }
    }

    private boolean isAttached() {
        return mNativeView != null && mNativeView.isAttached();
    }

    void assertAttached() {
        if (!isAttached())
            throw new AssertionError("Platform view is not attached");
    }

    private void preRun() {
        resetAccessibilityTree();
    }

    @Override
    public void resetAccessibilityTree() {
        if (mAccessibilityNodeProvider != null) {
            mAccessibilityNodeProvider.reset();
        }
    }

    private void postRun() {
    }

    public void runFromBundle(FlutterRunArguments args) {
      assertAttached();
      preRun();
      mNativeView.runFromBundle(args);
      postRun();
    }

    /**
     * @deprecated
     * Please use runFromBundle with `FlutterRunArguments`.
     */
    @Deprecated
    public void runFromBundle(String bundlePath, String defaultPath) {
        runFromBundle(bundlePath, defaultPath, "main", false);
    }

    /**
     * @deprecated
     * Please use runFromBundle with `FlutterRunArguments`.
     */
    @Deprecated
    public void runFromBundle(String bundlePath, String defaultPath, String entrypoint) {
        runFromBundle(bundlePath, defaultPath, entrypoint, false);
    }

    /**
     * @deprecated
     * Please use runFromBundle with `FlutterRunArguments`.
     * Parameter `reuseRuntimeController` has no effect.
     */
    @Deprecated
    public void runFromBundle(String bundlePath, String defaultPath, String entrypoint, boolean reuseRuntimeController) {
        FlutterRunArguments args = new FlutterRunArguments();
        args.bundlePath = bundlePath;
        args.entrypoint = entrypoint;
        args.defaultPath = defaultPath;
        runFromBundle(args);
    }

    /**
     * Return the most recent frame as a bitmap.
     *
     * @return A bitmap.
     */
    public Bitmap getBitmap() {
        assertAttached();
        return mNativeView.getFlutterJNI().getBitmap();
    }

    private void updateViewportMetrics() {
        if (!isAttached())
            return;
        mNativeView.getFlutterJNI().setViewportMetrics(mMetrics.devicePixelRatio, mMetrics.physicalWidth,
                mMetrics.physicalHeight, mMetrics.physicalPaddingTop, mMetrics.physicalPaddingRight,
                mMetrics.physicalPaddingBottom, mMetrics.physicalPaddingLeft, mMetrics.physicalViewInsetTop,
                mMetrics.physicalViewInsetRight, mMetrics.physicalViewInsetBottom, mMetrics.physicalViewInsetLeft);

        WindowManager wm = (WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE);
        float fps = wm.getDefaultDisplay().getRefreshRate();
        VsyncWaiter.refreshPeriodNanos = (long) (1000000000.0 / fps);
        VsyncWaiter.refreshRateFPS = fps;
    }

    // Called by native to update the semantics/accessibility tree.
    @Override
    public void updateSemantics(ByteBuffer buffer, String[] strings) {
        try {
            if (mAccessibilityNodeProvider != null) {
                buffer.order(ByteOrder.LITTLE_ENDIAN);
                mAccessibilityNodeProvider.updateSemantics(buffer, strings);
            }
        } catch (Exception ex) {
            Log.e(TAG, "Uncaught exception while updating semantics", ex);
        }
    }

    @Override
    public void updateCustomAccessibilityActions(ByteBuffer buffer, String[] strings) {
        try {
            if (mAccessibilityNodeProvider != null) {
                buffer.order(ByteOrder.LITTLE_ENDIAN);
                mAccessibilityNodeProvider.updateCustomAccessibilityActions(buffer, strings);
            }
        } catch (Exception ex) {
            Log.e(TAG, "Uncaught exception while updating local context actions", ex);
        }
    }

    // Called by native to notify first Flutter frame rendered.
    @Override
    public void onFirstFrame() {
        // Allow listeners to remove themselves when they are called.
        List<FirstFrameListener> listeners = new ArrayList<>(mFirstFrameListeners);
        for (FirstFrameListener listener : listeners) {
            listener.onFirstFrame();
        }
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();

        // BD ADD
        if (mAccessibilityNodeProvider == null) {
            PlatformViewsController platformViewsController = getPluginRegistry().getPlatformViewsController();
            mAccessibilityNodeProvider = new AccessibilityBridge(
                    this,
                    new AccessibilityChannel(dartExecutor, getFlutterNativeView().getFlutterJNI()),
                    (AccessibilityManager) getContext().getSystemService(Context.ACCESSIBILITY_SERVICE),
                    getContext().getContentResolver(),
                    platformViewsController
            );
            mAccessibilityNodeProvider.setOnAccessibilityChangeListener(onAccessibilityChangeListener);
        }

        resetWillNotDraw(
            mAccessibilityNodeProvider.isAccessibilityEnabled(),
            mAccessibilityNodeProvider.isTouchExplorationEnabled()
        );
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();

        // BD DEL:
        // mAccessibilityNodeProvider.release();
        // mAccessibilityNodeProvider = null;
    }

    // TODO(mattcarroll): Confer with Ian as to why we need this method. Delete if possible, otherwise add comments.
    private void resetWillNotDraw(boolean isAccessibilityEnabled, boolean isTouchExplorationEnabled) {
        if (!mIsSoftwareRenderingEnabled) {
            setWillNotDraw(!(isAccessibilityEnabled || isTouchExplorationEnabled));
        } else {
            setWillNotDraw(false);
        }
    }

    @Override
    public AccessibilityNodeProvider getAccessibilityNodeProvider() {
        if (mAccessibilityNodeProvider != null && mAccessibilityNodeProvider.isAccessibilityEnabled()) {
            return mAccessibilityNodeProvider;
        } else {
            // TODO(goderbauer): when a11y is off this should return a one-off snapshot of
            // the a11y
            // tree.
            return null;
        }
    }

    @Override
    public void send(String channel, ByteBuffer message) {
        send(channel, message, null);
    }

    @Override
    public void send(String channel, ByteBuffer message, BinaryReply callback) {
        if (!isAttached()) {
            Log.d(TAG, "FlutterView.send called on a detached view, channel=" + channel);
            return;
        }
        mNativeView.send(channel, message, callback);
    }

    @Override
    public void setMessageHandler(String channel, BinaryMessageHandler handler) {
        mNativeView.setMessageHandler(channel, handler);
    }

    /**
     * Listener will be called on the Android UI thread once when Flutter renders
     * the first frame.
     */
    public interface FirstFrameListener {
        void onFirstFrame();
    }

    @Override
    public TextureRegistry.SurfaceTextureEntry createSurfaceTexture() {
        final SurfaceTexture surfaceTexture = new SurfaceTexture(0);
        surfaceTexture.detachFromGLContext();
        final SurfaceTextureRegistryEntry entry = new SurfaceTextureRegistryEntry(nextTextureId.getAndIncrement(),
                surfaceTexture);
        mNativeView.getFlutterJNI().registerTexture(entry.id(), surfaceTexture);
        return entry;
    }

    final class SurfaceTextureRegistryEntry implements TextureRegistry.SurfaceTextureEntry {
        private final long id;
        private final SurfaceTexture surfaceTexture;
        private boolean released;

        SurfaceTextureRegistryEntry(long id, SurfaceTexture surfaceTexture) {
            this.id = id;
            this.surfaceTexture = surfaceTexture;

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                // The callback relies on being executed on the UI thread (unsynchronised read of mNativeView
                // and also the engine code check for platform thread in Shell::OnPlatformViewMarkTextureFrameAvailable),
                // so we explicitly pass a Handler for the current thread.
                this.surfaceTexture.setOnFrameAvailableListener(onFrameListener, new Handler());
            } else {
                // Android documentation states that the listener can be called on an arbitrary thread.
                // But in practice, versions of Android that predate the newer API will call the listener
                // on the thread where the SurfaceTexture was constructed.
                this.surfaceTexture.setOnFrameAvailableListener(onFrameListener);
            }
        }

        private SurfaceTexture.OnFrameAvailableListener onFrameListener = new SurfaceTexture.OnFrameAvailableListener() {
            @Override
            public void onFrameAvailable(SurfaceTexture texture) {
                if (released || mNativeView == null) {
                    // Even though we make sure to unregister the callback before releasing, as of Android O
                    // SurfaceTexture has a data race when accessing the callback, so the callback may
                    // still be called by a stale reference after released==true and mNativeView==null.
                    return;
                }
                mNativeView.getFlutterJNI().markTextureFrameAvailable(SurfaceTextureRegistryEntry.this.id);
            }
        };

        @Override
        public SurfaceTexture surfaceTexture() {
            return surfaceTexture;
        }

        @Override
        public long id() {
            return id;
        }

        @Override
        public void release() {
            if (released) {
                return;
            }
            released = true;

            // The ordering of the next 3 calls is important:
            // First we remove the frame listener, then we release the SurfaceTexture, and only after we unregister
            // the texture which actually deletes the GL texture.

            // Otherwise onFrameAvailableListener might be called after mNativeView==null
            // (https://github.com/flutter/flutter/issues/20951). See also the check in onFrameAvailable.
            surfaceTexture.setOnFrameAvailableListener(null);
            surfaceTexture.release();
            mNativeView.getFlutterJNI().unregisterTexture(id);
        }
    }
}
