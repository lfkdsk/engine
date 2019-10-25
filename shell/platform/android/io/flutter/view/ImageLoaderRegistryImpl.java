package io.flutter.view;

import io.flutter.view.AndroidImageLoader.RealImageLoader;
import io.flutter.view.FlutterNativeView;
import io.flutter.view.ImageLoaderRegistry;

/**
 * BD ADD:
 * ImageLoaderRegistryImpl
 */
public class ImageLoaderRegistryImpl implements ImageLoaderRegistry {

    private AndroidImageLoader mAndroidImageLoader;
    private FlutterNativeView mNativeView;

    public ImageLoaderRegistryImpl(FlutterNativeView nativeView) {
      mNativeView = nativeView;
    }

    /**
     * register android image loader
     */
    @Override
    public void registerImageLoader(RealImageLoader realImageLoader) {
      ensureAndroidImageLoaderAttached();
      mAndroidImageLoader.registerImageLoader(realImageLoader);
    }
    /**
     * unregister android image loader
     */
    @Override
    public void unRegisterImageLoader() {
      mAndroidImageLoader.unRegisterImageLoader();
    }
    /**
     * initialize android image loader
     */
    private void ensureAndroidImageLoaderAttached() {
      if (mAndroidImageLoader != null) {
        return;
      }

      mAndroidImageLoader = new AndroidImageLoader();
      registerAndroidImageLoader(mAndroidImageLoader);
    }
    /**
     * register android image loader
     */
    private void registerAndroidImageLoader(AndroidImageLoader androidImageLoader) {
        mNativeView.getFlutterJNI().registerAndroidImageLoader(androidImageLoader);
    }
    /**
     * unregister android image loader
     */
    private void unRegisterAndroidImageLoader() {
        mNativeView.getFlutterJNI().unRegisterAndroidImageLoader();
    }


}