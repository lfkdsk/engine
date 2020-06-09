// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package io.flutter.view;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Insets;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.os.Build;
import android.os.Handler;
import android.os.LocaleList;
// BD ADD
import android.os.Looper;
import android.support.annotation.NonNull;
import android.support.annotation.RequiresApi;
import android.support.annotation.UiThread;
import android.text.format.DateFormat;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.View;
import android.view.WindowInsets;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityNodeProvider;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;

import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicLong;

import io.flutter.app.FlutterPluginRegistry;
import io.flutter.embedding.android.AndroidKeyProcessor;
import io.flutter.embedding.android.AndroidTouchProcessor;
import io.flutter.embedding.engine.FlutterJNI;
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
import io.flutter.plugin.common.ActivityLifecycleListener;
import io.flutter.plugin.common.BinaryMessenger;
import io.flutter.plugin.editing.TextInputPlugin;
import io.flutter.plugin.platform.PlatformPlugin;
import io.flutter.plugin.platform.PlatformViewsController;

/**
 * BD ADD:
 * An Android view containing a Flutter app.
 */
public class FlutterTextureView extends CachedTextureView implements BinaryMessenger, TextureRegistry, IFlutterView, ImageLoaderRegistry {
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
        FlutterTextureView getFlutterView();
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
        int systemGestureInsetTop = 0;
        int systemGestureInsetRight = 0;
        int systemGestureInsetBottom = 0;
        int systemGestureInsetLeft = 0;

        // BD ADD: XieRan
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
            systemGestureInsetTop = metrics.systemGestureInsetTop;
            systemGestureInsetRight = metrics.systemGestureInsetRight;
            systemGestureInsetBottom = metrics.systemGestureInsetBottom;
            systemGestureInsetLeft = metrics.systemGestureInsetLeft;
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
            metrics.systemGestureInsetTop = systemGestureInsetTop;
            metrics.systemGestureInsetRight = systemGestureInsetRight;
            metrics.systemGestureInsetBottom = systemGestureInsetBottom;
            metrics.systemGestureInsetLeft = systemGestureInsetLeft;
            return metrics;
        }
        // END

    }

    private final List<SurfaceTextureListener> mSurfaceTextureListeners = new LinkedList<>();
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
    private final ViewportMetrics mMetrics;
    private final List<ActivityLifecycleListener> mActivityLifecycleListeners;
    private final List<FirstFrameListener> mFirstFrameListeners;
    private final AtomicLong nextTextureId = new AtomicLong(0L);
    private FlutterNativeView mNativeView;
    private boolean mIsSoftwareRenderingEnabled = false; // using the software renderer or not
    private boolean didRenderFirstFrame = false;

    // BD ADD: HuWeijie
    private AndroidImageLoader mAndroidImageLoader;
    // END

    private final AccessibilityBridge.OnAccessibilityChangeListener onAccessibilityChangeListener = new AccessibilityBridge.OnAccessibilityChangeListener() {
        @Override
        public void onAccessibilityChanged(boolean isAccessibilityEnabled, boolean isTouchExplorationEnabled) {
            resetWillNotDraw(isAccessibilityEnabled, isTouchExplorationEnabled);
        }
    };

    // BD ADD: XIERAN
    private PlatformPlugin mPlatformPlugin;
    private WeakReference<Activity> mActivityRef;
    private ActivityLifecycleListener mActivityLifecycleListener;

    public FlutterTextureView(Context context) {
        this(context, null);
    }

    public FlutterTextureView(Context context, AttributeSet attrs) {
        this(context, attrs, null);
    }

    public FlutterTextureView(Context context, AttributeSet attrs, FlutterNativeView nativeView) {
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
        mIsSoftwareRenderingEnabled = mNativeView.getFlutterJNI().nativeGetIsSoftwareRenderingEnabled();
        mMetrics = new ViewportMetrics();
        mMetrics.devicePixelRatio = context.getResources().getDisplayMetrics().density;
        setFocusable(true);
        setFocusableInTouchMode(true);

        SurfaceTextureListener surfaceTextureListener = new SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(SurfaceTexture surfaceTexture, int width, int height) {
                assertAttached();
                if (mNativeView != null) {
                    mNativeView.getFlutterJNI().onSurfaceCreated(getSurface());
                }
                for(SurfaceTextureListener listener : mSurfaceTextureListeners) {
                    listener.onSurfaceTextureAvailable(surfaceTexture, width, height);
                }
            }

            @Override
            public void onSurfaceTextureSizeChanged(SurfaceTexture surfaceTexture, int width, int height) {
                if (mNativeView != null) {
                    mNativeView.getFlutterJNI().onSurfaceChanged(width, height);
                }
                for(SurfaceTextureListener listener : mSurfaceTextureListeners) {
                    listener.onSurfaceTextureSizeChanged(surfaceTexture, width, height);
                }
            }

            @Override
            public boolean onSurfaceTextureDestroyed(SurfaceTexture surfaceTexture) {
                if (mNativeView != null) {
                    mNativeView.getFlutterJNI().onSurfaceDestroyed();
                }
                for(SurfaceTextureListener listener : mSurfaceTextureListeners) {
                    listener.onSurfaceTextureDestroyed(surfaceTexture);
                }
                return true;
            }

            @Override
            public void onSurfaceTextureUpdated(SurfaceTexture surfaceTexture) {
                for(SurfaceTextureListener listener : mSurfaceTextureListeners) {
                    listener.onSurfaceTextureUpdated(surfaceTexture);
                }
            }
        };
        setSurfaceTextureListener(surfaceTextureListener);

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
        // BD MOD: XieRan
        mActivityLifecycleListener = new ActivityLifecycleListener() {
            @Override
            public void onPostResume() {
                mPlatformPlugin.updateSystemUiOverlays();
            }
        };
//         mImm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
        PlatformViewsController platformViewsController = mNativeView.getPluginRegistry().getPlatformViewsController();
        mTextInputPlugin = new TextInputPlugin(this, dartExecutor, platformViewsController);
        androidKeyProcessor = new AndroidKeyProcessor(keyEventChannel, mTextInputPlugin);
        androidTouchProcessor = new AndroidTouchProcessor(flutterRenderer);
        mNativeView.getPluginRegistry().getPlatformViewsController().attachTextInputPlugin(mTextInputPlugin);

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
        addActivityLifecycleListener(mActivityLifecycleListener);
        mActivityRef = new WeakReference<>(activity);
    }

    /**
     * Call this method on Activity onDestroy in case of memory leak on Activity.
     */
    public void detachActivity() {
        // mPlatformPlugin hold the strong reference of Activity, need to release.
        if (mPlatformPlugin != null) {
            removeActivityLifecycleListener(mActivityLifecycleListener);
            mPlatformPlugin = null;
            platformChannel.setPlatformMessageHandler(null);
            if (mNativeView != null && mNativeView.isAttached()) {
                mNativeView.detachFromFlutterView();
            }
            mActivityRef = null;
        }
    }

    public void addSurfaceTextureListener(SurfaceTextureListener listener) {
        mSurfaceTextureListeners.add(listener);
    }

    public void removeSurfaceTextureListener(SurfaceTextureListener listener) {
        mSurfaceTextureListeners.remove(listener);
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

    @NonNull
    public DartExecutor getDartExecutor() {
        return dartExecutor;
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
     * Returns true if the Flutter experience associated with this {@code FlutterView} has
     * rendered its first frame, or false otherwise.
     */
    public boolean hasRenderedFirstFrame() {
        return didRenderFirstFrame;
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
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
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
        setSurfaceTextureListener(null);
        mNativeView.detachFromFlutterView();

        FlutterNativeView view = mNativeView;
        mNativeView = null;
        return view;
    }

    public void destroy() {
        if (mAccessibilityNodeProvider != null) {
            mAccessibilityNodeProvider.release();
            mAccessibilityNodeProvider = null;
        }
        if (!isAttached())
            return;

        setSurfaceTextureListener(null);

        mNativeView.destroy();
        mNativeView = null;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        return mTextInputPlugin.createInputConnection(this, outAttrs);
    }

    @Override
    public boolean checkInputConnectionProxy(View view) {
        return mNativeView.getPluginRegistry().getPlatformViewsController().checkInputConnectionProxy(view);
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
    // The annotations to suppress "InlinedApi" and "NewApi" lints prevent lint warnings
    // caused by usage of Android Q APIs. These calls are safe because they are
    // guarded.
    @Override
    @TargetApi(20)
    @RequiresApi(20)
    @SuppressLint({"InlinedApi", "NewApi"})
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

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            Insets systemGestureInsets = insets.getSystemGestureInsets();
            mMetrics.systemGestureInsetTop = systemGestureInsets.top;
            mMetrics.systemGestureInsetRight = systemGestureInsets.right;
            mMetrics.systemGestureInsetBottom = systemGestureInsets.bottom;
            mMetrics.systemGestureInsetLeft = systemGestureInsets.left;
        }
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
     * Return the most recent frame as a bitmap.
     *
     * BD ADD: START
     * Some device's system method will call this method on non-UI thread, which will cause crash
     * See https://slardar.bytedance.net/node/app_detail/?aid=1691&os=Android&region=cn&lang=zh-Hans#/abnormal/detail/app/62f5310962a5c0751ead01b5a49de91d
     * So,return null on non-UI thread
     * END
     *
     * @return A bitmap.
     */
    public Bitmap getBitmap() {
        assertAttached();
        // BD ADD: START
        if (Looper.getMainLooper() != Looper.myLooper()) {
            return null;
        }
        // END
        return mNativeView.getFlutterJNI().getBitmap();
    }

    private void updateViewportMetrics() {
        if (!isAttached())
            return;
        mNativeView.getFlutterJNI().setViewportMetrics(
                mMetrics.devicePixelRatio,
                mMetrics.physicalWidth,
                mMetrics.physicalHeight,
                mMetrics.physicalPaddingTop,
                mMetrics.physicalPaddingRight,
                mMetrics.physicalPaddingBottom,
                mMetrics.physicalPaddingLeft,
                mMetrics.physicalViewInsetTop,
                mMetrics.physicalViewInsetRight,
                mMetrics.physicalViewInsetBottom,
                mMetrics.physicalViewInsetLeft,
                mMetrics.systemGestureInsetTop,
                mMetrics.systemGestureInsetRight,
                mMetrics.systemGestureInsetBottom,
                mMetrics.systemGestureInsetLeft
        );
    }

    // Called by FlutterNativeView to notify first Flutter frame rendered.
    // BD ADD: XieRan
    @Override
    // END
    public void onFirstFrame() {
        didRenderFirstFrame = true;

        // Allow listeners to remove themselves when they are called.
        List<FirstFrameListener> listeners = new ArrayList<>(mFirstFrameListeners);
        for (FirstFrameListener listener : listeners) {
            listener.onFirstFrame();
        }
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
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
    }

    // TODO(mattcarroll): Confer with Ian as to why we need this method. Delete if possible, otherwise add comments.
    private void resetWillNotDraw(boolean isAccessibilityEnabled, boolean isTouchExplorationEnabled) {
        // if (!mIsSoftwareRenderingEnabled) {
        //     setWillNotDraw(!(isAccessibilityEnabled || isTouchExplorationEnabled));
        // } else {
        //     setWillNotDraw(false);
        // }
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
    @UiThread
    public void send(String channel, ByteBuffer message) {
        send(channel, message, null);
    }

    @Override
    @UiThread
    public void send(String channel, ByteBuffer message, BinaryReply callback) {
        if (!isAttached()) {
            Log.d(TAG, "FlutterView.send called on a detached view, channel=" + channel);
            return;
        }
        mNativeView.send(channel, message, callback);
    }

    @Override
    @UiThread
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

    /**
     * BD ADD: register android image loader
     */
    @Override
    public void registerImageLoader(AndroidImageLoader.RealImageLoader realImageLoader) {
        ensureAndroidImageLoaderAttached();
        mAndroidImageLoader.registerImageLoader(realImageLoader);
    }
    /**
     * BD ADD: unregister android image loader
     */
    @Override
    public void unRegisterImageLoader() {
        mAndroidImageLoader.unRegisterImageLoader();
    }
    /**
     * BD ADD: initialize android image loader
     */
    private void ensureAndroidImageLoaderAttached() {
        if (mAndroidImageLoader != null) {
            return;
        }

        mAndroidImageLoader = new AndroidImageLoader();
        registerAndroidImageLoader(mAndroidImageLoader);
    }
    /**
     * BD ADD: register android image loader
     */
    private void registerAndroidImageLoader(AndroidImageLoader androidImageLoader) {
        mNativeView.getFlutterJNI().registerAndroidImageLoader(androidImageLoader);
    }
    /**
     * BD ADD: unregister android image loader
     */
    private void unRegisterAndroidImageLoader() {
        mNativeView.getFlutterJNI().unRegisterAndroidImageLoader();
    }

    @Override
    public SurfaceTextureEntry createSurfaceTexture() {
        final SurfaceTexture surfaceTexture = new SurfaceTexture(0);
        surfaceTexture.detachFromGLContext();
        final SurfaceTextureRegistryEntry entry = new SurfaceTextureRegistryEntry(nextTextureId.getAndIncrement(),
                surfaceTexture);
        mNativeView.getFlutterJNI().registerTexture(entry.id(), surfaceTexture);
        return entry;
    }

    final class SurfaceTextureRegistryEntry implements SurfaceTextureEntry {
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
