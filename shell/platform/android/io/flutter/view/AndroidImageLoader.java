package io.flutter.view;

import io.flutter.view.NativeLoadCallback;

/**
 * Interface for those objects that load a image
 */
public interface AndroidImageLoader {

    /**
     * load the given url
     * @param url the image url to load
     * @param callback the callback to notify load result
     */
    void load(String url, NativeLoadCallback callback);
}