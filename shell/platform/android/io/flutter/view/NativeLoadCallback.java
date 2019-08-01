package io.flutter.view;

import android.graphics.Bitmap;

/**
 * Interface for image load callback
 */
public class NativeLoadCallback {

    protected long cppSuccessCallbackPtr;
    protected long cppFailCallbackPtr;

    public NativeLoadCallback(long successCallbackPtr, long failCallbackPtr) {
        cppSuccessCallbackPtr = successCallbackPtr;
        cppFailCallbackPtr = failCallbackPtr;
    }

    public native void nativeSuccessCallback(long cppSuccessCallbackPtr, Bitmap bitmap);

    public native void nativeFailCallback(long cppFailCallbackPtr);

    public void onLoadSuccess(Bitmap bitmap) {
        nativeSuccessCallback(cppSuccessCallbackPtr, bitmap);
    }

    public void onLoadFail() {
        nativeFailCallback(cppFailCallbackPtr);
    }
}