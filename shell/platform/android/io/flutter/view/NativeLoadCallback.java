package io.flutter.view;

import android.graphics.Bitmap;

/**
 * BD ADD: Interface for image load callback
 */
public class NativeLoadCallback {

    public native void nativeSuccessCallback(String key, Bitmap bitmap);

    public native void nativeFailCallback(String key);

    public void onLoadSuccess(String key, Bitmap bitmap) {
        nativeSuccessCallback(key, bitmap);
    }

    public void onLoadFail(String key) {
        nativeFailCallback(key);
    }
}